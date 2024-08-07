// Copyright (c) 2024 FRC 6328
// http://github.com/Mechanical-Advantage
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file at
// the root directory of this project.

#include "URCL.h"
#include "URCLDriver.h"

#include <frc/Errors.h>
#include <frc/Notifier.h>
#include <frc/DataLogManager.h>
#include <networktables/NetworkTableInstance.h>
#include <networktables/RawTopic.h>
#include <string_view>
#include <wpi/DataLog.h>
#include <units/time.h>
#include <cstring>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <functional>
#include <iostream>

static constexpr auto period = 20_ms;

bool URCL::running = false;
char* URCL::persistentBuffer = nullptr;
char* URCL::periodicBuffer = nullptr;
nt::RawPublisher URCL::persistentPublisher =
  nt::NetworkTableInstance::GetDefault()
    .GetRawTopic("/URCL/Raw/Persistent")
    .Publish("URCLr2_persistent");
nt::RawPublisher URCL::periodicPublisher =
  nt::NetworkTableInstance::GetDefault()
    .GetRawTopic("/URCL/Raw/Periodic")
    .Publish("URCLr2_periodic");
nt::RawPublisher URCL::aliasesPublisher =
  nt::NetworkTableInstance::GetDefault()
    .GetRawTopic("/URCL/Raw/Aliases")
    .Publish("URCLr2_aliases");
frc::Notifier URCL::notifier{URCL::Periodic};

void URCL::Start() {
  std::map<int, std::string_view> aliases;
  URCL::Start(aliases);
}

void URCL::Start(bool withNT) {
  std::map<int, std::string_view> aliases;
  URCL::Start(aliases, withNT);
}

void URCL::Start(std::map<int, std::string_view> aliases) {
  URCL::Start(aliases, true);
}

void URCL::Start(std::map<int, std::string_view> aliases, bool withNT) {
  URCL::withNT = withNT;

  if (running) {
    FRC_ReportError(frc::err::Error, "{}", "URCL cannot be started multiple times");
    return;
  }

  // Publish aliases
  std::ostringstream aliasesBuilder;
  aliasesBuilder << "{";
  bool firstEntry = true;
  for (auto const& [key, value] : aliases) {
    if (!firstEntry) {
      aliasesBuilder << ",";
    }
    firstEntry = false;
    aliasesBuilder << "\"";
    aliasesBuilder << key;
    aliasesBuilder << "\":\"";
    aliasesBuilder << value;
    aliasesBuilder << "\"";
  }
  aliasesBuilder << "}";
  std::string aliasesString = aliasesBuilder.str();
  std::vector<uint8_t> aliasesVector(aliasesString.size());
  std::memcpy(aliasesVector.data(), aliasesString.c_str(), aliasesString.size());
  aliasesPublisher.Set(aliasesVector);

  // Start driver
  URCLDriver_start();
  persistentBuffer = URCLDriver_getPersistentBuffer();
  periodicBuffer = URCLDriver_getPeriodicBuffer();

  persistentLogEntry = wpi::log::RawLogEntry{frc::DataLogManager::GetLog(), "/URCL/Raw/Persistent", "", "URCLr2_persistent"};
  periodicLogEntry = wpi::log::RawLogEntry{frc::DataLogManager::GetLog(), "/URCL/Raw/Periodic", "", "URCLr2_periodic"};
  aliasesLogEntry = wpi::log::RawLogEntry{frc::DataLogManager::GetLog(), "/URCL/Raw/Aliases", "", "URCLr2_aliases"};
  aliasesLogEntry.Append(aliasesVector);

  // Start notifier
  notifier.SetName("URCL");
  notifier.StartPeriodic(period);
}

void URCL::Periodic() {
  URCLDriver_read();
  uint32_t persistentSize;
  uint32_t periodicSize;
  std::memcpy(&persistentSize, persistentBuffer, 4);
  std::memcpy(&periodicSize, periodicBuffer, 4);
  std::vector<uint8_t> persistentVector(persistentSize);
  std::vector<uint8_t> periodicVector(periodicSize);
  std::memcpy(persistentVector.data(), persistentBuffer + 4, persistentVector.size());
  std::memcpy(periodicVector.data(), periodicBuffer + 4, periodicVector.size());
  if (withNT) {
    persistentPublisher.Set(persistentVector);
    periodicPublisher.Set(periodicVector);
  } else {
    persistentLogEntry.Append(persistentVector);
    periodicLogEntry.Append(periodicVector);
  }
}
