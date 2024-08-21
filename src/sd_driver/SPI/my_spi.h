/* spi.h
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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
//
// Pico includes
#include "pico/stdlib.h"
//
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/mutex.h"
#include "pico/sem.h"
#include "pico/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_FILL_CHAR (0xFF)

// "Class" representing SPIs
typedef struct spi_t {
    spi_inst_t *hw_inst;    // SPI HW
    uint miso_gpio;  // SPI MISO GPIO number (not pin number)
    uint mosi_gpio;
    uint sck_gpio;
    uint baud_rate;
    bool no_miso_gpio_pull_up;

    /* Drive strength levels for GPIO outputs.
        GPIO_DRIVE_STRENGTH_2MA, 
        GPIO_DRIVE_STRENGTH_4MA, 
        GPIO_DRIVE_STRENGTH_8MA,
        GPIO_DRIVE_STRENGTH_12MA
    */
    bool set_drive_strength;
    enum gpio_drive_strength mosi_gpio_drive_strength;
    enum gpio_drive_strength sck_gpio_drive_strength;

    bool use_static_dma_channels;
    uint tx_dma;
    uint rx_dma;

    /* The following fields are not part of the configuration. They are dynamically assigned. */
    dma_channel_config tx_dma_cfg;
    dma_channel_config rx_dma_cfg;
    semaphore_t sem;
    mutex_t mutex;    
    bool initialized;  
} spi_t;

void __not_in_flash_func(spi_transfer_start)(spi_t *spi_p, const uint8_t *tx, uint8_t *rx, size_t length);
bool __not_in_flash_func(spi_transfer_wait_complete)(spi_t *spi_p, uint32_t timeout_ms);
bool __not_in_flash_func(spi_transfer)(spi_t *spi_p, const uint8_t *tx, uint8_t *rx, size_t length);  
void spi_lock(spi_t *spi_p);
void spi_unlock(spi_t *spi_p);
bool my_spi_init(spi_t *spi_p);

/* 
This uses the Pico LED to show SD card activity.
You can use it to get a rough idea of utilization.
Warning: Pico W uses GPIO 25 for SPI communication to the CYW43439.

You can enable this by putting something like
    add_compile_definitions(USE_LED=1)
in CMakeLists.txt, for example.
*/
#if !defined(NO_PICO_LED) && defined(USE_LED) && USE_LED && defined(PICO_DEFAULT_LED_PIN)
#  define LED_INIT()                     \
    {                                    \
        gpio_init(PICO_DEFAULT_LED_PIN);              \
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT); \
    }
#  define LED_ON() gpio_put(PICO_DEFAULT_LED_PIN, 1)
#  define LED_OFF() gpio_put(PICO_DEFAULT_LED_PIN, 0)
#else
#  define LED_ON()
#  define LED_OFF()
#  define LED_INIT()
#endif

#ifdef __cplusplus
}
#endif

/* [] END OF FILE */
