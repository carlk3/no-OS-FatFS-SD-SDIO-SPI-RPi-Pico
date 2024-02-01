/* crash.c
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
#include <stdio.h>
#include <string.h>
#include <time.h>
//
#include "pico/stdlib.h"
#include "RP2040.h"
//
#include "my_debug.h"
#include "rtc.h"
#include "util.h"
//
#include "crash.h"

static volatile crash_info_ram_t crash_info_ram
    __attribute__((section(".uninitialized_data")));

static volatile crash_info_ram_t *p_crash_info_ram = &crash_info_ram;

static crash_info_t previous_crash_info;
static bool _previous_crash_info_valid = false;

void crash_handler_init() {
    volatile crash_info_t *pCrashInfo = &p_crash_info_ram->crash_info;

    if (pCrashInfo->magic == crash_magic_hard_fault ||
        pCrashInfo->magic == crash_magic_stack_overflow ||
        pCrashInfo->magic == crash_magic_reboot_requested ||
        pCrashInfo->magic == crash_magic_assert ||
        pCrashInfo->magic == crash_magic_debug_mon) {
        uint32_t xor_checksum = calculate_checksum(
            (uint32_t *)pCrashInfo, offsetof(crash_info_t, xor_checksum));
        if (xor_checksum == pCrashInfo->xor_checksum) {
            // valid crash record
            memcpy(&previous_crash_info, (void *)pCrashInfo,
                   sizeof previous_crash_info);
            _previous_crash_info_valid = true;
        }
    }
    memset((void *)p_crash_info_ram, 0, sizeof *p_crash_info_ram);
}

const crash_info_t *crash_handler_get_info() {
    if (_previous_crash_info_valid) {
        return &previous_crash_info;
    }
    return NULL;
}

__attribute__((noreturn)) void system_reset_func(char const *const func) {
    memset((void *)p_crash_info_ram, 0, sizeof *p_crash_info_ram);
    p_crash_info_ram->crash_info.magic = crash_magic_reboot_requested;
    p_crash_info_ram->crash_info.timestamp = epochtime;
    snprintf((char *)p_crash_info_ram->crash_info.calling_func,
             sizeof p_crash_info_ram->crash_info.calling_func, "%s", func);
    p_crash_info_ram->crash_info.xor_checksum =
        calculate_checksum((uint32_t *)&p_crash_info_ram->crash_info,
                           offsetof(crash_info_t, xor_checksum));
    __DSB();

    
    NVIC_SystemReset();
    __builtin_unreachable();
}

void capture_assert(const char *file, int line, const char *func,
                    const char *pred) {
    memset((void *)p_crash_info_ram, 0, sizeof *p_crash_info_ram);
    p_crash_info_ram->crash_info.magic = crash_magic_assert;
    p_crash_info_ram->crash_info.timestamp = epochtime;

    // If the filename is too long, take the end:
    size_t full_len = strlen(file) + 1;
    size_t offset = 0;
    if (full_len > sizeof p_crash_info_ram->crash_info.assert.file) {
        offset = full_len - sizeof p_crash_info_ram->crash_info.assert.file;
    }
    snprintf((char *)p_crash_info_ram->crash_info.assert.file,
             sizeof p_crash_info_ram->crash_info.assert.file, "%s",
             file + offset);
    snprintf((char *)p_crash_info_ram->crash_info.assert.func,
             sizeof p_crash_info_ram->crash_info.assert.func, "%s", func);
    snprintf((char *)p_crash_info_ram->crash_info.assert.pred,
             sizeof p_crash_info_ram->crash_info.assert.pred, "%s", pred);
    p_crash_info_ram->crash_info.assert.line = line;
    p_crash_info_ram->crash_info.xor_checksum =
        calculate_checksum((uint32_t *)&p_crash_info_ram->crash_info,
                           offsetof(crash_info_t, xor_checksum));
    __DSB();
    NVIC_SystemReset();
}

__attribute__((noreturn)) void system_request_bootloader_entry(void) {
    memset((void *)p_crash_info_ram, 0, sizeof *p_crash_info_ram);
    p_crash_info_ram->crash_info.magic = crash_magic_bootloader_entry;
    p_crash_info_ram->crash_info.timestamp = epochtime;
    p_crash_info_ram->crash_info.xor_checksum =
        calculate_checksum((uint32_t *)&p_crash_info_ram->crash_info,
                           offsetof(crash_info_t, xor_checksum));
    __DSB();
    NVIC_SystemReset();
    __builtin_unreachable();
}

__attribute__((used)) extern void DebugMon_HandlerC(
    uint32_t const *faultStackAddr) {
    memset((void *)p_crash_info_ram, 0, sizeof *p_crash_info_ram);
    p_crash_info_ram->crash_info.magic = crash_magic_debug_mon;
    p_crash_info_ram->crash_info.timestamp = epochtime;

    /* Stores general registers */
    p_crash_info_ram->crash_info.cy_faultFrame.r0 = faultStackAddr[CY_R0_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.r1 = faultStackAddr[CY_R1_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.r2 = faultStackAddr[CY_R2_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.r3 = faultStackAddr[CY_R3_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.r12 = faultStackAddr[CY_R12_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.lr = faultStackAddr[CY_LR_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.pc = faultStackAddr[CY_PC_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.psr = faultStackAddr[CY_PSR_Pos];

    volatile uint8_t __attribute__((unused)) watchpoint_number = 0;
    // if (DWT->FUNCTION0 & DWT_FUNCTION_MATCHED_Msk) {
    //    watchpoint_number = 0;
    //} else if (DWT->FUNCTION1 & DWT_FUNCTION_MATCHED_Msk) {
    //    watchpoint_number = 1;
    //} else if (DWT->FUNCTION2 & DWT_FUNCTION_MATCHED_Msk) {
    //    watchpoint_number = 2;
    //}
    p_crash_info_ram->crash_info.xor_checksum =
        calculate_checksum((uint32_t *)&p_crash_info_ram->crash_info,
                           offsetof(crash_info_t, xor_checksum));
    __DSB();  // make sure all data is really written into the memory before
              // doing a reset
    // if (DBG_Connected()) {
    //     __BKPT(1);
    // }
    NVIC_SystemReset();
}

extern void DebugMon_Handler(void);
__attribute__((naked)) void DebugMon_Handler(void) {
    __asm volatile(
        " movs r0,#4 \n"
        " movs r1, lr \n"
        " tst r0, r1 \n"
        " beq _MSP2 \n"
        " mrs r0, psp \n"
        " b _HALT2 \n"
        "_MSP2: \n"
        " mrs r0, msp \n"
        "_HALT2: \n"
        " ldr r1,[r0,#20] \n"
        " b DebugMon_HandlerC \n");
}

void Hardfault_HandlerC(uint32_t const *faultStackAddr) {
    memset((void *)p_crash_info_ram, 0, sizeof *p_crash_info_ram);
    p_crash_info_ram->crash_info.magic = crash_magic_hard_fault;
    p_crash_info_ram->crash_info.timestamp = epochtime;

    /* Stores general registers */
    p_crash_info_ram->crash_info.cy_faultFrame.r0 = faultStackAddr[CY_R0_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.r1 = faultStackAddr[CY_R1_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.r2 = faultStackAddr[CY_R2_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.r3 = faultStackAddr[CY_R3_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.r12 = faultStackAddr[CY_R12_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.lr = faultStackAddr[CY_LR_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.pc = faultStackAddr[CY_PC_Pos];
    p_crash_info_ram->crash_info.cy_faultFrame.psr = faultStackAddr[CY_PSR_Pos];

    p_crash_info_ram->crash_info.xor_checksum =
        calculate_checksum((uint32_t *)&p_crash_info_ram->crash_info,
                           offsetof(crash_info_t, xor_checksum));
    __DSB();  // make sure all data is really written into the memory before
              // doing a reset

    NVIC_SystemReset();
}
__attribute__((naked)) void isr_hardfault(void) {
    __asm volatile(
        " movs r0,#4 \n"
        " movs r1, lr \n"
        " tst r0, r1 \n"
        " beq _MSP3 \n"
        " mrs r0, psp \n"
        " b _HALT3 \n"
        "_MSP3: \n"
        " mrs r0, msp \n"
        "_HALT3: \n"
        " ldr r1,[r0,#20] \n"
        " b Hardfault_HandlerC \n");
}

enum {
    crash_info_magic,
    crash_info_hf_lr,
    crash_info_hf_pc,
    crash_info_reset_request_line,
    crash_info_assert
};

int dump_crash_info(crash_info_t const *const pCrashInfo, int next,
                    char *const buf, size_t const buf_sz) {
    int nwrit = 0;
    switch (next) {
        case crash_info_magic:
            nwrit = snprintf(buf, buf_sz, "Event: ");
            switch (pCrashInfo->magic) {
                case crash_magic_none:
                    nwrit += sniprintf(buf + nwrit, buf_sz - nwrit, "\tNone.");
                    next = 0;
                    break;
                case crash_magic_bootloader_entry:
                    nwrit += sniprintf(buf + nwrit, buf_sz - nwrit,
                                       "\tBootloader Entry.");
                    next = 0;
                    break;
                case crash_magic_hard_fault:
                    nwrit += sniprintf(buf + nwrit, buf_sz - nwrit,
                                       "\tCM4 Hard Fault.");
                    next = crash_info_hf_lr;
                    break;
                case crash_magic_debug_mon:
                    nwrit += sniprintf(buf + nwrit, buf_sz - nwrit,
                                       "\tDebug Monitor Watchpoint Triggered.");
                    next = crash_info_hf_lr;
                    break;
                case crash_magic_reboot_requested:
                    nwrit += sniprintf(buf + nwrit, buf_sz - nwrit,
                                       "\tReboot Requested.");
                    next = crash_info_reset_request_line;
                    break;
                case crash_magic_assert:
                    nwrit += sniprintf(buf + nwrit, buf_sz - nwrit,
                                       "\tAssertion Failed.");
                    next = crash_info_assert;
                    break;
                default:
                    printf("Unknown CrashInfo magic: 0x%08lx\n",
                           pCrashInfo->magic);
                    next = 0;
            }
            {
                struct tm tmbuf;
                struct tm *ptm = localtime_r(&pCrashInfo->timestamp, &tmbuf);
                char tsbuf[32];
                size_t n = strftime(tsbuf, sizeof tsbuf,
                                    "\n\tTime: %Y-%m-%d %H:%M:%S\n", ptm);
                myASSERT(n);
                nwrit = snprintf(buf + nwrit, buf_sz - nwrit, "%s", tsbuf);
            }
            break;
        case crash_info_hf_lr:
            nwrit += snprintf(buf + nwrit, buf_sz - nwrit,
                              "\tLink Register (LR): %p\n",
                              (void *)pCrashInfo->cy_faultFrame.lr);
            next = crash_info_hf_pc;
            break;
        case crash_info_hf_pc:
            nwrit += snprintf(buf + nwrit, buf_sz - nwrit,
                              "\tProgram Counter (PC): %p\n",
                              (void *)pCrashInfo->cy_faultFrame.pc);
            next = 0;
            break;
        case crash_info_reset_request_line:
            nwrit += snprintf(buf + nwrit, buf_sz - nwrit,
                              "\tReset request calling function: %s\n",
                              pCrashInfo->calling_func);
            next = 0;
            break;
        case crash_info_assert:
            nwrit += snprintf(buf + nwrit, buf_sz - nwrit,
                              "\tAssertion \"%s\" failed: file \"%s\", line "
                              "%d, function: %s\n",
                              pCrashInfo->assert.pred, pCrashInfo->assert.file,
                              pCrashInfo->assert.line, pCrashInfo->assert.func);
            next = 0;
            break;
        default:
            ASSERT_CASE_NOT(pCrashInfo->magic);
    }
    return next;
}

/* [] END OF FILE */
