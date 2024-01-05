// Copyright (c) 2024 FRC 6328
// http://github.com/Mechanical-Advantage
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file at
// the root directory of this project.

package org.littletonrobotics.urcl;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.function.Supplier;

import edu.wpi.first.networktables.NetworkTableInstance;
import edu.wpi.first.networktables.RawPublisher;
import edu.wpi.first.wpilibj.DriverStation;
import edu.wpi.first.wpilibj.Notifier;

/**
 * <h2>URCL (Unofficial REV-Compatible Logger)</h2>
 * 
 * This unofficial logger enables automatic capture of CAN traffic from REV
 * motor controllers to NetworkTables, viewable using AdvantageScope. See the
 * corresponding <a href=
 * "https://github.com/Mechanical-Advantage/AdvantageScope/blob/main/docs/REV-LOGGING.md">
 * AdvantageScope documentation</a> for more details.
 * 
 * <p>
 * <b>As this library is not an official REV tool, support queries should be
 * directed to the URCL
 * <a href="https://github.com/Mechanical-Advantage/URCL/issues">issues
 * page</a> or software@team6328.org rather than REV's support contact.</b>
 */
public class URCL {
  private static final double period = 0.02;

  private static boolean running = false;
  private static ByteBuffer buffer;
  private static RawPublisher publisher;
  private static Notifier notifier;

  /**
   * Start capturing data from REV motor controllers to NetworkTables. This method
   * should only be called once.
   */
  public static void start() {
    if (running) {
      DriverStation.reportError("URCL cannot be started multiple times", true);
      return;
    }
    running = true;

    buffer = URCLJNI.start();
    buffer.order(ByteOrder.LITTLE_ENDIAN);
    publisher = NetworkTableInstance.getDefault()
        .getRawTopic("/URCL")
        .publish("URCL");

    notifier = new Notifier(() -> publisher.set(getData()));
    notifier.setName("URCL");
    notifier.startPeriodic(period);
  }

  /**
   * Start capturing data from REV motor controllers to an external framework like
   * <a href=
   * "https://github.com/Mechanical-Advantage/AdvantageKit">AdvantageKit</a>. This
   * method should only be called once.
   * 
   * @return The log supplier, to be called periodically
   */
  public static Supplier<ByteBuffer> startExternal() {
    if (running) {
      DriverStation.reportError("URCL cannot be started multiple times", true);
      ByteBuffer dummyBuffer = ByteBuffer.allocate(0);
      return () -> dummyBuffer;
    }
    running = true;

    buffer = URCLJNI.start();
    buffer.order(ByteOrder.LITTLE_ENDIAN);
    return URCL::getData;
  }

  /** Refreshes and reads all data from the queue. */
  private static ByteBuffer getData() {
    URCLJNI.read();
    int size = buffer.getInt(0);
    return buffer.slice(4, size);
  }
}
