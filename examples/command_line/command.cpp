#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//
#include "hardware/clocks.h" 
#include "hardware/rtc.h"
#include "pico/stdlib.h"
// Keep these in order:
#include "ff.h" /* Obtains integer types */
#include "diskio.h" /* Declarations of disk functions */
//
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "sd_card.h"
#include "tests/tests.h"
//
#include "command.h"

static char *saveptr;  // For strtok_r

bool logger_enabled;
const uint32_t period = 1000;
absolute_time_t next_log_time;

#pragma GCC diagnostic ignored "-Wunused-function"
#ifdef NDEBUG 
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static sd_card_t *sd_get_by_name(const char *const name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name)) return sd_get_by_num(i);
    printf("%s: unknown name %s\n", __func__, name);
    return NULL;
}
static FATFS *sd_get_fs_by_name(const char *name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name)) return &sd_get_by_num(i)->fatfs;
    printf("%s: unknown name %s\n", __func__, name);
    return NULL;
}

static void missing_argument_msg() {
    printf("Missing argument\n");
}
static void extra_argument_msg(const char *s) {
    printf("Unexpected argument: %s\n", s);
}
static bool expect_argc(const size_t argc, const char *argv[], const size_t expected) {
    if (argc < expected) {
        missing_argument_msg();
        return false;
    }
    if (argc > expected) {
        extra_argument_msg(argv[expected]);
        return false;
    }
    return true;
}
const char *chk_dflt_log_drv(const size_t argc, const char *argv[]) {
    if (argc > 1) {
        extra_argument_msg(argv[1]);
        return NULL;
    }
    if (!argc) {
        if (1 == sd_get_num()) {
            return sd_get_by_num(0)->pcName;
        } else {
            printf("Missing argument: Specify logical drive\n");
            return NULL;
        }
    } else {
        return argv[0];
    }
}

static void run_setrtc(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 6)) return;

    int date = atoi(argv[0]);
    int month = atoi(argv[1]);
    int year = atoi(argv[2]) + 2000;
    int hour = atoi(argv[3]);
    int min = atoi(argv[4]);
    int sec = atoi(argv[5]);

    datetime_t t = {.year = static_cast<int16_t>(year),
                    .month = static_cast<int8_t>(month),
                    .day = static_cast<int8_t>(date),
                    .dotw = 0,  // 0 is Sunday, so 5 is Friday
                    .hour = static_cast<int8_t>(hour),
                    .min = static_cast<int8_t>(min),
                    .sec = static_cast<int8_t>(sec)};
    rtc_set_datetime(&t);
}
static void run_date(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    char buf[128] = {0};
    time_t epoch_secs = time(NULL);
    struct tm *ptm = localtime(&epoch_secs);
    size_t n = strftime(buf, sizeof(buf), "%c", ptm);
    assert(n);
    printf("%s\n", buf);
    strftime(buf, sizeof(buf), "%j",
             ptm);  // The day of the year as a decimal number (range
                    // 001 to 366).
    printf("Day of year: %s\n", buf);
}
static void run_info(const size_t argc, const char *argv[]) {
    const char *arg = chk_dflt_log_drv(argc, argv);
    if (!arg)
        return;
    sd_card_t *sd_card_p = sd_get_by_name(arg);
    if (!sd_card_p) {
        printf("Unknown logical drive id: \"%s\"\n", arg);
        return;
    }
    if (!sd_card_p->mounted) {
        printf("Drive \"%s\" is not mounted\n", arg);
        return;
    }
    // Card IDendtification register. 128 buts wide.
    cidDmp(sd_card_p, printf);
    // Card-Specific Data register. 128 bits wide.
    csdDmp(sd_card_p, printf);

    // Report cluster size ("allocation unit")
    printf("\nFAT Cluster size (\"allocation unit\"): %d sectors (%llu bytes)\n",
           sd_card_p->fatfs.csize,
           (uint64_t)sd_card_p->fatfs.csize * FF_MAX_SS);
}
static void run_lliot(const size_t argc, const char *argv[]) {
    const char *arg = chk_dflt_log_drv(argc, argv);
    if (!arg)
        return;

    size_t pnum = 0;
    pnum = strtoul(arg, NULL, 0);
    lliot(pnum);
}
static void run_format(const size_t argc, const char *argv[]) {
    const char *arg = chk_dflt_log_drv(argc, argv);
    if (!arg)
        return;

    FATFS *p_fs = sd_get_fs_by_name(arg);
    if (!p_fs) {
        printf("Unknown logical drive id: \"%s\"\n", arg);
        return;
    }
    /* Format the drive with default parameters */
    FRESULT fr = f_mkfs(arg, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr) printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_mount(const size_t argc, const char *argv[]) {
    const char *arg = chk_dflt_log_drv(argc, argv);
    if (!arg)
        return;

    FATFS *p_fs = sd_get_fs_by_name(arg);
    if (!p_fs) {
        printf("Unknown logical drive id: \"%s\"\n", arg);
        return;
    }
    FRESULT fr = f_mount(p_fs, arg, 1);
    if (FR_OK != fr) {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg);
    assert(pSD);
    pSD->mounted = true;
}
static void run_unmount(const size_t argc, const char *argv[]) {
    const char *arg = chk_dflt_log_drv(argc, argv);
    if (!arg)
        return;

    FATFS *p_fs = sd_get_fs_by_name(arg);
    if (!p_fs) {
        printf("Unknown logical drive id: \"%s\"\n", arg);
        return;
    }
    FRESULT fr = f_unmount(arg);
    if (FR_OK != fr) {
        printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg);
    assert(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT;  // in case medium is removed
}
static void run_chdrive(const size_t argc, const char *argv[]) {
    const char *arg = chk_dflt_log_drv(argc, argv);
    if (!arg)
        return;

    FRESULT fr = f_chdrive(arg);
    if (FR_OK != fr) printf("f_chdrive error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_getfree(const size_t argc, const char *argv[]) {
    const char *arg = chk_dflt_log_drv(argc, argv);
    if (!arg)
        return;    

    /* Get volume information and free clusters of drive */
    FATFS *p_fs = sd_get_fs_by_name(arg);
    if (!p_fs) {
        printf("Unknown logical drive id: \"%s\"\n", arg);
        return;
    }
    DWORD fre_clust, fre_sect, tot_sect;
    FRESULT fr = f_getfree(arg, &fre_clust, &p_fs);
    if (FR_OK != fr) {
        printf("f_getfree error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    /* Get total sectors and free sectors */
    tot_sect = (p_fs->n_fatent - 2) * p_fs->csize;
    fre_sect = fre_clust * p_fs->csize;
    /* Print the free space (assuming 512 bytes/sector) */
    printf("%10lu KiB (%lu MiB) total drive space.\n%10lu KiB (%lu MiB) available.\n",
           tot_sect / 2, tot_sect / 2 / 1024,
           fre_sect / 2, fre_sect / 2 / 1024);
}
static void run_cd(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    FRESULT fr = f_chdir(argv[0]);
    if (FR_OK != fr) printf("f_chdir error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_mkdir(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    FRESULT fr = f_mkdir(argv[0]);
    if (FR_OK != fr) printf("f_mkdir error: %s (%d)\n", FRESULT_str(fr), fr);
}
void ls(const char *dir) {
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr; /* Return value */
    char const *p_dir;
    if (dir[0]) {
        p_dir = dir;
    } else {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr) {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        p_dir = cwdbuf;
    }
    printf("Directory Listing: %s\n", p_dir);
    DIR dj = {};      /* Directory object */
    FILINFO fno = {}; /* File information */
    assert(p_dir);
    fr = f_findfirst(&dj, &fno, p_dir, "*");
    if (FR_OK != fr) {
        printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    while (fr == FR_OK && fno.fname[0]) { /* Repeat while an item is found */
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        /* Point pcAttrib to a string that describes the file. */
        if (fno.fattrib & AM_DIR) {
            pcAttrib = pcDirectory;
        } else if (fno.fattrib & AM_RDO) {
            pcAttrib = pcReadOnlyFile;
        } else {
            pcAttrib = pcWritableFile;
        }
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        printf("%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);

        fr = f_findnext(&dj, &fno); /* Search for next item */
    }
    f_closedir(&dj);
}
static void run_ls(const size_t argc, const char *argv[]) {
    if (argc > 1) {
        extra_argument_msg(argv[1]);
        return;
    }
    if (argc)
        ls(argv[0]);
    else
        ls("");
}
static void run_pwd(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    char buf[512];
    FRESULT fr = f_getcwd(buf, sizeof buf);
    if (FR_OK != fr)
        printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
    else
        printf("%s", buf);
}
static void run_cat(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    FIL fil;
    FRESULT fr = f_open(&fil, argv[0], FA_READ);
    if (FR_OK != fr) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    char buf[256];
    while (f_gets(buf, sizeof buf, &fil)) {
        printf("%s", buf);
    }
    fr = f_close(&fil);
    if (FR_OK != fr) printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_cp(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 2)) return;

    FIL fsrc, fdst;    /* File objects */
    BYTE buffer[4096]; /* File copy buffer */
    FRESULT fr;        /* FatFs function common result code */
    UINT br, bw;       /* File read/write count */

    /* Open source file on the drive 1 */
    fr = f_open(&fsrc, argv[0], FA_READ);
    if (FR_OK != fr) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    /* Create destination file on the drive 0 */
    fr = f_open(&fdst, argv[1], FA_WRITE | FA_CREATE_ALWAYS);
    if (FR_OK != fr) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    /* Copy source to destination */
    for (;;) {
        fr = f_read(&fsrc, buffer, sizeof buffer, &br); /* Read a chunk of data from the source file */
        if (FR_OK != fr) printf("f_read error: %s (%d)\n", FRESULT_str(fr), fr);
        if (br == 0) break;                   /* error or eof */
        fr = f_write(&fdst, buffer, br, &bw); /* Write it to the destination file */
        if (FR_OK != fr) printf("f_write error: %s (%d)\n", FRESULT_str(fr), fr);
        if (bw < br) break; /* error or disk full */
    }
    /* Close open files */
    fr = f_close(&fsrc);
    if (FR_OK != fr) printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    fr = f_close(&fdst);
    if (FR_OK != fr) printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_mv(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 2)) return;

    FRESULT fr = f_rename(argv[0], argv[1]);
    if (FR_OK != fr) printf("f_rename error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_big_file_test(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 3)) return;

    const char *pcPathName = argv[0];
    size_t size = strtoul(argv[1], 0, 0);
    uint32_t seed = atoi(argv[2]);
    big_file_test(pcPathName, size, seed);
}
static void del_node(const char *path) {
    FILINFO fno;
    char buff[256];
    /* Directory to be deleted */
    strlcpy(buff, path, sizeof(buff));
    /* Delete the directory */
    FRESULT fr = delete_node(buff, sizeof buff / sizeof buff[0], &fno);
    /* Check the result */
    if (fr) {
        printf("Failed to delete the directory %s. ", path);
        printf("%s error: %s (%d)\n", __func__, FRESULT_str(fr), fr);
    }
}
static void run_del_node(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;
    del_node(argv[0]);
}
static void run_rm(const size_t argc, const char *argv[]) {
    if (argc < 1) {
        missing_argument_msg();
        return;
    }
    if (argc > 2) {
        extra_argument_msg(argv[2]);
        return;
    }
    if (2 == argc) {
        if (0 == strcmp("-r", argv[0])) {
            del_node(argv[1]);
        } else if (0 == strcmp("-d", argv[0])) {
            FRESULT fr = f_unlink(argv[1]);
            if (FR_OK != fr) printf("f_unlink error: %s (%d)\n", FRESULT_str(fr), fr);
        } else {
            EMSG_PRINTF("Unknown option: %s\n", argv[0]);
        }
    } else {
        FRESULT fr = f_unlink(argv[0]);
        if (FR_OK != fr) printf("f_unlink error: %s (%d)\n", FRESULT_str(fr), fr);
    }
}
static void run_simple(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    simple();
}
static void run_bench(const size_t argc, const char *argv[]) {
    const char *arg = chk_dflt_log_drv(argc, argv);
    if (!arg)
        return;

    bench(arg);
}
static void run_cdef(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    f_mkdir("/cdef");  // fake mountpoint
    vCreateAndVerifyExampleFiles("/cdef");
}
static void run_swcwdt(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    vStdioWithCWDTest("/cdef");
}
static void run_loop_swcwdt(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    int cRxedChar = 0;
    do {
        del_node("/cdef");
        f_mkdir("/cdef");  // fake mountpoint
        vCreateAndVerifyExampleFiles("/cdef");
        vStdioWithCWDTest("/cdef");
        cRxedChar = getchar_timeout_us(0);
    } while (PICO_ERROR_TIMEOUT == cRxedChar);
}
static void run_start_logger(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    logger_enabled = true;
    next_log_time = delayed_by_ms(get_absolute_time(), period);
}
static void run_stop_logger(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    logger_enabled = false;
}
static void run_mem_stats(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
 
    // extern ptrdiff_t __StackTop, __StackBottom, __StackOneTop, __StackOneBottom;
    // printf("__StackTop - __StackBottom = %zu\n", __StackTop - __StackBottom);
    // printf("__StackOneTop - __StackOneBottom = %zu\n", __StackOneTop - __StackOneBottom);

    malloc_stats();
}

/* Derived from pico-examples/clocks/hello_48MHz/hello_48MHz.c */
static void run_measure_freqs(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\treported  = %lukHz\n", f_clk_sys, clock_get_hz(clk_sys) / KHZ);
    printf("clk_peri = %dkHz\treported  = %lukHz\n", f_clk_peri, clock_get_hz(clk_peri) / KHZ);
    printf("clk_usb  = %dkHz\treported  = %lukHz\n", f_clk_usb, clock_get_hz(clk_usb) / KHZ);
    printf("clk_adc  = %dkHz\treported  = %lukHz\n", f_clk_adc, clock_get_hz(clk_adc) / KHZ);
    printf("clk_rtc  = %dkHz\treported  = %lukHz\n", f_clk_rtc, clock_get_hz(clk_rtc) / KHZ);

    // Can't measure clk_ref / xosc as it is the ref
}
static void run_set_sys_clock_48mhz(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    set_sys_clock_48mhz();
    setup_default_uart();
}
static void run_set_sys_clock_khz(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    int khz = atoi(argv[0]);

    bool configured = set_sys_clock_khz(khz, false);
    if (!configured) {
        printf("Not possible. Clock not configured.\n");
        return;
    }
    /*
    By default, when reconfiguring the system clock PLL settings after runtime initialization,
    the peripheral clock is switched to the 48MHz USB clock to ensure continuity of peripheral operation.
    There seems to be a problem with running the SPI 2.4 times faster than the system clock,
    even at the same SPI baud rate.
    Anyway, for now, reconfiguring the peripheral clock to the system clock at its new frequency works OK.
    */
    bool ok = clock_configure(clk_peri,
                              0,
                              CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                              clock_get_hz(clk_sys),
                              clock_get_hz(clk_sys));
    assert(ok);

    setup_default_uart();
}

static void clr(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    int gp = atoi(argv[0]);

    gpio_init(gp);
    gpio_set_dir(gp, GPIO_OUT);
    gpio_put(gp, 0);
}
static void set(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    int gp = atoi(argv[0]);

    gpio_init(gp);
    gpio_set_dir(gp, GPIO_OUT);
    gpio_put(gp, 1);
}

static void run_help(const size_t argc, const char *argv[]);

typedef void (*p_fn_t)(const size_t argc, const char *argv[]);
typedef struct {
    char const *const command;
    p_fn_t const function;
    char const *const help;
} cmd_def_t;

static cmd_def_t cmds[] = {
    {"setrtc", run_setrtc,
     "setrtc <DD> <MM> <YY> <hh> <mm> <ss>:\n"
     " Set Real Time Clock\n"
     " Parameters: new date (DD MM YY) new time in 24-hour format "
     "(hh mm ss)\n"
     "\te.g.:setrtc 16 3 21 0 4 0"},
    {"date", run_date, "date:\n Print current date and time"},
    {"format", run_format,
     "format [<drive#:>]:\n"
     " Creates an FAT/exFAT volume on the logical drive.\n"
     "\te.g.: format 0:"},
    {"mount", run_mount,
     "mount [<drive#:>]:\n"
     " Register the work area of the volume\n"
     "\te.g.: mount 0:"},
    {"unmount", run_unmount,
     "unmount <drive#:>:\n"
     " Unregister the work area of the volume"},
    {"chdrive", run_chdrive,
     "chdrive <drive#:>:\n"
     " Changes the current directory of the logical drive.\n"
     " <path> Specifies the directory to be set as current directory.\n"
     "\te.g.: chdrive 1:"},
    {"info", run_info, 
    "info [<drive#:>]:\n"
      " Print information about an SD card"},
    {"getfree", run_getfree,
     "getfree [<drive#:>]:\n"
     " Print the free space on drive"},
    {"cd", run_cd,
     "cd <path>:\n"
     " Changes the current directory of the logical drive.\n"
     " <path> Specifies the directory to be set as current directory.\n"
     "\te.g.: cd /dir1"},
    {"mkdir", run_mkdir,
     "mkdir <path>:\n"
     " Make a new directory.\n"
     " <path> Specifies the name of the directory to be created.\n"
     "\te.g.: mkdir /dir1"},
    // {"del_node", run_del_node,
    //  "del_node <path>:\n"
    //  "  Remove directory and all of its contents.\n"
    //  "  <path> Specifies the name of the directory to be deleted.\n"
    //  "\te.g.: del_node /dir1"},
    {"rm", run_rm,
     "rm [options] <pathname>:\n"
     " Removes (deletes) a file or directory\n"
     " <pathname> Specifies the path to the file or directory to be removed\n"
     " Options:\n"
     " -d Remove an empty directory\n"
     " -r Recursively remove a directory and its contents"},
    {"cp", run_cp,
     "cp <source file> <dest file>:\n"
     " Copies <source file> to <dest file>"},
    {"mv", run_mv,
     "mv <source file> <dest file>:\n"
     " Moves (renames) <source file> to <dest file>"},
    {"pwd", run_pwd,
     "pwd:\n"
     " Print Working Directory"},
    {"ls", run_ls, "ls [pathname]:\n  List directory"},
    {"cat", run_cat, "cat <filename>:\n Type file contents"},
    {"simple", run_simple, "simple:\n Run simple FS tests"},
    {"lliot", run_lliot,
     "lliot <physical drive#>:\n !DESTRUCTIVE! Low Level I/O Driver Test\n"
     "The SD card will need to be reformatted after this test.\n"
     "\te.g.: lliot 1"},
    {"bench", run_bench, "bench <drive#:>:\n A simple binary write/read benchmark"},
    {"big_file_test", run_big_file_test,
     "big_file_test <pathname> <size in MiB> <seed>:\n"
     " Writes random data to file <pathname>.\n"
     " Specify <size in MiB> in units of mebibytes (2^20, or 1024*1024 bytes)\n"
     "\te.g.: big_file_test 0:/bf 1 1\n"
     "\tor: big_file_test 1:big3G-3 3072 3"},
    {"cdef", run_cdef,
     "cdef:\n Create Disk and Example Files\n"
     " Expects card to be already formatted and mounted"},
    {"swcwdt", run_swcwdt,
     "swcwdt:\n Stdio With CWD Test\n"
     "Expects card to be already formatted and mounted.\n"
     "Note: run cdef first!"},
    {"loop_swcwdt", run_loop_swcwdt,
     "loop_swcwdt:\n Run Create Disk and Example Files and Stdio With CWD "
     "Test in a loop.\n"
     "Expects card to be already formatted and mounted.\n"
     "Note: Type any key to quit."},
    {"start_logger", run_start_logger,
     "start_logger:\n"
     " Start Data Log Demo"},
    {"stop_logger", run_stop_logger,
     "stop_logger:\n"
     " Stop Data Log Demo"},
    {"mem-stats", run_mem_stats,
     "mem-stats:\n"
     " Print memory statistics"},
    // // Clocks testing:
    // {"set_sys_clock_48mhz", run_set_sys_clock_48mhz,
    //  "set_sys_clock_48mhz:\n"
    //  " Set the system clock to 48MHz"},
    // {"set_sys_clock_khz", run_set_sys_clock_khz,
    //  "set_sys_clock_khz <khz>:\n"
    //  " Set the system clock system clock frequency in khz."},
    // {"measure_freqs", run_measure_freqs,
    //  "measure_freqs:\n"
    //  " Count the RP2040 clock frequencies and report."},
    // {"clr", clr, "clr <gpio #>: clear a GPIO"},
    // {"set", set, "set <gpio #>: set a GPIO"},

    {"help", run_help,
     "help:\n"
     " Shows this command help."}
};
static void run_help(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    for (size_t i = 0; i < count_of(cmds); ++i) {
        printf("%s\n\n", cmds[i].help);
    }
}

static void process_cmd(char *cmd) {
    char *cmdn = strtok_r(cmd, " ", &saveptr);
    if (cmdn) {
        /* Breaking with Unix tradition of arg[0] being command name,
        arg[0] is first argument after command name */

        size_t argc = 0;
        const char *args[10] = {0};  // Arbitrary limit of 10 arguments
        const char *argp;
        do {
            argp = strtok_r(NULL, " ", &saveptr);
            if (argp) {
                if (argc >= count_of(args) - 1) {
                    extra_argument_msg(argp);
                    return;
                }
                args[argc++] = argp;
            }
        } while (argp);

        size_t i;
        for (i = 0; i < count_of(cmds); ++i) {
            if (0 == strcmp(cmds[i].command, cmdn)) {
                (*cmds[i].function)(argc, args);
                break;
            }
        }
        if (count_of(cmds) == i) printf("Command \"%s\" not found\n", cmdn);
    }
}

void process_stdio(int cRxedChar) {
    static char cmd[256];
    static size_t ix;

    if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
        '\b' != cRxedChar && cRxedChar != (char)127)
        return;
    printf("%c", cRxedChar);  // echo
    stdio_flush();
    if (cRxedChar == '\r') {
        /* Just to space the output from the input. */
        printf("%c", '\n');
        stdio_flush();

        if (!cmd[0]) {  // Empty input
            printf("> ");
            stdio_flush();
            return;
        }

        /* Process the input string received prior to the newline. */
        process_cmd(cmd);

        /* Reset everything for next cmd */
        ix = 0;
        memset(cmd, 0, sizeof cmd);
        printf("\n> ");
        stdio_flush();
    } else {  // Not newline
        if (cRxedChar == '\b' || cRxedChar == (char)127) {
            /* Backspace was pressed.  Erase the last character
             in the string - if any. */
            if (ix > 0) {
                ix--;
                cmd[ix] = '\0';
            }
        } else {
            /* A character was entered.  Add it to the string
             entered so far.  When a \n is entered the complete
             string will be passed to the command interpreter. */
            if (ix < sizeof cmd - 1) {
                cmd[ix] = cRxedChar;
                ix++;
            }
        }
    }
}
