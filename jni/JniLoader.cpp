/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/logging.h>

#include <jni.h>

namespace android {

extern "C" jint initializeDmSocket(JavaVM* vm);

}  // namespace android

using namespace android;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    // Registers CarEvsService's native methods and initializes variables.
    jint ret = JNI_OK;
    ret = initializeDmSocket(vm);
    if (ret < 0) { 
        LOG(ERROR) << __FUNCTION__ << ": Failed to initializeDmSocket";
        return ret;
    }

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* /*reserved*/) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOG(ERROR) << __FUNCTION__ << ": Failed to get the environment.";
        return;
    }
}

