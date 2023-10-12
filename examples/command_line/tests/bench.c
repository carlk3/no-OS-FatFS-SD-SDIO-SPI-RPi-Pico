/* Ported from: https://github.com/greiman/SdFat/blob/master/examples/bench/bench.ino
 *
 * This program is a simple binary write/read benchmark.
 */
#include <my_debug.h>
#include <string.h>

#include "SdCardInfo.h"
#include "SDIO/SdioCard.h"
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"

#define error(s)                       \
    {                                  \
        EMSG_PRINTF("ERROR: %s\n", s); \
        __breakpoint();                \
    }

static uint32_t millis() {
    return to_ms_since_boot(get_absolute_time());
}
static uint64_t micros() {
    return to_us_since_boot(get_absolute_time());
}
static sd_card_t* sd_get_by_name(const char* const name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name)) return sd_get_by_num(i);
    EMSG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

// Set PRE_ALLOCATE true to pre-allocate file clusters.
static const bool PRE_ALLOCATE = true;

// Set SKIP_FIRST_LATENCY true if the first read/write to the SD can
// be avoid by writing a file header or reading the first record.
static const bool SKIP_FIRST_LATENCY = true;

// Size of read/write.
// static const size_t BUF_SIZE = 512;
#define BUF_SIZE (20 * 1024)

// File size in MiB where MiB = 1048576 bytes.
#define FILE_SIZE_MiB 5

// Write pass count.
static const uint8_t WRITE_COUNT = 2;

// Read pass count.
static const uint8_t READ_COUNT = 2;
//==============================================================================
// End of configuration constants.
//------------------------------------------------------------------------------
// File size in bytes.
// static const uint32_t FILE_SIZE = 1000000UL * FILE_SIZE_MB;
#define FILE_SIZE (1024 * 1024 * FILE_SIZE_MiB)

static FIL file;

//------------------------------------------------------------------------------
void bench(char const* logdrv) {
    float s;
    uint32_t t;
    uint32_t maxLatency;
    uint32_t minLatency;
    uint32_t totalLatency;
    bool skipLatency;    

    static_assert(0 == FILE_SIZE % BUF_SIZE, 
            "For accurate results, FILE_SIZE must be a multiple of BUF_SIZE.");

    // Insure 4-byte alignment.
    uint32_t buf32[BUF_SIZE] __attribute__ ((aligned (4)));
    uint8_t* buf = (uint8_t*)buf32;

    sd_card_t *sd_card_p = sd_get_by_name(logdrv);
    if (!sd_card_p) {
        EMSG_PRINTF("Unknown logical drive name: %s\n", logdrv);
        return;
    }
    FRESULT fr = f_chdrive(logdrv);
    if (FR_OK != fr) {
        EMSG_PRINTF("f_chdrive error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    switch (sd_card_p->fatfs.fs_type) {
        case FS_EXFAT:
            IMSG_PRINTF("Type is exFAT\n");
            break;
        case FS_FAT12:
            IMSG_PRINTF("Type is FAT12\n");
            break;
        case FS_FAT16:
            IMSG_PRINTF("Type is FAT16\n");
            break;
        case FS_FAT32:
            IMSG_PRINTF("Type is FAT32\n");
            break;
    }

    IMSG_PRINTF("Card size: ");
    IMSG_PRINTF("%.2f", sd_card_p->get_num_sectors(sd_card_p) * 512E-9);
    IMSG_PRINTF(" GB (GB = 1E9 bytes)\n");

    cidDmp(sd_card_p, info_message_printf);

    // fill buf with known data
    if (BUF_SIZE > 1) {
        for (size_t i = 0; i < (BUF_SIZE - 2); i++) {
            buf[i] = 'A' + (i % 26);
        }
        buf[BUF_SIZE - 2] = '\r';
    }
    buf[BUF_SIZE - 1] = '\n';

    // Open or create file.
    // FA_CREATE_ALWAYS:
    //	Creates a new file. 
    //  If the file is existing, it will be truncated and overwritten.
    fr = f_open(&file, "bench.dat", FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
    if (FR_OK != fr) {
        EMSG_PRINTF("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    if (PRE_ALLOCATE) {
        // prepares or allocates a contiguous data area to the file:
        fr = f_expand(&file, FILE_SIZE, 1);
        if (FR_OK != fr) {
            EMSG_PRINTF("f_expand error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return;
        }
    }
    IMSG_PRINTF("FILE_SIZE_MB = %d\n", FILE_SIZE_MiB);     // << FILE_SIZE_MB << endl;
    IMSG_PRINTF("BUF_SIZE = %zu\n", BUF_SIZE);             // << BUF_SIZE << F(" bytes\n");
    IMSG_PRINTF("Starting write test, please wait.\n\n");  // << endl
                                                      // << endl;
    // do write test
    uint32_t n = FILE_SIZE / BUF_SIZE;
    IMSG_PRINTF("write speed and latency\n");
    IMSG_PRINTF("speed,max,min,avg\n");
    IMSG_PRINTF("KB/Sec,usec,usec,usec\n");
    for (uint8_t nTest = 0; nTest < WRITE_COUNT; nTest++) {
        fr = f_rewind(&file);
        if (FR_OK != fr) {
            EMSG_PRINTF("f_rewind error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return;
        }
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            uint32_t m = micros();
            unsigned int bw;
            fr = f_write(&file, buf, BUF_SIZE, &bw); /* Write it to the destination file */
            if (FR_OK != fr) {
                EMSG_PRINTF("f_write error: %s (%d)\n", FRESULT_str(fr), fr);
                f_close(&file);
                return;
            }
            if (bw < BUF_SIZE) { /* error or disk full */
                error("write failed");
            }
            m = micros() - m;
            totalLatency += m;
            if (skipLatency) {
                // Wait until first write to SD, not just a copy to the cache.
                // skipLatency = file.curPosition() < 512;
                skipLatency = f_tell(&file) < 512;
            } else {
                if (maxLatency < m) {
                    maxLatency = m;
                }
                if (minLatency > m) {
                    minLatency = m;
                }
            }
        }
        fr = f_sync(&file);
        if (FR_OK != fr) {
            EMSG_PRINTF("f_sync error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return;
        }
        t = millis() - t;
        s = f_size(&file);
        IMSG_PRINTF("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        IMSG_PRINTF(",%lu\n", totalLatency / n);
    }
    IMSG_PRINTF("\nStarting read test, please wait.\n");
    IMSG_PRINTF("\nread speed and latency\n");
    IMSG_PRINTF("speed,max,min,avg\n");
    IMSG_PRINTF("KB/Sec,usec,usec,usec\n");

    // do read test
    for (uint8_t nTest = 0; nTest < READ_COUNT; nTest++) {
        fr = f_rewind(&file);
        if (FR_OK != fr) {
            EMSG_PRINTF("f_rewind error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return;
        }
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            buf[BUF_SIZE - 1] = 0;
            uint32_t m = micros();
            unsigned int nr;
            fr = f_read(&file, buf, BUF_SIZE, &nr);
            if (FR_OK != fr) {
                EMSG_PRINTF("f_read error: %s (%d)\n", FRESULT_str(fr), fr);
                f_close(&file);
                return;
            }
            if (nr != BUF_SIZE) {
                error("read failed");
            }
            m = micros() - m;
            totalLatency += m;
            if (buf[BUF_SIZE - 1] != '\n') {
                error("data check error");
            }
            if (skipLatency) {
                skipLatency = false;
            } else {
                if (maxLatency < m) {
                    maxLatency = m;
                }
                if (minLatency > m) {
                    minLatency = m;
                }
            }
        }
        s = f_size(&file);
        t = millis() - t;
        IMSG_PRINTF("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        IMSG_PRINTF(",%lu\n", totalLatency / n);
    }
    IMSG_PRINTF("\nDone\n");
    fr = f_close(&file);
    if (FR_OK != fr) {
        EMSG_PRINTF("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
}
