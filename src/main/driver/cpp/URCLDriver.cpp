// Copyright (c) 2024 FRC 6328
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

constexpr int periodicApiClass = 46;
constexpr int periodicMessageId = ((deviceType & 0x1f) << 24) |
                                  ((manufacturer & 0xff) << 16) |
                                  ((periodicApiClass & 0x3f) << 10);
constexpr int periodicMessageIdMask = 0x1ffffc00;

bool running = false;
char *persistentBuffer;
char *periodicBuffer;
int32_t halStatus;
uint32_t timeOffsetMillis;
uint32_t firmwareStreamHandle;
uint32_t periodicStreamHandle;
uint32_t persistentMessageCount = 0;

uint32_t readCount = 0;
uint64_t devicesFound = 0;
uint64_t devicesFirmwareReceived = 0;
uint64_t devicesCANReady = 0;
HAL_CANHandle devicesCANHandles[64];

void URCLDriver_start(void) {
  if (running)
    return;
  running = true;
  persistentBuffer = (char *)malloc(persistentSize);
  periodicBuffer = (char *)malloc(periodicSize);
  uint32_t fpgaMillis = (HAL_GetFPGATime(&halStatus) / 1000ull) & 0xffffffff;
  timeOffsetMillis = fpgaMillis - HAL_GetCANPacketBaseTime();
  HAL_CAN_OpenStreamSession(&firmwareStreamHandle, firmwareMessageId,
                            firmwareMessageIdMask, maxPersistentMessages,
                            &halStatus);
  HAL_CAN_OpenStreamSession(&periodicStreamHandle, periodicMessageId,
                            periodicMessageIdMask, maxPeriodicMessages,
                            &halStatus);
}

char *URCLDriver_getPersistentBuffer() { return persistentBuffer; }

char *URCLDriver_getPeriodicBuffer() { return periodicBuffer; }

/**
 * Write a persistent message to the buffer, replacing the value if it exists.
 */
void writeMessagePersistent(HAL_CANStreamMessage message) {
  uint16_t messageIdShort = message.messageID & 0xffff;
  uint32_t index = 0;
  while (index < persistentMessageCount) {
    char *messageBuffer =
        persistentBuffer + 4 + (index * persistentMessageSize);
    uint16_t existingMessageId;
    std::memcpy(&existingMessageId, messageBuffer, 2);
    if (messageIdShort == existingMessageId) {
      std::memcpy(messageBuffer + 2, &message.data, 6);
      return;
    }
    index++;
  }

  if (persistentMessageCount < maxPersistentMessages) {
    persistentMessageCount++;
    char *messageBuffer =
        persistentBuffer + 4 + (index * persistentMessageSize);
    std::memcpy(messageBuffer, &messageIdShort, 2);
    std::memcpy(messageBuffer + 2, &message.data, 6);
  }
}

/**
 * Write a periodic message to the buffer at the specified index.
 */
void writeMessagePeriodic(HAL_CANStreamMessage message, uint32_t index) {
  char *messageBuffer = periodicBuffer + 4 + (index * periodicMessageSize);
  uint32_t timestamp = message.timeStamp + timeOffsetMillis;
  uint16_t messageIdShort = message.messageID & 0xffff;
  std::memcpy(messageBuffer + 0, &timestamp, 4);
  std::memcpy(messageBuffer + 4, &messageIdShort, 2);
  std::memcpy(messageBuffer + 6, &message.data, 8);
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
    uint64_t unknownFirmwareDevices = devicesFound & ~devicesFirmwareReceived;
    for (uint8_t i = 0; i < 64; i++) {
      bool unknownFirmware = (unknownFirmwareDevices >> i) & 1;
      if (unknownFirmware && ((devicesCANReady >> i) & 1) == 0) {
        devicesCANHandles[i] =
            HAL_InitializeCAN(manufacturer, i, deviceType, &halStatus);
        devicesCANReady |= (uint64_t)1 << i;
      }
      if (unknownFirmware) {
        HAL_WriteCANRTRFrame(devicesCANHandles[i], 0, firmwareApi, &halStatus);
      }
    }
  }

  HAL_CANStreamMessage
      messages[std::max(maxPersistentMessages, maxPeriodicMessages)];
  uint32_t messageCount = 0;

  // Read firmware messages
  HAL_CAN_ReadStreamSession(firmwareStreamHandle, messages,
                            maxPersistentMessages, &messageCount, &halStatus);
  for (uint32_t i = 0; i < messageCount; i++) {
    writeMessagePersistent(messages[i]);
    uint8_t deviceId = messages[i].messageID & 0x3f;
    devicesFound |= (uint64_t)1 << deviceId;
    devicesFirmwareReceived |= (uint64_t)1 << deviceId;
  }

  // Read periodic messages
  HAL_CAN_ReadStreamSession(periodicStreamHandle, messages, maxPeriodicMessages,
                            &messageCount, &halStatus);
  for (uint32_t i = 0; i < messageCount; i++) {
    writeMessagePeriodic(messages[i], i);
    uint8_t deviceId = messages[i].messageID & 0x3f;
    devicesFound |= (uint64_t)1 << deviceId;
  }

  // Write sizes
  uint32_t persistentSize = persistentMessageCount * persistentMessageSize;
  uint32_t periodicSize = messageCount * periodicMessageSize;
  std::memcpy(persistentBuffer, &persistentSize, 4);
  std::memcpy(periodicBuffer, &periodicSize, 4);
}
} // extern "C"
