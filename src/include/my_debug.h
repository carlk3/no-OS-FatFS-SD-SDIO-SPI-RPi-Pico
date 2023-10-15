/* my_debug.h
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

#ifdef __cplusplus
extern "C" {
#endif

/* USE_PRINTF
If this is defined and not zero, 
these message output functions will use the Pico SDK's stdout.
*/

/* USE_DBG_PRINTF
If this is not defined or is zero or NDEBUG is defined, 
DBG_PRINTF statements will be effectively stripped from the code.
*/

/* Single string output callbacks: send message output somewhere.
To use these, do not define the USE_PRINTF compile definition,
and override these "weak" functions by strongly implementing them in user code.
The weak implementations do nothing.
 */
void put_out_error_message(const char *s);
void put_out_info_message(const char *s);
void put_out_debug_message(const char *s);

// https://gcc.gnu.org/onlinedocs/gcc-3.2.3/cpp/Variadic-Macros.html

int error_message_printf(const char *func, int line, const char *fmt, ...) __attribute__ ((format (__printf__, 3, 4)));
#ifndef EMSG_PRINTF
#define EMSG_PRINTF(fmt, ...) error_message_printf(__func__, __LINE__, fmt, ##__VA_ARGS__)
#endif

int error_message_printf_plain(const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));

int debug_message_printf(const char *func, int line, const char *fmt, ...) __attribute__ ((format (__printf__, 3, 4)));
#ifndef DBG_PRINTF
#  if defined(USE_DBG_PRINTF) && USE_DBG_PRINTF && !defined(NDEBUG)
#    define DBG_PRINTF(fmt, ...) debug_message_printf(__func__, __LINE__, fmt, ##__VA_ARGS__)
#  else
#    define DBG_PRINTF(fmt, ...) (void)0
#  endif
#endif

int info_message_printf(const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
#ifndef IMSG_PRINTF
#define IMSG_PRINTF(fmt, ...) info_message_printf(fmt, ##__VA_ARGS__)
#endif

/* For passing an output function as an argument */
typedef int (*printer_t)(const char* format, ...);

void my_assert_func(const char *file, int line, const char *func, const char *pred);
#define myASSERT(__e) \
    { ((__e) ? (void)0 : my_assert_func(__func__, __LINE__, __func__, #__e)); }

#ifdef __cplusplus
}
#endif
