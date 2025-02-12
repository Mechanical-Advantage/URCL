// Copyright (c) 2025 FRC 6328
// http://github.com/Mechanical-Advantage
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file at
// the root directory of this project.

#include "URCLDriver.h"
#include "jni.h"
#include "org_littletonrobotics_urcl_URCLJNI.h"

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  // Check to ensure the JNI version is valid

  JNIEnv *env;
  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
    return JNI_ERR;

  // In here is also where you store things like class references
  // if they are ever needed
  return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {}

/*
 * Class:     org_littletonrobotics_urcl_URCLJNI
 * Method:    start
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_littletonrobotics_urcl_URCLJNI_start(JNIEnv *env, jclass clazz) {
  URCLDriver_start();
}

/*
 * Class:     org_littletonrobotics_urcl_URCLJNI
 * Method:    getPersistentBuffer
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL
Java_org_littletonrobotics_urcl_URCLJNI_getPersistentBuffer(JNIEnv *env,
                                                            jclass clazz) {
  return env->NewDirectByteBuffer(URCLDriver_getPersistentBuffer(),
                                  persistentSize);
}

/*
 * Class:     org_littletonrobotics_urcl_URCLJNI
 * Method:    getPeriodicBuffer
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL
Java_org_littletonrobotics_urcl_URCLJNI_getPeriodicBuffer(JNIEnv *env,
                                                          jclass clazz) {
  return env->NewDirectByteBuffer(URCLDriver_getPeriodicBuffer(), periodicSize);
}

/*
 * Class:     org_littletonrobotics_urcl_URCLJNI
 * Method:    read
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_littletonrobotics_urcl_URCLJNI_read(JNIEnv *env, jclass clazz) {
  URCLDriver_read();
}
