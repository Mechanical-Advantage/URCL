// Copyright (c) 2025 FRC 6328
// http://github.com/Mechanical-Advantage
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file at
// the root directory of this project.

#include "URCL.h"

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include <frc/DataLogManager.h>
#include <frc/Errors.h>
#include <frc/Notifier.h>
#include <networktables/NetworkTableInstance.h>
#include <networktables/RawTopic.h>
#include <units/time.h>
#include <wpi/DataLog.h>

#include "URCLDriver.h"

static constexpr auto period = 20_ms;

bool URCL::running = false;
char *URCL::persistentBuffer = nullptr;
char *URCL::periodicBuffer = nullptr;
nt::RawPublisher URCL::persistentPublisher;
nt::RawPublisher URCL::periodicPublisher;
nt::RawPublisher URCL::aliasesPublisher;
wpi::log::RawLogEntry URCL::persistentLogEntry;
wpi::log::RawLogEntry URCL::periodicLogEntry;
wpi::log::RawLogEntry URCL::aliasesLogEntry;
frc::Notifier URCL::notifier{URCL::Periodic};

void URCL::Start() {
  std::map<int, std::string_view> aliases;
  URCL::Start(aliases);
}

void URCL::Start(std::map<int, std::string_view> aliases) {
  if (running) {
    FRC_ReportError(frc::err::Error, "{}",
                    "URCL cannot be started multiple times");
    return;
  }

  // Publish aliases
  std::ostringstream aliasesBuilder;
  aliasesBuilder << "{";
  bool firstEntry = true;
  for (auto const &[key, value] : aliases) {
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
  std::memcpy(aliasesVector.data(), aliasesString.c_str(),
              aliasesString.size());

  // Start driver
  URCLDriver_start();
  persistentBuffer = URCLDriver_getPersistentBuffer();
  periodicBuffer = URCLDriver_getPeriodicBuffer();

  // Start publishers
  persistentPublisher = nt::NetworkTableInstance::GetDefault()
                            .GetRawTopic("/URCL/Raw/Persistent")
                            .Publish("URCLr3_persistent");
  periodicPublisher = nt::NetworkTableInstance::GetDefault()
                          .GetRawTopic("/URCL/Raw/Periodic")
                          .Publish("URCLr3_periodic");
  aliasesPublisher = nt::NetworkTableInstance::GetDefault()
                         .GetRawTopic("/URCL/Raw/Aliases")
                         .Publish("URCLr3_aliases");

  aliasesPublisher.Set(aliasesVector);

  // Start notifier
  notifier.SetName("URCL");
  notifier.StartPeriodic(period);
}

void URCL::Start(wpi::log::DataLog &log) {
  std::map<int, std::string_view> aliases;
  URCL::Start(aliases, log);
}

void URCL::Start(std::map<int, std::string_view> aliases,
                 wpi::log::DataLog &log) {
  if (running) {
    FRC_ReportError(frc::err::Error, "{}",
                    "URCL cannot be started multiple times");
    return;
  }

  // Publish aliases
  std::ostringstream aliasesBuilder;
  aliasesBuilder << "{";
  bool firstEntry = true;
  for (auto const &[key, value] : aliases) {
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
  std::memcpy(aliasesVector.data(), aliasesString.c_str(),
              aliasesString.size());

  // Start driver
  URCLDriver_start();
  persistentBuffer = URCLDriver_getPersistentBuffer();
  periodicBuffer = URCLDriver_getPeriodicBuffer();

  persistentLogEntry = wpi::log::RawLogEntry{log, "/URCL/Raw/Persistent", "",
                                             "URCLr3_persistent"};
  periodicLogEntry =
      wpi::log::RawLogEntry{log, "/URCL/Raw/Periodic", "", "URCLr3_periodic"};
  aliasesLogEntry =
      wpi::log::RawLogEntry{log, "/URCL/Raw/Aliases", "", "URCLr3_aliases"};

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
  std::memcpy(persistentVector.data(), persistentBuffer + 4,
              persistentVector.size());
  std::memcpy(periodicVector.data(), periodicBuffer + 4, periodicVector.size());

  if (persistentPublisher && periodicPublisher) {
    persistentPublisher.Set(persistentVector);
    periodicPublisher.Set(periodicVector);
  }

  if (persistentLogEntry && periodicLogEntry) {
    persistentLogEntry.Update(persistentVector);
    periodicLogEntry.Update(periodicVector);
  }
}
