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
#include <networktables/NetworkTableInstance.h>
#include <networktables/RawTopic.h>
#include <wpi/DataLog.h>
#include <units/time.h>
#include <cstring>
#include <stdlib.h>
#include <functional>
#include <iostream>

static constexpr auto period = 20_ms;

bool URCL::running = false;
char* URCL::buffer = nullptr;
nt::RawPublisher URCL::publisher = 
  nt::NetworkTableInstance::GetDefault()
    .GetRawTopic("/URCL")
    .Publish("URCL");
frc::Notifier URCL::notifier{URCL::Periodic};

void URCL::Start() {
  if (running) {
    FRC_ReportError(frc::err::Error, "{}", "URCL cannot be started multiple times");
    return;
  }

  buffer = URCLDriver_start();
  notifier.SetName("URCL");
  notifier.StartPeriodic(period);
}

void URCL::Periodic() {
  URCLDriver_read();
  uint32_t bufferSize;
  std::memcpy(&bufferSize, buffer, 4);
  std::vector<uint8_t> dataVector(bufferSize);
  std::memcpy(dataVector.data(), buffer + 4, dataVector.size());
  publisher.Set(dataVector);
}