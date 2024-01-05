// Copyright (c) 2024 FRC 6328
// http://github.com/Mechanical-Advantage
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file at
// the root directory of this project.

#pragma once

#include <frc/Notifier.h>
#include <networktables/RawTopic.h>

/**
 * URCL (Unofficial REV-Compatible Logger)
 * 
 * This unofficial logger enables automatic capture of CAN traffic from REV
 * motor controllers to NetworkTables, viewable using AdvantageScope. See the
 * corresponding AdvantageScope documentation for more details:
 * https://github.com/Mechanical-Advantage/AdvantageScope/blob/main/docs/REV-LOGGING.md
 * 
 * As this library is not an official REV tool, support queries should be
 * directed to the URCL issues page or software@team6328.org
 * rather than REV's support contact.
 */
class URCL final {
 public:
  URCL() = delete;

  /**
   * Start capturing data from REV motor controllers to NetworkTables. This method
   * should only be called once.
   */
  static void Start();

 private:
  static void Periodic();

  static bool running;
  static char* buffer;
  static nt::RawPublisher publisher;
  static frc::Notifier notifier;
};