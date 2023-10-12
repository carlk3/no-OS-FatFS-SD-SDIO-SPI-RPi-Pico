/* util.h
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
#ifndef _UTIL_H_
#define _UTIL_H_

#include <stddef.h>    
#include <stdint.h>
// #include "hardware/structs/scb.h"
// #include "RP2040.h"
#include "my_debug.h"

// works with negative index
static inline int wrap_ix(int index, int n)
{
    return ((index % n) + n) % n;
}
static inline int mod_floor(int a, int n) {
    return ((a % n) + n) % n;
}

__attribute__((always_inline)) static inline uint32_t calculate_checksum(uint32_t const *p, size_t const size){
	uint32_t checksum = 0;
	for (uint32_t i = 0; i < (size/sizeof(uint32_t))-1; i++){
		checksum ^= *p;
		p++;
	}
	return checksum;
}

// static inline void system_reset() {
//     __NVIC_SystemReset();
// }

static inline void dump_bytes(size_t num, uint8_t bytes[]) {
    DBG_PRINTF("     ");
    for (size_t j = 0; j < 16; ++j) {
        DBG_PRINTF("%02hhx", j);
        if (j < 15)
            DBG_PRINTF(" ");
        else {
            DBG_PRINTF("\n");
        }
    }
    for (size_t i = 0; i < num; i += 16) {
        DBG_PRINTF("%04x ", i);        
        for (size_t j = 0; j < 16 && i + j < num; ++j) {
            DBG_PRINTF("%02hhx", bytes[i + j]);
            if (j < 15)
                DBG_PRINTF(" ");
            else {
                DBG_PRINTF("\n");
            }
        }
    }
    DBG_PRINTF("\n");
}

char const* uint_binary_str(unsigned int number);

static inline uint32_t ext_bits(unsigned char const *data, int msb, int lsb) {
    uint32_t bits = 0;
    uint32_t size = 1 + msb - lsb;
    for (uint32_t i = 0; i < size; i++) {
        uint32_t position = lsb + i;
        uint32_t byte = 15 - (position >> 3);
        uint32_t bit = position & 0x7;
        uint32_t value = (data[byte] >> bit) & 1;
        bits |= value << i;
    }
    return bits;
}

#endif
/* [] END OF FILE */
