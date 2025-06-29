// Copyright (c) 2025 FRC 6328
// http://github.com/Mechanical-Advantage
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file at
// the root directory of this project.

#include "URCLDriver.h"

#include <bit>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <hal/CAN.h>
#include <hal/CANAPI.h>
#include <hal/HALBase.h>

extern "C" {

constexpr uint8_t canBusCount = 5;
constexpr HAL_CANManufacturer manufacturer = HAL_CAN_Man_kREV;
constexpr HAL_CANDeviceType deviceType = HAL_CAN_Dev_kMotorController;

constexpr int firmwareApiClass = 9;
constexpr int firmwareApiIndex = 8;
constexpr int firmwareApi =
    (firmwareApiClass & 0x3f) << 4 | (firmwareApiIndex & 0xf);
constexpr int firmwareMessageId = ((deviceType & 0x1f) << 24) |
                                  ((manufacturer & 0xff) << 16) |
                                  ((firmwareApi & 0x3ff) << 6);
constexpr int firmwareMessageIdMask = 0x1fffffc0;
constexpr HAL_CANMessage firmwareMessage = {
    .flags = 0, .dataSize = 8, .data = {}};

constexpr int periodicApiClass = 46;
constexpr int periodicMessageId = ((deviceType & 0x1f) << 24) |
                                  ((manufacturer & 0xff) << 16) |
                                  ((periodicApiClass & 0x3f) << 10);
constexpr int periodicMessageIdMask = 0x1ffffc00;

bool running = false;
char *persistentBuffer;
char *periodicBuffer;
int32_t halStatus;
uint32_t firmwareStreamHandle[canBusCount];
uint32_t periodicStreamHandle[canBusCount];
uint32_t persistentMessageCount = 0;

uint32_t readCount = 0;
uint64_t devicesFound[canBusCount] = {0, 0, 0, 0, 0};
uint64_t devicesFirmwareReceived[canBusCount] = {0, 0, 0, 0, 0};
uint64_t devicesCANReady[canBusCount] = {0, 0, 0, 0, 0};
HAL_CANHandle devicesCANHandles[canBusCount][64];

void URCLDriver_start(void) {
  if (running)
    return;
  running = true;
  persistentBuffer = (char *)malloc(persistentSize);
  periodicBuffer = (char *)malloc(periodicSize);
  for (uint8_t busId = 0; busId < canBusCount; busId++) {
    firmwareStreamHandle[busId] = HAL_CAN_OpenStreamSession(
        busId, firmwareMessageId, firmwareMessageIdMask, maxPersistentMessages,
        &halStatus);
    periodicStreamHandle[busId] = HAL_CAN_OpenStreamSession(
        busId, periodicMessageId, periodicMessageIdMask, maxPeriodicMessages,
        &halStatus);
  }
}

char *URCLDriver_getPersistentBuffer() { return persistentBuffer; }

char *URCLDriver_getPeriodicBuffer() { return periodicBuffer; }

/**
 * Write a persistent message to the buffer, replacing the value if it exists.
 */
void writeMessagePersistent(uint8_t busId, HAL_CANStreamMessage message) {
  uint16_t messageIdShort = message.messageId & 0xffff;
  uint32_t index = 0;
  while (index < persistentMessageCount) {
    char *messageBuffer =
        persistentBuffer + 4 + (index * persistentMessageSize);
    uint8_t existingBusId;
    uint32_t existingMessageId;
    std::memcpy(&existingBusId, messageBuffer, 1);
    std::memcpy(&existingMessageId, messageBuffer + 1, 2);
    if (busId == existingBusId && messageIdShort == existingMessageId) {
      std::memcpy(messageBuffer + 3, &message.message.message.data, 6);
      return;
    }
    index++;
  }

  if (persistentMessageCount < maxPersistentMessages) {
    persistentMessageCount++;
    char *messageBuffer =
        persistentBuffer + 4 + (index * persistentMessageSize);
    std::memcpy(messageBuffer, &busId, 1);
    std::memcpy(messageBuffer + 1, &messageIdShort, 2);
    std::memcpy(messageBuffer + 3, &message.message.message.data, 6);
  }
}

/**
 * Write a periodic message to the buffer at the specified index.
 */
void writeMessagePeriodic(uint8_t busId, HAL_CANStreamMessage message,
                          uint32_t index) {
  char *messageBuffer = periodicBuffer + 4 + (index * periodicMessageSize);
  uint32_t timestamp = message.message.timeStamp;
  uint16_t messageIdShort = message.messageId & 0xffff;
  std::memcpy(messageBuffer + 0, &timestamp, 4);
  std::memcpy(messageBuffer + 4, &index, 1);
  std::memcpy(messageBuffer + 5, &messageIdShort, 2);
  std::memcpy(messageBuffer + 7, &message.message.message.data, 8);
}

void URCLDriver_read(void) {
  if (!running)
    return;
  if (std::endian::native != std::endian::little) {
    // Little endian expected by AdvantageScope
    return;
  }

  // Request unknown firmware and models every (~400ms)
  readCount += 1;
  if (readCount >= 20) {
    readCount = 0;
    for (uint8_t busId = 0; busId < canBusCount; busId++) {
      uint64_t unknownFirmwareDevices =
          devicesFound[busId] & ~devicesFirmwareReceived[busId];
      for (uint8_t i = 0; i < 64; i++) {
        bool unknownFirmware = (unknownFirmwareDevices >> i) & 1;
        if (unknownFirmware && ((devicesCANReady[busId] >> i) & 1) == 0) {
          devicesCANHandles[busId][i] =
              HAL_InitializeCAN(busId, manufacturer, i, deviceType, &halStatus);
          devicesCANReady[busId] |= (uint64_t)1 << i;
        }
        if (unknownFirmware) {
          HAL_WriteCANRTRFrame(devicesCANHandles[busId][i], firmwareApi,
                               &firmwareMessage, &halStatus);
        }
      }
    }
  }

  HAL_CANStreamMessage
      messages[std::max(maxPersistentMessages, maxPeriodicMessages)];
  uint32_t messageCount = 0;

  // Read firmware messages
  for (uint8_t busId = 0; busId < canBusCount; busId++) {
    HAL_CAN_ReadStreamSession(firmwareStreamHandle[busId], messages,
                              maxPersistentMessages, &messageCount, &halStatus);
    for (uint32_t i = 0; i < messageCount; i++) {
      writeMessagePersistent(busId, messages[i]);
      uint8_t deviceId = messages[i].messageId & 0x3f;
      devicesFound[busId] |= (uint64_t)1 << deviceId;
      devicesFirmwareReceived[busId] |= (uint64_t)1 << deviceId;
    }
  }

  // Read periodic messages
  for (uint8_t busId = 0; busId < canBusCount; busId++) {
    HAL_CAN_ReadStreamSession(periodicStreamHandle[busId], messages,
                              maxPeriodicMessages, &messageCount, &halStatus);
    for (uint32_t i = 0; i < messageCount; i++) {
      writeMessagePeriodic(busId, messages[i], i);
      uint8_t deviceId = messages[i].messageId & 0x3f;
      devicesFound[busId] |= (uint64_t)1 << deviceId;
    }
  }

  // Write sizes
  uint32_t persistentSize = persistentMessageCount * persistentMessageSize;
  uint32_t periodicSize = messageCount * periodicMessageSize;
  std::memcpy(persistentBuffer, &persistentSize, 4);
  std::memcpy(periodicBuffer, &periodicSize, 4);
}
} // extern "C"
