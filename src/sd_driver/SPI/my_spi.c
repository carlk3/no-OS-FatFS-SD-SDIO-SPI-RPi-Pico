/* spi.c
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

#include <stdbool.h>
//
#include "pico/stdlib.h"
//
#include "pico/mutex.h"
#include "delays.h"
#include "hw_config.h"
#include "my_debug.h"
#include "util.h"
//
#include "my_spi.h"

#ifdef NDEBUG
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

/**
 * Starts a SPI transfer by configuring and starting the DMA channels.
 *
 * @param spi_p Pointer to the SPI object.
 * @param tx Pointer to the transmit buffer. If NULL, data will be filled with SPI_FILL_CHAR.
 * @param rx Pointer to the receive buffer. If NULL, data will be ignored.
 * @param length Length of the transfer.
 */
void spi_transfer_start(spi_t *spi_p, const uint8_t *tx, uint8_t *rx, size_t length) {
    // myASSERT(512 == length || 1 == length);
    myASSERT(tx || rx);
    // myASSERT(!(tx && rx));

    // tx write increment is already false
    if (tx) {
        channel_config_set_read_increment(&spi_p->tx_dma_cfg, true);
    } else {
        static const uint8_t dummy = SPI_FILL_CHAR;
        tx = &dummy;
        channel_config_set_read_increment(&spi_p->tx_dma_cfg, false);
    }
    // rx read increment is already false
    if (rx) {
        channel_config_set_write_increment(&spi_p->rx_dma_cfg, true);
    } else {
        static uint8_t dummy = 0xA5;
        rx = &dummy;
        channel_config_set_write_increment(&spi_p->rx_dma_cfg, false);
    }

    dma_channel_configure(spi_p->tx_dma, &spi_p->tx_dma_cfg,
                          &spi_get_hw(spi_p->hw_inst)->dr,  // write address
                          tx,                               // read address
                          length,                           // element count (each element is of
                                                            // size transfer_data_size)
                          false);                           // start
    dma_channel_configure(spi_p->rx_dma, &spi_p->rx_dma_cfg,
                          rx,                               // write address
                          &spi_get_hw(spi_p->hw_inst)->dr,  // read address
                          length,                           // element count (each element is of
                                                            // size transfer_data_size)
                          false);                           // start

    // start them exactly simultaneously to avoid races (in extreme cases
    // the FIFO could overflow)
    dma_start_channel_mask((1u << spi_p->tx_dma) | (1u << spi_p->rx_dma));
}

static bool chk_spi(spi_inst_t *spi_p) {
    bool rc = true;

    if (spi_get_const_hw(spi_p)->sr & SPI_SSPSR_BSY_BITS) {
        DBG_PRINTF("\tSPI is busy\n");
        rc = false;
    }
    if (spi_get_const_hw(spi_p)->sr & SPI_SSPSR_RFF_BITS) {
        DBG_PRINTF("\tSPI Receive FIFO full\n");
        rc = false;
    }
    if (spi_get_const_hw(spi_p)->sr & SPI_SSPSR_RNE_BITS) {
        DBG_PRINTF("\tSPI Receive FIFO not empty\n");
        rc = false;
    }
    if (!(spi_get_const_hw(spi_p)->sr & SPI_SSPSR_TNF_BITS)) {
        DBG_PRINTF("\tSPI Transmit FIFO is full\n");
        rc = false;
    }
    if (!(spi_get_const_hw(spi_p)->sr & SPI_SSPSR_TFE_BITS)) {
        DBG_PRINTF("\tSPI Transmit FIFO is not empty\n");
        rc = false;
    }
    return rc;
}

/**
 * @brief Wait until SPI transfer is complete.
 * @details This function waits until the SPI master completes the transfer
 * or a timeout has occurred. The timeout is specified in milliseconds.
 * If the timeout is reached the function will return false.
 * This function uses busy waiting to check for completion of the transfer.
 *
 * @param spi_p The SPI configuration.
 * @param timeout_ms The timeout in milliseconds.
 * @return true if the transfer is complete, false if the timeout is reached.
 */
bool spi_transfer_wait_complete(spi_t *spi_p, uint32_t timeout_ms) {
    // Record the start time in milliseconds
    uint32_t start = millis();

    // Wait until DMA channels are not busy or timeout is reached
    do {
        tight_loop_contents();
    } while ((dma_channel_is_busy(spi_p->rx_dma) ||
              dma_channel_is_busy(spi_p->tx_dma)) &&
             millis() - start < timeout_ms);

    // Check if the DMA channels are still busy
    bool timed_out = dma_channel_is_busy(spi_p->rx_dma) ||
                     dma_channel_is_busy(spi_p->tx_dma);

    // Print debug information if the DMA channels are still busy
    if (timed_out) {
        DBG_PRINTF("DMA busy wait timed out in %s\n", __FUNCTION__);
    } else {
        // If the DMA channels are not busy, wait for the SPI peripheral to become idle
        start = millis();
        do {
            tight_loop_contents();
        } while (spi_is_busy(spi_p->hw_inst) && millis() - start < timeout_ms);

        // Check if the SPI peripheral is still busy
        timed_out = spi_is_busy(spi_p->hw_inst);

        // Print debug information if the SPI peripheral is still busy
        if (timed_out) {
            DBG_PRINTF("SPI busy wait timed out in %s\n", __FUNCTION__);
        }
    }

    // Check the status of the SPI peripheral
    bool spi_ok = chk_spi(spi_p->hw_inst);

    // Return true if the transfer is complete and the SPI peripheral is in a good state
    return !(timed_out || !spi_ok);
}

/**
 * @brief Perform a simultaneous read and write operation on the SPI bus.
 *
 * @param spi_p Pointer to the SPI object.
 * @param tx Pointer to the transmit buffer. If NULL, SPI_FILL_CHAR is sent as each data
 * element.
 * @param rx Pointer to the receive buffer. If NULL, data is ignored.
 * @param length Number of data elements to transfer.
 * @return true if the transfer is completed successfully within the timeout.
 * @return false if the transfer times out or encounters an error.
 */
bool spi_transfer(spi_t *spi_p, const uint8_t *tx, uint8_t *rx, size_t length) {
    // Start the SPI transfer
    spi_transfer_start(spi_p, tx, rx, length);

    // Wait for the transfer to complete within the specified timeout
    return spi_transfer_wait_complete(spi_p, 2000);
}

void spi_lock(spi_t *spi_p) {
    myASSERT(mutex_is_initialized(&spi_p->mutex));
    mutex_enter_blocking(&spi_p->mutex);
}
void spi_unlock(spi_t *spi_p) {
    myASSERT(mutex_is_initialized(&spi_p->mutex));
    mutex_exit(&spi_p->mutex);
}

/**
 * @brief Initialize the SPI peripheral and DMA channels.
 *
 * @param spi_p Pointer to the SPI object.
 * @return true if the initialization is successful, false otherwise.
 */
bool my_spi_init(spi_t *spi_p) {
    auto_init_mutex(my_spi_init_mutex);
    mutex_enter_blocking(&my_spi_init_mutex);
    if (!spi_p->initialized) {
        //// The SPI may be shared (using multiple SSs); protect it
        if (!mutex_is_initialized(&spi_p->mutex)) mutex_init(&spi_p->mutex);
        spi_lock(spi_p);

        // Defaults:
        if (!spi_p->hw_inst) spi_p->hw_inst = spi0;
        if (!spi_p->baud_rate) spi_p->baud_rate = 12500000;  // default

        /* Configure component */
        // Enable SPI at 100 kHz and connect to GPIOs
        spi_init(spi_p->hw_inst, 100 * 1000);
        spi_set_format(spi_p->hw_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

        gpio_set_function(spi_p->miso_gpio, GPIO_FUNC_SPI);
        gpio_set_function(spi_p->mosi_gpio, GPIO_FUNC_SPI);
        gpio_set_function(spi_p->sck_gpio, GPIO_FUNC_SPI);
        // ss_gpio is initialized in sd_spi_ctor()

        // Slew rate limiting levels for GPIO outputs.
        // enum gpio_slew_rate { GPIO_SLEW_RATE_SLOW = 0, GPIO_SLEW_RATE_FAST = 1 }
        // void gpio_set_slew_rate (uint gpio,enum gpio_slew_rate slew)
        // Default appears to be GPIO_SLEW_RATE_SLOW.
        gpio_set_slew_rate(spi_p->sck_gpio, GPIO_SLEW_RATE_FAST);

        /* Drive strength levels for GPIO outputs:
            enum gpio_drive_strength {
            GPIO_DRIVE_STRENGTH_2MA = 0,
            GPIO_DRIVE_STRENGTH_4MA = 1,
            GPIO_DRIVE_STRENGTH_8MA = 2,
            GPIO_DRIVE_STRENGTH_12MA = 3 }
         enum gpio_drive_strength gpio_get_drive_strength (uint gpio)
        */
        if (spi_p->set_drive_strength) {
            gpio_set_drive_strength(spi_p->mosi_gpio, spi_p->mosi_gpio_drive_strength);
            gpio_set_drive_strength(spi_p->sck_gpio, spi_p->sck_gpio_drive_strength);
        }

        // SD cards' DO MUST be pulled up. However, it might be done externally.
        if (!spi_p->no_miso_gpio_pull_up) gpio_pull_up(spi_p->miso_gpio);

        gpio_set_input_hysteresis_enabled(spi_p->miso_gpio, false);

        // Check if the user has provided DMA channels
        if (spi_p->use_static_dma_channels) {
            // Claim the channels provided
            dma_channel_claim(spi_p->tx_dma);
            dma_channel_claim(spi_p->rx_dma);
        } else {
            // Grab some unused dma channels
            spi_p->tx_dma = dma_claim_unused_channel(true);
            spi_p->rx_dma = dma_claim_unused_channel(true);
        }
        spi_p->tx_dma_cfg = dma_channel_get_default_config(spi_p->tx_dma);
        spi_p->rx_dma_cfg = dma_channel_get_default_config(spi_p->rx_dma);
        channel_config_set_transfer_data_size(&spi_p->tx_dma_cfg, DMA_SIZE_8);
        channel_config_set_transfer_data_size(&spi_p->rx_dma_cfg, DMA_SIZE_8);

        // We set the outbound DMA to transfer from a memory buffer to the SPI
        // transmit FIFO paced by the SPI TX FIFO DREQ The default is for the
        // read address to increment every element (in this case 1 byte -
        // DMA_SIZE_8) and for the write address to remain unchanged.
        channel_config_set_dreq(&spi_p->tx_dma_cfg,
                                spi_get_index(spi_p->hw_inst) ? DREQ_SPI1_TX : DREQ_SPI0_TX);
        channel_config_set_write_increment(&spi_p->tx_dma_cfg, false);

        // We set the inbound DMA to transfer from the SPI receive FIFO to a
        // memory buffer paced by the SPI RX FIFO DREQ We coinfigure the read
        // address to remain unchanged for each element, but the write address
        // to increment (so data is written throughout the buffer)
        channel_config_set_dreq(&spi_p->rx_dma_cfg,
                                spi_get_index(spi_p->hw_inst) ? DREQ_SPI1_RX : DREQ_SPI0_RX);
        channel_config_set_read_increment(&spi_p->rx_dma_cfg, false);

        LED_INIT();

        spi_p->initialized = true;
        spi_unlock(spi_p);
    }
    mutex_exit(&my_spi_init_mutex);
    return true;
}

/* [] END OF FILE */
