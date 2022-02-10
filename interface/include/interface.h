/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef RECOVERY_COMMON_H
#define RECOVERY_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <cutils/properties.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <assert.h>

#define LOG_TAG  "AUTOTEST"
#include <android/log.h>
#include <log/log.h>
#include "sprd_fts_type.h"
#include "sprd_fts_diag.h"
#include "sprd_fts_log.h"

#if 0
#define LOGE(...) fprintf(stderr, "E:" __VA_ARGS__)
#define LOGD(...) fprintf(stderr, "D:" __VA_ARGS__)
#define LOGI(...) fprintf(stderr, "I:" __VA_ARGS__)
#else
#define LOGD(format, ...)  ALOGD(format "\n", ## __VA_ARGS__)
#define LOGE(format, ...)  ALOGE(format "\n", ## __VA_ARGS__)
#define LOGI(format, ...)  ALOGI(format "\n", ## __VA_ARGS__)

#endif

#define STRINGIFY(x) #x
#define EXPAND(x) STRINGIFY(x)

#define FUN_ENTER             LOGD("[ %s ++ ]\n", __FUNCTION__)
#define FUN_EXIT              LOGD("[ %s -- ]\n", __FUNCTION__)

#define AT_ASSERT(x)         \
    { int cnd = (x); if(!cnd) {LOGE("!!!!!!!! %s(), line: %d Asserted !!!!!!!!\n", __FUNCTION__, __LINE__); assert(cnd);} }

#endif  // RECOVERY_COMMON_H
