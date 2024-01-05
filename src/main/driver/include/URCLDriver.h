// Copyright (c) 2024 FRC 6328
// http://github.com/Mechanical-Advantage
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file at
// the root directory of this project.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

constexpr int startSize = 4 + 4 + 4;
constexpr int persistentMessageSize = 8;
constexpr int periodicMessageSize = 14;
constexpr int maxPersistentMessages = 200;
constexpr int maxPeriodicMessages = 500;
constexpr int bufferSize = 
    startSize +
    (persistentMessageSize * maxPersistentMessages) +
    (periodicMessageSize * maxPeriodicMessages);

char* URCLDriver_start();

void URCLDriver_read();

#ifdef __cplusplus
}  // extern "C"
#endif
