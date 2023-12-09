#include <stdio.h>
//
#include "hardware/clocks.h" 
#include "pico/stdlib.h"
//
#include "hw_config.h"
#include "command.h"
#include "f_util.h"
#include "rtc.h"
#include "tests/tests.h"
//
#include "diskio.h" /* Declarations of disk functions */

#ifndef USE_PRINTF
#error This program is useless without standard input and output.
#endif

// If the card is physically removed, unmount the filesystem:
static void card_detect_callback(uint gpio, uint32_t events) {
    static bool busy;
    if (busy) return; // Avoid switch bounce
    busy = true;
    for (size_t i = 0; i < sd_get_num(); ++i) {
        sd_card_t *pSD = sd_get_by_num(i);
        if (pSD->card_detect_gpio == gpio) {
            if (pSD->mounted) {
                DBG_PRINTF("(Card Detect Interrupt: unmounting %s)\n", pSD->pcName);
                FRESULT fr = f_unmount(pSD->pcName);
                if (FR_OK == fr) {
                    pSD->mounted = false;
                } else {
                    printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
                }
            }
            pSD->m_Status |= STA_NOINIT; // in case medium is removed
            sd_card_detect(pSD);
        }
    }
    busy = false;
}

int main() {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 1); // specify that the stream should be unbuffered
    time_init();

    printf("\033[2J\033[H");  // Clear Screen
    printf("\n> ");
    stdio_flush();

    // Implicitly called by disk_initialize, 
    // but called here to set up the GPIOs 
    // before enabling the card detect interrupt:
    sd_init_driver();

    for (size_t i = 0; i < sd_get_num(); ++i) {
        sd_card_t *pSD = sd_get_by_num(i);
        if (pSD->use_card_detect) {
            // Set up an interrupt on Card Detect to detect removal of the card
            // when it happens:
            gpio_set_irq_enabled_with_callback(
                pSD->card_detect_gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                true, &card_detect_callback);
        }
    }

    for (;;) {  // Super Loop
        if (logger_enabled &&
            absolute_time_diff_us(get_absolute_time(), next_log_time) < 0) {
            if (!process_logger()) logger_enabled = false;
            next_log_time = delayed_by_ms(next_log_time, period);
        }
        int cRxedChar = getchar_timeout_us(0);
        /* Get the character from terminal */
        if (PICO_ERROR_TIMEOUT != cRxedChar) process_stdio(cRxedChar);
    }
    return 0;
}
