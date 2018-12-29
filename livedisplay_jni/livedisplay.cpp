/*
 * Copyright 2016, Kostyan_nsk
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "LiveDisplay-JNI"

#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <log/log.h>
#include "jni.h"
#include "JNIHelp.h"

#define LCD_IOR(num, dtype)	_IOR('L', num, dtype)
#define LCD_TUNING_DCPR		LCD_IOR(34, unsigned int)
#define LCD_TUNING_DEV		"/dev/pri_lcd"

static jboolean native_setColors(JNIEnv *env, jobject obj, jstring jcolors)
{
    const char *colors;
    uint32_t ctValue[9] = {0};
    int i, fd, ret;

    if (jcolors == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return JNI_FALSE;
    }

    colors = env->GetStringUTFChars(jcolors, NULL);
    if (colors == NULL) {  // Out of memory
        return JNI_FALSE;
    }
    ALOGV("setColors: %s", colors);

    ret = sscanf(colors, "%u %u %u", &ctValue[0], &ctValue[4], &ctValue[8]);
    env->ReleaseStringUTFChars(jcolors, colors);
    if (ret != 3)
        return JNI_FALSE;

    for (i = 0; i < 9; i += 4) {
        if (ctValue[i] > 256)
            ctValue[i] = 256;
        else
            if (ctValue[i] < 1)
                ctValue[i] = 1;
    }

    fd = open(LCD_TUNING_DEV, O_RDONLY);
    if (fd < 0) {
        ALOGE("setColors: failed to open " LCD_TUNING_DEV ": %d (%s)", errno, strerror(errno));
        return JNI_FALSE;
    }

    ret = ioctl(fd, LCD_TUNING_DCPR, ctValue);
    close(fd);
    if (ret < 0) {
        ALOGE("setColors: ioctl failed: %d (%s)", errno, strerror(errno));
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

static const char *className = "org/cyanogenmod/hardware/Native";

static JNINativeMethod gMethods[] = {
    {"setColors", "(Ljava/lang/String;)Z", (void *)native_setColors},
};

jint JNI_OnLoad(JavaVM* vm, void* /* reserved */)
{
    JNIEnv *env = NULL;
    jclass jclass;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed\n");
        return -1;
    }

    jclass = env->FindClass(className);
    if (jclass == NULL) {
        ALOGE("Native registration unable to find class '%s'", className);
        return -1;
    }
    if (env->RegisterNatives(jclass, gMethods, NELEM(gMethods)) < 0) {
        ALOGE("RegisterNatives failed for '%s'", className);
        return -1;
    }

    return JNI_VERSION_1_4;
}
