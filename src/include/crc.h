/* crc.h
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
/* Derived from:
 * SD/MMC File System Library
 * Copyright (c) 2016 Neil Thiessen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SD_CRC_H
#define SD_CRC_H

#include <stddef.h>
#include <stdint.h>

extern const char m_Crc7Table[];    
__attribute__((optimize("O3")))
static inline char crc7(const uint8_t* data, int length) {
	//Calculate the CRC7 checksum for the specified data block
	char crc = 0;
	for (int i = 0; i < length; i++) {
		crc = m_Crc7Table[(crc << 1) ^ data[i]];
	}

	//Return the calculated checksum
	return crc;
}

unsigned short crc16(uint8_t * data, int length);


#endif

/* [] END OF FILE */
