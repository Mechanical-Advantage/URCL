// Copyright (c) 2024 FRC 6328
// http://github.com/Mechanical-Advantage
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file at
// the root directory of this project.

package org.littletonrobotics.urcl;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicBoolean;

import edu.wpi.first.util.RuntimeLoader;

/**
 * JNI for unofficial URCL
 */
public class URCLJNI {
  static boolean libraryLoaded = false;
  static RuntimeLoader<URCLJNI> loader = null;

  /**
   * Helper class for determining whether or not to load the driver on static
   * initialization.
   */
  public static class Helper {
    private static AtomicBoolean extractOnStaticLoad = new AtomicBoolean(true);

    /**
     * Get whether to load the driver on static init.
     * 
     * @return true if the driver will load on static init
     */
    public static boolean getExtractOnStaticLoad() {
      return extractOnStaticLoad.get();
    }

    /**
     * Set whether to load the driver on static init.
     * 
     * @param load the new value
     */
    public static void setExtractOnStaticLoad(boolean load) {
      extractOnStaticLoad.set(load);
    }
  }

  static {
    if (Helper.getExtractOnStaticLoad()) {
      try {
        loader = new RuntimeLoader<>("URCLDriver", RuntimeLoader.getDefaultExtractionRoot(),
            URCLJNI.class);
        loader.loadLibrary();
      } catch (IOException ex) {
        ex.printStackTrace();
        System.exit(1);
      }
      libraryLoaded = true;
    }
  }

  /**
   * Force load the library.
   * 
   * @throws java.io.IOException thrown if the native library cannot be found
   */
  public static synchronized void forceLoad() throws IOException {
    if (libraryLoaded) {
      return;
    }
    loader = new RuntimeLoader<>("URCLDriver", RuntimeLoader.getDefaultExtractionRoot(),
        URCLJNI.class);
    loader.loadLibrary();
    libraryLoaded = true;
  }

  /** Start logging. */
  public static native void start();

  /** 
   * Get the shared buffer with persistent data.
   * 
   * @return The shared buffer
   */
  public static native ByteBuffer getPersistentBuffer();

  /** 
   * Get the shared buffer with periodic data.
   * 
   * @return The shared buffer
   */
  public static native ByteBuffer getPeriodicBuffer();

  /** Read new data. */
  public static native void read();
}
