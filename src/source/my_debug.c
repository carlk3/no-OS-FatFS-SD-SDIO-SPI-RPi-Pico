/* my_debug.c
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
//
// #include "pico/stdio.h"
//
#include "my_debug.h"

/* Function Attribute ((weak))
The weak attribute causes a declaration of an external symbol to be emitted as a weak symbol rather than a global.
This is primarily useful in defining library functions that can be overridden in user code, though it can also be used with non-function declarations.
The overriding symbol must have the same type as the weak symbol.
https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html

You can override these functions in your application to redirect "stdout"-type messages.
*/

/* Single string output callbacks */

void __attribute__((weak)) put_out_error_message(const char *s) {
    (void)s;
}
void __attribute__((weak)) put_out_info_message(const char *s) {
    (void)s;
}
void __attribute__((weak)) put_out_debug_message(const char *s) {
    (void)s;
}

/* "printf"-style output callbacks */

#if defined(USE_PRINTF) && USE_PRINTF

int __attribute__((weak)) error_message_printf(const char *func, int line, const char *fmt, ...) {
    printf("%s:%d: ", func, line);
    va_list args;
    va_start(args, fmt);
    int cw = vprintf(fmt, args);
    va_end(args);
    // stdio_flush();
    fflush(stdout);
    return cw;
}
int __attribute__((weak)) error_message_printf_plain(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int cw = vprintf(fmt, args);
    va_end(args);
    // stdio_flush();
    fflush(stdout);
    return cw;
}
int __attribute__((weak)) info_message_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int cw = vprintf(fmt, args);
    va_end(args);
    return cw;
}
int __attribute__((weak)) debug_message_printf(const char *func, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int cw = vprintf(fmt, args);
    va_end(args);
    // stdio_flush();
    fflush(stdout);
    return cw;
}

#else

/* These will truncate at 256 bytes. You can tell by checking the return code. */

int __attribute__((weak)) error_message_printf(const char *func, int line, const char *fmt, ...) {
    char buf[256] = {0};
    va_list args;
    va_start(args, fmt);
    int cw = vsnprintf(buf, sizeof(buf), fmt, args);
    put_out_error_message(buf);
    va_end(args);
    return cw;
}
int __attribute__((weak)) error_message_printf_plain(const char *fmt, ...) {
    char buf[256] = {0};
    va_list args;
    va_start(args, fmt);
    int cw = vsnprintf(buf, sizeof(buf), fmt, args);
    put_out_info_message(buf);
    va_end(args);
    return cw;
}
int __attribute__((weak)) info_message_printf(const char *fmt, ...) {
    char buf[256] = {0};
    va_list args;
    va_start(args, fmt);
    int cw = vsnprintf(buf, sizeof(buf), fmt, args);
    put_out_info_message(buf);
    va_end(args);
    return cw;
}
int __attribute__((weak)) debug_message_printf(const char *func, int line, const char *fmt, ...) {
    char buf[256] = {0};
    va_list args;
    va_start(args, fmt);
    int cw = vsnprintf(buf, sizeof(buf), fmt, args);
    put_out_debug_message(buf);
    va_end(args);
    return cw;
}

#endif

void __attribute__((weak)) 
my_assert_func(const char *file, int line, const char *func, const char *pred) {
    error_message_printf_plain("assertion \"%s\" failed: file \"%s\", line %d, function: %s\n",
                         pred, file, line, func);
    __asm volatile("cpsid i" : : : "memory"); /* Disable global interrupts. */
    exit(1);
}
