
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
* Each element of spi_ifs[] must point to an spi_t instance with member "spi".

There should be one element of the sdio_ifs[] array for each SDIO interface object.

There should be one element of the sd_cards[] array for each SD card slot.
* Each element of sd_cards[] must point to its interface with spi_if_p or sdio_if_p.
* The name (pcName) should correspond to the FatFs "logical drive" identifier.
  (See http://elm-chan.org/fsw/ff/doc/filename.html#vol)
  In general, this should correspond to the (zero origin) array index.

*/

/* Hardware configuration for Pico SD Card Development Board
See https://oshwlab.com/carlk3/rp2040-sd-card-dev

See https://docs.google.com/spreadsheets/d/1BrzLWTyifongf_VQCc2IpJqXWtsrjmG7KnIbSBy-CPU/edit?usp=sharing,
tab "Monster", for pin assignments assumed in this configuration file.
*/

//
#include "hw_config.h"
#include "my_debug.h"


// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects (or "chip selects").
static spi_t spis[] = {  // One for each RP2040 SPI component used
    {   // spis[0]
        .hw_inst = spi0,  // RP2040 SPI component
        .sck_gpio = 2,    // GPIO number (not Pico pin number)
        .mosi_gpio = 3,
        .miso_gpio = 4,
        .set_drive_strength = true,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
        .DMA_IRQ_num = DMA_IRQ_0,
        .use_exclusive_DMA_IRQ_handler = true,
        .baud_rate = 12 * 1000 * 1000   // Actual frequency: 10416666.
    },
    {   // spis[1]
        .hw_inst = spi1,  // RP2040 SPI component
        .miso_gpio = 8,   // GPIO number (not Pico pin number)
        .sck_gpio = 10,
        .mosi_gpio = 11,
        .set_drive_strength = true,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
        .DMA_IRQ_num = DMA_IRQ_1,
        .baud_rate = 12 * 1000 * 1000   // Actual frequency: 10416666.
    }
};

/* SPI Interfaces */
static sd_spi_if_t spi_ifs[] = {
    {   // spi_ifs[0]
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 7,     // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA
    },
    {   // spi_ifs[1]
        .spi = &spis[1],   // Pointer to the SPI driving this card
        .ss_gpio = 12,     // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA
    },
    {   // spi_ifs[2]
        .spi = &spis[1],   // Pointer to the SPI driving this card
        .ss_gpio = 13,     // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    }
};
/* SDIO Interfaces */
static sd_sdio_if_t sdio_ifs[] = {
    {   // sdio_ifs[0]
        /*
        Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
        The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
            CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
            As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
              which is -2 in mod32 arithmetic, so:
            CLK_gpio = D0_gpio -2.
            D1_gpio = D0_gpio + 1;
            D2_gpio = D0_gpio + 2;
            D3_gpio = D0_gpio + 3;
        */
        .CMD_gpio = 17,
        .D0_gpio = 18,
        .SDIO_PIO = pio1,
        .DMA_IRQ_num = DMA_IRQ_1,
        .baud_rate = 15 * 1000 * 1000  // 15 MHz
    }
};

/* Hardware Configuration of the SD Card "objects"
    These correspond to SD card sockets
*/
static sd_card_t sd_cards[] = {  // One for each SD card
    {   // sd_cards[0]: Socket sd0
        /* "pcName" is the FatFs "logical drive" identifier.
        (See http://elm-chan.org/fsw/ff/doc/filename.html#vol) */
        .pcName = "0:",
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 9,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true                                 
    },
    {   // sd_cards[1]: Socket sd1
        /* "pcName" is the FatFs "logical drive" identifier.
        (See http://elm-chan.org/fsw/ff/doc/filename.html#vol) */
        .pcName = "1:",
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[1],  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 14,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true                                 
    },
    {   // sd_cards[2]: Socket sd2
        /* "pcName" is the FatFs "logical drive" identifier.
        (See http://elm-chan.org/fsw/ff/doc/filename.html#vol) */
        .pcName = "2:", 
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[2],  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 15,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true                                 
    },
    {   // sd_cards[3]: Socket sd3
        /* "pcName" is the FatFs "logical drive" identifier.
        (See http://elm-chan.org/fsw/ff/doc/filename.html#vol) */
        .pcName = "3:",
        .type = SD_IF_SDIO,
        .sdio_if_p = &sdio_ifs[0],
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 22,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true
    }
};

/* ********************************************************************** */

size_t sd_get_num() { return count_of(sd_cards); }

sd_card_t *sd_get_by_num(size_t num) {
    myASSERT(num < sd_get_num());
    if (num < sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

/* [] END OF FILE */
