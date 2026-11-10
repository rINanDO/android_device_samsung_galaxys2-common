/*
 * Copyright (C) 2026 The LineageOS Project
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

#define LOG_TAG "PowerHAL"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <hardware/hardware.h>
#include <hardware/power.h>
#include <log/log.h>

#define PEGASUSQ_PATH "/sys/devices/system/cpu/cpufreq/pegasusq/"

/* The governor stores these in a 32-bit long, so about 2.1 s at most */
#define INTERACTION_BOOST_NS "1000000000"
#define LAUNCH_BOOST_NS "2000000000"
#define WAKE_BOOST_NS "2000000000"

static bool low_power;

static void sysfs_write(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        /* The directory only exists while pegasusq is the governor */
        if (errno != ENOENT)
            ALOGE("Error opening %s: %s", path, strerror(errno));
        return;
    }

    if (write(fd, value, strlen(value)) < 0)
        ALOGE("Error writing to %s: %s", path, strerror(errno));

    close(fd);
}

static void boost_setup(void)
{
    /* The governor lowers boost_freq if it ever boosts under a lower limit */
    sysfs_write(PEGASUSQ_PATH "boost_freq", "1200000");
    sysfs_write(PEGASUSQ_PATH "boost_mincpus", "2");
}

static void power_init(struct power_module *module __unused)
{
    boost_setup();
}

static void power_set_interactive(struct power_module *module __unused, int on)
{
    if (on) {
        boost_setup();
        /*
         * Waking runs on one slow core otherwise: pegasusq only brings
         * up CPU1 seconds later, after the screen is already on.
         */
        if (!low_power)
            sysfs_write(PEGASUSQ_PATH "boost_lock_time", WAKE_BOOST_NS);
    } else
        sysfs_write(PEGASUSQ_PATH "boost_lock_time", "0");
}

static void power_hint(struct power_module *module __unused, power_hint_t hint,
                       void *data)
{
    switch (hint) {
    case POWER_HINT_INTERACTION:
        if (!low_power)
            sysfs_write(PEGASUSQ_PATH "boost_lock_time", INTERACTION_BOOST_NS);
        break;
    case POWER_HINT_LAUNCH:
        /* Data is only set when the launch starts */
        if (!low_power && data)
            sysfs_write(PEGASUSQ_PATH "boost_lock_time", LAUNCH_BOOST_NS);
        break;
    case POWER_HINT_LOW_POWER:
        low_power = data != NULL;
        if (low_power)
            sysfs_write(PEGASUSQ_PATH "boost_lock_time", "0");
        break;
    default:
        break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "SMDK4210 Power HAL",
        .author = "The LineageOS Project",
        .methods = &power_module_methods,
    },
    .init = power_init,
    .setInteractive = power_set_interactive,
    .powerHint = power_hint,
};
