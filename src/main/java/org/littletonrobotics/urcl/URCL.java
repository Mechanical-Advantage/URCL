// Copyright (c) 2025 FRC 6328
// http://github.com/Mechanical-Advantage
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file at
// the root directory of this project.

package org.littletonrobotics.urcl;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.util.List;
import java.util.Map;
import java.util.function.Supplier;

import edu.wpi.first.datalog.DataLog;
import edu.wpi.first.datalog.RawLogEntry;
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
  private static ByteBuffer persistentBuffer;
  private static ByteBuffer periodicBuffer;
  private static ByteBuffer aliasesBuffer;
  private static RawPublisher persistentPublisher;
  private static RawPublisher periodicPublisher;
  private static RawPublisher aliasesPublisher;
  private static Notifier notifier;
  private static RawLogEntry persistentLogEntry;
  private static RawLogEntry periodicLogEntry;
  private static RawLogEntry aliasLogEntry;

  /**
   * Start capturing data from REV motor controllers to NetworkTables. This method
   * should only be called once.
   */
  public static void start() {
    start(List.of());
  }

  /**
   * Start capturing data from REV motor controllers to a Datalog. This method
   * should only be called once.
   *
   * @param log the DataLog to log to.
   */
  public static void start(DataLog log) {
    start(List.of(), log);
  }

  /**
   * Start capturing data from REV motor controllers to NetworkTables. This method
   * should only be called once.
   *
   * @param aliases An array for each CAN bus mapping CAN IDs to names.
   */
  public static void start(List<Map<Integer, String>> aliases) {
    if (running) {
      DriverStation.reportError("URCL cannot be started multiple times", true);
      return;
    }
    running = true;

    // Update aliases buffer
    updateAliasesBuffer(aliases);

    // Start driver
    URCLJNI.start();
    persistentBuffer = URCLJNI.getPersistentBuffer();
    periodicBuffer = URCLJNI.getPeriodicBuffer();
    persistentBuffer.order(ByteOrder.LITTLE_ENDIAN);
    periodicBuffer.order(ByteOrder.LITTLE_ENDIAN);

    // Start publishers
    persistentPublisher = NetworkTableInstance.getDefault()
        .getRawTopic("/URCL/Raw/Persistent")
        .publish("URCLr4_persistent");
    periodicPublisher = NetworkTableInstance.getDefault()
        .getRawTopic("/URCL/Raw/Periodic")
        .publish("URCLr4_periodic");
    aliasesPublisher = NetworkTableInstance.getDefault()
        .getRawTopic("/URCL/Raw/Aliases")
        .publish("URCLr4_aliases");
    notifier = new Notifier(() -> {
      var data = getData();
      persistentPublisher.set(data[0]);
      periodicPublisher.set(data[1]);
      aliasesPublisher.set(data[2]);
    });
    notifier.setName("URCL");
    notifier.startPeriodic(period);
  }

  /**
   * Start capturing data from REV motor controllers to a DataLog. This method
   * should only be called once.
   *
   * @param aliases An array for each CAN bus mapping CAN IDs to names.
   * @param log     the DataLog to log to. Note using a DataLog means it will not
   *                log to NetworkTables.
   */
  public static void start(List<Map<Integer, String>> aliases, DataLog log) {
    if (running) {
      DriverStation.reportError("URCL cannot be started multiple times", true);
      return;
    }
    running = true;

    // Update aliases buffer
    updateAliasesBuffer(aliases);

    // Start driver
    URCLJNI.start();
    persistentBuffer = URCLJNI.getPersistentBuffer();
    periodicBuffer = URCLJNI.getPeriodicBuffer();
    persistentBuffer.order(ByteOrder.LITTLE_ENDIAN);
    periodicBuffer.order(ByteOrder.LITTLE_ENDIAN);

    persistentLogEntry = new RawLogEntry(log, "URCL/Raw/Persistent", "",
        "URCLr4_persistent");
    periodicLogEntry = new RawLogEntry(log, "/URCL/Raw/Periodic", "", "URCLr4_periodic");
    aliasLogEntry = new RawLogEntry(log, "/URCL/Raw/Aliases", "", "URCLr4_aliases");
    notifier = new Notifier(() -> {
      var data = getData();
      persistentLogEntry.update(data[0]);
      periodicLogEntry.append(data[1]);
      aliasLogEntry.update(data[2]);
    });
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
  public static Supplier<ByteBuffer[]> startExternal() {
    return startExternal(List.of());
  }

  /**
   * Start capturing data from REV motor controllers to an external framework like
   * <a href=
   * "https://github.com/Mechanical-Advantage/AdvantageKit">AdvantageKit</a>. This
   * method should only be called once.
   *
   * @param aliases An array for each CAN bus mapping CAN IDs to names.
   * @return The log supplier, to be called periodically
   */
  public static Supplier<ByteBuffer[]> startExternal(List<Map<Integer, String>> aliases) {
    if (running) {
      DriverStation.reportError("URCL cannot be started multiple times", true);
      ByteBuffer[] emptyOutput = new ByteBuffer[] {
          ByteBuffer.allocate(0),
          ByteBuffer.allocate(0),
          ByteBuffer.allocate(0)
      };
      return () -> emptyOutput;
    }
    running = true;

    // Update aliases buffer
    updateAliasesBuffer(aliases);

    // Start driver
    URCLJNI.start();
    persistentBuffer = URCLJNI.getPersistentBuffer();
    periodicBuffer = URCLJNI.getPeriodicBuffer();
    persistentBuffer.order(ByteOrder.LITTLE_ENDIAN);
    periodicBuffer.order(ByteOrder.LITTLE_ENDIAN);
    return URCL::getData;
  }

  /** Fills the alias data into the aliases buffer as JSON. */
  private static void updateAliasesBuffer(List<Map<Integer, String>> aliases) {
    StringBuilder aliasesBuilder = new StringBuilder();
    aliasesBuilder.append("[");
    boolean firstBus = true;
    for (var busAliases : aliases) {
      if (!firstBus) {
        aliasesBuilder.append(",");
      }
      firstBus = false;
      aliasesBuilder.append("{");
      boolean firstEntry = true;
      for (Map.Entry<Integer, String> entry : busAliases.entrySet()) {
        if (!firstEntry) {
          aliasesBuilder.append(",");
        }
        firstEntry = false;
        aliasesBuilder.append("\"");
        aliasesBuilder.append(entry.getKey().toString());
        aliasesBuilder.append("\":\"");
        aliasesBuilder.append(entry.getValue());
        aliasesBuilder.append("\"");
      }
      aliasesBuilder.append("}");
    }
    aliasesBuilder.append("]");
    aliasesBuffer = Charset.forName("UTF-8").encode(aliasesBuilder.toString());
  }

  /** Refreshes and reads all data from the queues. */
  private static ByteBuffer[] getData() {
    URCLJNI.read();
    int persistentSize = persistentBuffer.getInt(0);
    int periodicSize = periodicBuffer.getInt(0);
    return new ByteBuffer[] {
        persistentBuffer.slice(4, persistentSize),
        periodicBuffer.slice(4, periodicSize),
        aliasesBuffer
    };
  }
}
