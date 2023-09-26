
/* hw_config.c
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

/*
This file should be tailored to match the hardware design.

See 
  https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration

There should be one element of the spi[] array for each RP2040 hardware SPI used.

There should be one element of the spi_ifs[] array for each SPI interface object.

There should be one element of the sd_cards[] array for each SD card slot.
* Each element of sd_cards[] must point to its interface with spi_if_p.
* The name (pcName) should correspond to the FatFs "logical drive" identifier.
(See http://elm-chan.org/fsw/ff/doc/filename.html#vol)
In general, this should correspond to the (zero origin) array index.

*/

/* Hardware configuration for Expansion Module Type A
See https://oshwlab.com/carlk3/rpi-pico-sd-card-expansion-module-1
*/

#include <assert.h>
//
#include "hw_config.h"

// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects.
static spi_t spis[] = {  // One for each RP2040 SPI component used
    {   // [0]
        .hw_inst = spi0,  // SPI component
        .sck_gpio = 2,  // GPIO number (not Pico pin number)
        .mosi_gpio = 3,
        .miso_gpio = 4,
        .set_drive_strength = true,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
        .no_miso_gpio_pull_up = true,
        .DMA_IRQ_num = DMA_IRQ_0,
        .use_exclusive_DMA_IRQ_handler = true,
        .baud_rate = 125 * 1000 * 1000 / 4,  // 31250000 Hz 
    },
};

/* SPI Interfaces */
static sd_spi_if_t spi_ifs[] = {
    {   // [0]
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 7,     // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA
    }
};


// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {   // Socket 0 over SPI
        .pcName = "0:",  // Name used to mount device
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],  // Pointer to the SPI interface driving this card

        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 9,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
    }
};

/* ********************************************************************** */
size_t sd_get_num() { return count_of(sd_cards); }

sd_card_t *sd_get_by_num(size_t num) {
    assert(num <= sd_get_num());
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

size_t spi_get_num() { return count_of(spis); }

spi_t *spi_get_by_num(size_t num) {
    assert(num <= spi_get_num());
    if (num <= spi_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}

/* [] END OF FILE */