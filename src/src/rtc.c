/* rtc.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use 
this file except in compliance with the License. You may obtain a copy of the 
License at

   http://www.apache.org/licenses/LICENSE-2.0 
Unless required by applicable law or agreed to in writing, software distributed 
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR 
CONDITIONS OF ANY KIND, either express or implied. See the License for the 
specific language governing permissions and limitations under the License.
*/
#include <time.h>
//
#include "pico/aon_timer.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
//
#include "ff.h"
#include "util.h"  // calculate_checksum
//
#include "rtc.h"

time_t epochtime;

// Make an attempt to save a recent time stamp across reset:
typedef struct rtc_save {
    uint32_t signature;
    struct timespec ts;
    uint32_t checksum;  // last, not included in checksum
} rtc_save_t;
static rtc_save_t rtc_save __attribute__((section(".uninitialized_data")));

static void update_epochtime() {
    struct tm timeinfo;
    aon_timer_get_time(&rtc_save.ts);
    rtc_save.signature = 0xBABEBABE;
    localtime_r(&rtc_save.ts.tv_sec, &timeinfo);
    rtc_save.checksum = calculate_checksum((uint32_t *)&rtc_save,
                                            offsetof(rtc_save_t, checksum));
    epochtime = mktime(&timeinfo);
    // configASSERT(-1 != epochtime);
}

time_t time(time_t *pxTime) {
    update_epochtime();
    if (pxTime) {
        *pxTime = epochtime;
    }
    return epochtime;
}

void time_init() {
    struct timespec ts;
    aon_timer_get_time(&ts);

    struct tm t, t2;
    localtime_r(&ts.tv_sec, &t);
    localtime_r(&rtc_save.ts.tv_sec, &t2);

    if (t.tm_year != t2.tm_year) {
        uint32_t xor_checksum = calculate_checksum(
            (uint32_t *)&rtc_save, offsetof(rtc_save_t, checksum));
        if (rtc_save.signature == 0xBABEBABE &&
            rtc_save.checksum == xor_checksum) {
            // Set rtc
            aon_timer_set_time(&rtc_save.ts);
        }
    }
}

// Called by FatFs:
DWORD get_fattime(void) {
    struct timespec ts;
    aon_timer_get_time(&ts);

    struct tm t;
    localtime_r(&ts.tv_sec, &t);

    DWORD fattime = 0;
    // bit31:25
    // Year origin from the 1980 (0..127, e.g. 37 for 2017)
    uint8_t yr = t.tm_year - 1980;
    fattime |= (0b01111111 & yr) << 25;
    // bit24:21
    // Month (1..12)
    uint8_t mo = t.tm_mon;
    fattime |= (0b00001111 & mo) << 21;
    // bit20:16
    // Day of the month (1..31)
    uint8_t da = t.tm_mday;
    fattime |= (0b00011111 & da) << 16;
    // bit15:11
    // Hour (0..23)
    uint8_t hr = t.tm_hour;
    fattime |= (0b00011111 & hr) << 11;
    // bit10:5
    // Minute (0..59)
    uint8_t mi = t.tm_min;
    fattime |= (0b00111111 & mi) << 5;
    // bit4:0
    // Second / 2 (0..29, e.g. 25 for 50)
    uint8_t sd = t.tm_sec / 2;
    fattime |= (0b00011111 & sd);
    return fattime;
}
