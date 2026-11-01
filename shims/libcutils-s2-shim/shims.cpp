/*
 * Copyright (C) 2015 The CyanogenMod Project
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

#include <stdlib.h>
#include <string.h>

#include <processgroup/sched_policy.h>
#include <utils/Unicode.h>

/* status_t Parcel::writeString16 */
extern "C" int _ZN7android6Parcel13writeString16EPKDsj();
extern "C" int _ZN7android6Parcel13writeString16EPKtj() {
    return _ZN7android6Parcel13writeString16EPKDsj();
}

extern "C" int get_sched_policy(int tid, SchedPolicy* policy) {
    return SP_BACKGROUND;
}

/* Removed from libcutils, still used by libsec-ril */
extern "C" char16_t* strdup8to16(const char* s, size_t* out_len) {
    if (s == nullptr) return nullptr;

    size_t len = strlen(s);
    ssize_t u16len = utf8_to_utf16_length(reinterpret_cast<const uint8_t*>(s), len);
    if (u16len < 0) return nullptr;

    char16_t* ret = static_cast<char16_t*>(malloc((u16len + 1) * sizeof(char16_t)));
    if (ret == nullptr) return nullptr;

    utf8_to_utf16(reinterpret_cast<const uint8_t*>(s), len, ret, u16len + 1);
    if (out_len != nullptr) *out_len = u16len;
    return ret;
}
