///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2014-2019, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief Tiny printf, sprintf and (v)snprintf implementation, optimized for speed on
//        embedded systems with a very limited resources. These routines are thread
//        safe and reentrant!
//        Use this instead of the bloated standard/newlib printf cause these use
//        malloc for printf (and may not be thread safe).
//
///////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "bkreg.h"
#include "bkstdio.h"
#include "drv_uart.h"

#define USE_STDIO_PRINTF    (0)
#define USE_STDIO_SSCANF    (0)

typedef struct _FILE FILE;

static   char uart_rx_cnt    = 0;
volatile char uart_rx_esc    = 0;
volatile char uart_rx_flag   = 1;
volatile char uart_rx_done   = 0;
volatile char uart_rx_value  = 0;
volatile int  uart_rx_index  = 0;
unsigned char uart_rx_buf[128] __attribute__((aligned(4)));

void int_handler_uart0(void)
{
    uint32_t status = REG_UART0_INT_STATUS;

    REG_UART0_INT_STATUS = status;

    if(status & (MSK_UART0_INT_STATUS_RX_FIFO_NEED_READ | MSK_UART0_INT_STATUS_RX_STOP_END))
    {
        while(REG_UART0_FIFO_STATUS & MSK_UART0_FIFO_STATUS_FIFO_RD_READY)
        {
            bk_uart_rx_int_callback((char)((REG_UART0_FIFO_PORT & MSK_UART0_FIFO_PORT_RX_FIFO_DOUT) >> 8));
        }
    }

    bk_uart_rx_done_callback();
}

void bk_uart_rx_cmd(char* cmd)
{
    uart_rx_done  = 1;
    uart_rx_index = strlen(cmd);
    uart_rx_buf[uart_rx_index] = '\0';
    memcpy(uart_rx_buf, cmd, uart_rx_index);
}

void bk_uart_rx_int_callback(char ch)
{
    uart_rx_cnt++;
    uart_rx_value = ch;

    if(uart_rx_value == (char)'\r')
    {
        if(uart_rx_flag && (uart_rx_index == 0 || uart_rx_buf[0] != 0x01/*BKREG CMD*/))
        {
            uart_rx_done = 1;
            uart_rx_buf[uart_rx_index++] = '\0';
        }
    }
    else if(uart_rx_value == (char)0x80)
    {
        uart_rx_esc = 1;
    }

    if(!uart_rx_done)
    {
        uart_rx_buf[uart_rx_index++] = (unsigned char)uart_rx_value;
        if(uart_rx_index == sizeof(uart_rx_buf)) uart_rx_index = 0;
        if(uart_rx_flag && uart_rx_buf[0] != 0x01)
        {
            if(isprint(uart_rx_value))
            {
                uart_send(PRINT_UART_PORT, (uint8_t*)&uart_rx_value, 1);
            }
            else if(uart_rx_value == (char)0x83)
            {
                uart_send(PRINT_UART_PORT, (uint8_t*)&uart_rx_value, 1);
                uart_rx_index -= 2; if(uart_rx_index < 0) uart_rx_index = 0;
            }
        }
    }
}

void bk_uart_rx_done_callback(void)
{
    extern int bkreg_process(char* buf, int len);

    if(uart_rx_cnt != 1 || uart_rx_esc) uart_rx_value = 0;

    if(bkreg_process((char*)uart_rx_buf, uart_rx_index))
    {
        uart_rx_esc   = 0;
        uart_rx_done  = 0;
        uart_rx_index = 0;
    }

    if(uart_rx_flag == 0) uart_rx_index = 0;

    uart_rx_cnt = 0;
}

uint8_t bk_uart_input_get(void)
{
    uint8_t key;

    while((key = GET_KEY_VALUE()) == 0);

    SET_KEY_VALUE(0);

    return key;
}

int32_t bk_uart_input_digital_number_get(char* hint)
{
    int32_t key;

    if(hint) bk_printf("%s", hint);

    while(isdigit(key = GET_KEY_VALUE()) == 0);

    SET_KEY_VALUE(0);

    return key - '0';
}

int32_t bk_uart_input_digital_value_get(char* hint)
{
    uint8_t keys[32];
    int32_t i = 0, value;

    if(hint) bk_printf("%s", hint);

    while(1)
    {
        uint8_t key = GET_KEY_VALUE();

        if(key)
        {
            if(key == 0x0D)
            {
                keys[i] = 0;
                uart_send(PRINT_UART_PORT, (uint8_t*)"\x0A", 1);
                break;
            }
            else if(isdigit(key))
            {
                keys[i++] = key;
                uart_send(PRINT_UART_PORT, (uint8_t*)&key, 1);
            }

            if(i >= sizeof(keys))
            {
                keys[i - 1] = 0;
                break;
            }

            SET_KEY_VALUE(0);
        }
    }

    SET_KEY_VALUE(0);

    #if 1
    value = strtol((const char*)keys, 0, 0);
    #else
    bk_sscanf((const char*)keys, "%d", (int*)&value);
    #endif

    return value;
}

void bk_putch(char c)
{
    uart_send(PRINT_UART_PORT, (uint8_t*)&c, 1);
}

int fputc(int c, FILE * stream)
{
    bk_putch(c);
	return c;
}

int fgetc(FILE * stream)
{
    while(!(REG_UART0_FIFO_STATUS & MSK_UART0_FIFO_STATUS_FIFO_RD_READY));
	return (REG_UART0_FIFO_PORT >> 8) & 0xFF;
}

#if USE_STDIO_PRINTF

#if 0

FILE __stdin;
FILE __stdout;

int fputc(int c, FILE * stream)
{
    while(!(REG_UART0_FIFO_STATUS & MSK_UART0_FIFO_STATUS_TX_FIFO_FULL));
	REG_UART0_FIFO_PORT = c;
	return c;
}

int fgetc(FILE * stream)
{
    while(!(REG_UART0_FIFO_STATUS & MSK_UART0_FIFO_STATUS_RX_FIFO_EMPTY));
	return (REG_UART0_FIFO_PORT >> 8) & 0xFF;
}

#endif

int bk_printf(const char *fmt, ...)
{
    int  len;
    char dbg_buff[256];

    va_list ap;
    va_start(ap, fmt);
    len = vsprintf(dbg_buff, fmt, ap);
    va_end(ap);

    if(len > 0) uart_send(PRINT_UART_PORT, (uint8_t*)dbg_buff, len);

    return len;
}

int bk_sprintf(char* buffer, const char* format, ...)
{
    #if 1
    va_list ap;
    va_start(ap, format);
    int len = sprintf(buffer, format, ap);
    va_end(ap);
    return len;
    #else
    return 0;
    #endif
}

#else

#define FUNC_PREFIX                         bk_
#define FUNC_POSTFIX
#define FUNC_STRCAT(prefix, func, postfix)  prefix##func##postfix
#define FUNC_RENAME(prefix, func, postfix)  FUNC_STRCAT(prefix, func, postfix)

#define printf                              FUNC_RENAME(FUNC_PREFIX, printf   , FUNC_POSTFIX)
#define sprintf                             FUNC_RENAME(FUNC_PREFIX, sprintf  , FUNC_POSTFIX)
#define snprintf                            FUNC_RENAME(FUNC_PREFIX, snprintf , FUNC_POSTFIX)
#define vprintf                             FUNC_RENAME(FUNC_PREFIX, vprintf  , FUNC_POSTFIX)
#define vsnprintf                           FUNC_RENAME(FUNC_PREFIX, vsnprintf, FUNC_POSTFIX)
#define fctprintf                           FUNC_RENAME(FUNC_PREFIX, fctprintf, FUNC_POSTFIX)

/**
 * Output a character to a custom device like UART, used by the printf() function
 * This function is declared here only. You have to write your custom implementation somewhere
 * \param character Character to output
 */
static inline void _putchar(char character)
{
    uart_send(PRINT_UART_PORT, (uint8_t*)&character, 1);
}

static inline void _putint(int value)
{
    int i, v;
    for(i = 7; i >= 0; i--)
    {
        v = (value >> (4 * i)) & 0xF;
        _putchar(v < 10 ? '0' + v : 'A' + (v - 10));
    }
}

// define this globally (e.g. gcc -DPRINTF_INCLUDE_CONFIG_H ...) to include the
// printf_config.h header file
// default: undefined
#ifdef PRINTF_INCLUDE_CONFIG_H
#include "printf_config.h"
#endif

// 'ntoa' conversion buffer size, this must be big enough to hold one converted
// numeric number including padded zeros (dynamically created on stack)
// default: 32 byte
#ifndef PRINTF_NTOA_BUFFER_SIZE
#define PRINTF_NTOA_BUFFER_SIZE    32U
#endif

// 'ftoa' conversion buffer size, this must be big enough to hold one converted
// float number including padded zeros (dynamically created on stack)
// default: 32 byte
#ifndef PRINTF_FTOA_BUFFER_SIZE
#define PRINTF_FTOA_BUFFER_SIZE    32U
#endif

// support for the floating point type (%f)
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_FLOAT
#define PRINTF_SUPPORT_FLOAT
#endif

// support for exponential floating point notation (%e/%g)
// default: activated
//#define PRINTF_DISABLE_SUPPORT_EXPONENTIAL
#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
#define PRINTF_SUPPORT_EXPONENTIAL
#endif

// define the default floating point precision
// default: 6 digits
#ifndef PRINTF_DEFAULT_FLOAT_PRECISION
#define PRINTF_DEFAULT_FLOAT_PRECISION  6U
#endif

// define the largest float suitable to print with %f
// default: 1e9
#ifndef PRINTF_MAX_FLOAT
#define PRINTF_MAX_FLOAT  1e9
#endif

// support for the long long types (%llu or %p)
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_LONG_LONG
#define PRINTF_SUPPORT_LONG_LONG
#endif

// support for the ptrdiff_t type (%t)
// ptrdiff_t is normally defined in <stddef.h> as long or long long type
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_PTRDIFF_T
#define PRINTF_SUPPORT_PTRDIFF_T
#endif

///////////////////////////////////////////////////////////////////////////////

// internal flag definitions
#define FLAGS_ZEROPAD   (1U <<  0U)
#define FLAGS_LEFT      (1U <<  1U)
#define FLAGS_PLUS      (1U <<  2U)
#define FLAGS_SPACE     (1U <<  3U)
#define FLAGS_HASH      (1U <<  4U)
#define FLAGS_UPPERCASE (1U <<  5U)
#define FLAGS_CHAR      (1U <<  6U)
#define FLAGS_SHORT     (1U <<  7U)
#define FLAGS_LONG      (1U <<  8U)
#define FLAGS_LONG_LONG (1U <<  9U)
#define FLAGS_PRECISION (1U << 10U)
#define FLAGS_ADAPT_EXP (1U << 11U)

// import float.h for DBL_MAX
#if defined(PRINTF_SUPPORT_FLOAT)
#include <float.h>
#endif

// output function type
typedef void (*out_fct_type)(char character, void* buffer, size_t idx, size_t maxlen);

// wrapper (used as buffer) for output function type
typedef struct
{
    void (*fct)(char character, void* arg);
    void* arg;
}out_fct_wrap_type;

// internal buffer output
static inline void _out_buffer(char character, void* buffer, size_t idx, size_t maxlen)
{
    if(idx < maxlen) ((char*)buffer)[idx] = character;
}

// internal null output
static inline void _out_null(char character, void* buffer, size_t idx, size_t maxlen)
{
    (void)character;
    (void)buffer;
    (void)idx;
    (void)maxlen;
}

// internal _putchar wrapper
static inline void _out_char(char character, void* buffer, size_t idx, size_t maxlen)
{
    (void)buffer;
    (void)idx;
    (void)maxlen;

    if(character) _putchar(character);
}

// internal output function wrapper
static inline void _out_fct(char character, void* buffer, size_t idx, size_t maxlen)
{
    (void)idx;
    (void)maxlen;

    if(character)
    {
        // buffer is the output fct pointer
        ((out_fct_wrap_type*)buffer)->fct(character, ((out_fct_wrap_type*)buffer)->arg);
    }
}

// internal secure strlen
// \return The length of the string (excluding the terminating 0) limited by 'maxsize'
static inline unsigned int _strnlen_s(const char* str, size_t maxsize)
{
    const char* s;
    for(s = str; *s && maxsize--; ++s);
    return (unsigned int)(s - str);
}

// internal test if char is a digit (0-9)
// \return true if char is a digit
static inline bool _is_digit(char ch)
{
    return (ch >= '0') && (ch <= '9');
}

// internal ASCII string to unsigned int conversion
static unsigned int _atoi(const char** str)
{
    unsigned int i = 0U;
    while(_is_digit(**str))
    {
        i = i * 10U + (unsigned int)(*((*str)++) - '0');
    }
    return i;
}

// output the specified string in reverse, taking care of any zero-padding
static size_t _out_rev(out_fct_type out, char* buffer, size_t idx, size_t maxlen, const char* buf, size_t len, unsigned int width, unsigned int flags)
{
    const size_t start_idx = idx;

    // pad spaces up to given width
    if(!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD))
    {
        size_t i;
        for(i = len; i < width; i++)
        {
            out(' ', buffer, idx++, maxlen);
        }
    }

    // reverse string
    while(len)
    {
        out(buf[--len], buffer, idx++, maxlen);
    }

    // append pad spaces up to given width
    if(flags & FLAGS_LEFT)
    {
        while(idx - start_idx < width)
        {
            out(' ', buffer, idx++, maxlen);
        }
    }

    return idx;
}

// internal itoa format
static size_t _ntoa_format(out_fct_type out, char* buffer, size_t idx, size_t maxlen, char* buf, size_t len, bool negative, unsigned int base, unsigned int prec, unsigned int width, unsigned int flags)
{
    // pad leading zeros
    if(!(flags & FLAGS_LEFT))
    {
        if(width && (flags & FLAGS_ZEROPAD) && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) width--;
        while((len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE)) buf[len++] = '0';
        while((flags & FLAGS_ZEROPAD) && (len < width)  && (len < PRINTF_NTOA_BUFFER_SIZE)) buf[len++] = '0';
    }

    // handle hash
    if(flags & FLAGS_HASH)
    {
        if(!(flags & FLAGS_PRECISION) && len && ((len == prec) || (len == width)))
        {
            len--;
            if(len && (base == 16U))
            {
                len--;
            }
        }
        if((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE))
        {
            buf[len++] = 'x';
        }
        else if((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE))
        {
            buf[len++] = 'X';
        }
        else if((base == 2U) && (len < PRINTF_NTOA_BUFFER_SIZE))
        {
            buf[len++] = 'b';
        }
        if(len < PRINTF_NTOA_BUFFER_SIZE)
        {
            buf[len++] = '0';
        }
    }

    if(len < PRINTF_NTOA_BUFFER_SIZE)
    {
        if(negative)
        {
            buf[len++] = '-';
        }
        else if(flags & FLAGS_PLUS)
        {
            buf[len++] = '+';  // ignore the space if the '+' exists
        }
        else if(flags & FLAGS_SPACE)
        {
            buf[len++] = ' ';
        }
    }

    return _out_rev(out, buffer, idx, maxlen, buf, len, width, flags);
}

// internal itoa for 'long' type
static size_t _ntoa_long(out_fct_type out, char* buffer, size_t idx, size_t maxlen, unsigned long value, bool negative, unsigned long base, unsigned int prec, unsigned int width, unsigned int flags)
{
    char buf[PRINTF_NTOA_BUFFER_SIZE];
    size_t len = 0U;

    // no hash for 0 values
    if(!value)
    {
        flags &= ~FLAGS_HASH;
    }

    // write if precision != 0 and value is != 0
    if(!(flags & FLAGS_PRECISION) || value)
    {
        do
        {
            const char digit = (char)(value % base);
            buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
            value /= base;
        }while(value && (len < PRINTF_NTOA_BUFFER_SIZE));
    }

    return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec, width, flags);
}

// internal itoa for 'long long' type
#if defined(PRINTF_SUPPORT_LONG_LONG)
static size_t _ntoa_long_long(out_fct_type out, char* buffer, size_t idx, size_t maxlen, unsigned long long value, bool negative, unsigned long long base, unsigned int prec, unsigned int width, unsigned int flags)
{
    char buf[PRINTF_NTOA_BUFFER_SIZE];
    size_t len = 0U;

    // no hash for 0 values
    if(!value)
    {
        flags &= ~FLAGS_HASH;
    }

    // write if precision != 0 and value is != 0
    if(!(flags & FLAGS_PRECISION) || value)
    {
        do
        {
            const char digit = (char)(value % base);
            buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
            value /= base;
        }while(value && (len < PRINTF_NTOA_BUFFER_SIZE));
    }

    return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec, width, flags);
}
#endif  // PRINTF_SUPPORT_LONG_LONG

#if defined(PRINTF_SUPPORT_FLOAT)

#if defined(PRINTF_SUPPORT_EXPONENTIAL)
// forward declaration so that _ftoa can switch to exp notation for values > PRINTF_MAX_FLOAT
static size_t _etoa(out_fct_type out, char* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width, unsigned int flags);
#endif

// internal ftoa for fixed decimal floating point
static size_t _ftoa(out_fct_type out, char* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width, unsigned int flags)
{
    char buf[PRINTF_FTOA_BUFFER_SIZE];
    size_t len = 0U;
    double diff = 0.0;

    // powers of 10
    static const double pow10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

    // test for special values
    if(value != value)   return _out_rev(out, buffer, idx, maxlen, "nan", 3, width, flags);
    if(value < -DBL_MAX) return _out_rev(out, buffer, idx, maxlen, "fni-", 4, width, flags);
    if(value >  DBL_MAX) return _out_rev(out, buffer, idx, maxlen, (flags & FLAGS_PLUS) ? "fni+" : "fni", (flags & FLAGS_PLUS) ? 4U : 3U, width, flags);

    // test for very large values
    // standard printf behavior is to print EVERY whole number digit -- which could be 100s of characters overflowing your buffers == bad
    if((value > PRINTF_MAX_FLOAT) || (value < -PRINTF_MAX_FLOAT))
    {
        #if defined(PRINTF_SUPPORT_EXPONENTIAL)
        return _etoa(out, buffer, idx, maxlen, value, prec, width, flags);
        #else
        return 0U;
        #endif
    }

    // test for negative
    bool negative = false;
    if(value < 0)
    {
        negative = true;
        value = 0 - value;
    }

    // set default precision, if not set explicitly
    if(!(flags & FLAGS_PRECISION))
    {
        prec = PRINTF_DEFAULT_FLOAT_PRECISION;
    }
    // limit precision to 9, cause a prec >= 10 can lead to overflow errors
    while((len < PRINTF_FTOA_BUFFER_SIZE) && (prec > 9U))
    {
        buf[len++] = '0';
        prec--;
    }

    #if defined(__BA2__)
    int whole = (int)(value - 0.5);
    #else
    int whole = (int)(value);
    #endif
    double tmp = (value - whole) * pow10[prec];
    unsigned long frac = (unsigned long)tmp;
    diff = tmp - frac;

    if(diff > 0.5)
    {
        ++frac;
        // handle rollover, e.g. case 0.99 with prec 1 is 1.0
        if(frac >= pow10[prec])
        {
            frac = 0;
            ++whole;
        }
    }
    else if(diff < 0.5)
    {
    }
    else if((frac == 0U) || (frac & 1U))
    {
        // if halfway, round up if odd OR if last digit is 0
        ++frac;
    }

    if(prec == 0U)
    {
        diff = value - (double)whole;
        if((!(diff < 0.5) || (diff > 0.5)) && (whole & 1))
        {
            // exactly 0.5 and ODD, then round up
            // 1.5 -> 2, but 2.5 -> 2
            ++whole;
        }
    }
    else
    {
        unsigned int count = prec;
        // now do fractional part, as an unsigned number
        while(len < PRINTF_FTOA_BUFFER_SIZE)
        {
            --count;
            buf[len++] = (char)(48U + (frac % 10U));
            if(!(frac /= 10U))
            {
                break;
            }
        }
        // add extra 0s
        while((len < PRINTF_FTOA_BUFFER_SIZE) && (count-- > 0U))
        {
            buf[len++] = '0';
        }
        if(len < PRINTF_FTOA_BUFFER_SIZE)
        {
            // add decimal
            buf[len++] = '.';
        }
    }

    // do whole part, number is reversed
    while(len < PRINTF_FTOA_BUFFER_SIZE)
    {
        buf[len++] = (char)(48 + (whole % 10));
        if(!(whole /= 10))
        {
            break;
        }
    }

    // pad leading zeros
    if(!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD))
    {
        if(width && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE))))
        {
            width--;
        }
        while((len < width) && (len < PRINTF_FTOA_BUFFER_SIZE))
        {
            buf[len++] = '0';
        }
    }

    if(len < PRINTF_FTOA_BUFFER_SIZE)
    {
        if(negative)
        {
            buf[len++] = '-';
        }
        else if(flags & FLAGS_PLUS)
        {
            buf[len++] = '+';  // ignore the space if the '+' exists
        }
        else if(flags & FLAGS_SPACE)
        {
            buf[len++] = ' ';
        }
    }

    return _out_rev(out, buffer, idx, maxlen, buf, len, width, flags);
}

#if defined(PRINTF_SUPPORT_EXPONENTIAL)
// internal ftoa variant for exponential floating-point type, contributed by Martijn Jasperse <m.jasperse@gmail.com>
static size_t _etoa(out_fct_type out, char* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width, unsigned int flags)
{
    // check for NaN and special values
    if((value != value) || (value > DBL_MAX) || (value < -DBL_MAX))
    {
        return _ftoa(out, buffer, idx, maxlen, value, prec, width, flags);
    }

    // determine the sign
    const bool negative = value < 0;
    if(negative)
    {
        value = -value;
    }

    // default precision
    if(!(flags & FLAGS_PRECISION))
    {
        prec = PRINTF_DEFAULT_FLOAT_PRECISION;
    }

    // determine the decimal exponent
    // based on the algorithm by David Gay (https://www.ampl.com/netlib/fp/dtoa.c)
    union
    {
        uint64_t U;
        double F;
    }conv;

    conv.F = value;
    int exp2 = (int)((conv.U >> 52U) & 0x07FFU) - 1023;      // effectively log2
    conv.U = (conv.U & ((1ULL << 52U) - 1U)) | (1023ULL << 52U); // drop the exponent so conv.F is now in [1,2)
    // now approximate log10 from the log2 integer part and an expansion of ln around 1.5
    int expval = (int)(0.1760912590558 + exp2 * 0.301029995663981
            + (conv.F - 1.5) * 0.289529654602168);
    // now we want to compute 10^expval but we want to be sure it won't overflow
    exp2 = (int)(expval * 3.321928094887362 + 0.5);
    const double z = expval * 2.302585092994046 - exp2 * 0.6931471805599453;
    const double z2 = z * z;
    conv.U = (uint64_t)(exp2 + 1023) << 52U;
    // compute exp(z) using continued fractions, see https://en.wikipedia.org/wiki/Exponential_function#Continued_fractions_for_ex
    conv.F *= 1 + 2 * z / (2 - z + (z2 / (6 + (z2 / (10 + z2 / 14)))));
    // correct for rounding errors
    if(value < conv.F)
    {
        expval--;
        conv.F /= 10;
    }

    // the exponent format is "%+03d" and largest value is "307", so set aside 4-5 characters
    unsigned int minwidth = ((expval < 100) && (expval > -100)) ? 4U : 5U;

    // in "%g" mode, "prec" is the number of *significant figures* not decimals
    if(flags & FLAGS_ADAPT_EXP)
    {
        // do we want to fall-back to "%f" mode?
        if((value >= 1e-4) && (value < 1e6))
        {
            if((int)prec > expval)
            {
                prec = (unsigned)((int)prec - expval - 1);
            }
            else
            {
                prec = 0;
            }
            flags |= FLAGS_PRECISION;   // make sure _ftoa respects precision
            // no characters in exponent
            minwidth = 0U;
            expval = 0;
        }
        else
        {
            // we use one sigfig for the whole part
            if((prec > 0) && (flags & FLAGS_PRECISION))
            {
                --prec;
            }
        }
    }

    // will everything fit?
    unsigned int fwidth = width;
    if(width > minwidth)
    {
        // we didn't fall-back so subtract the characters required for the exponent
        fwidth -= minwidth;
    }
    else
    {
        // not enough characters, so go back to default sizing
        fwidth = 0U;
    }
    if((flags & FLAGS_LEFT) && minwidth)
    {
        // if we're padding on the right, DON'T pad the floating part
        fwidth = 0U;
    }

    // rescale the float value
    if(expval)
    {
        value /= conv.F;
    }

    // output the floating part
    const size_t start_idx = idx;
    idx = _ftoa(out, buffer, idx, maxlen, negative ? -value : value, prec, fwidth, flags & ~FLAGS_ADAPT_EXP);

    // output the exponent part
    if(minwidth)
    {
        // output the exponential symbol
        out((flags & FLAGS_UPPERCASE) ? 'E' : 'e', buffer, idx++, maxlen);
        // output the exponent value
        idx = _ntoa_long(out, buffer, idx, maxlen, (expval < 0) ? -expval : expval, expval < 0, 10, 0, minwidth - 1, FLAGS_ZEROPAD | FLAGS_PLUS);
        // might need to right-pad spaces
        if(flags & FLAGS_LEFT)
        {
            while(idx - start_idx < width) out(' ', buffer, idx++, maxlen);
        }
    }
    return idx;
}

#endif  // PRINTF_SUPPORT_EXPONENTIAL
#endif  // PRINTF_SUPPORT_FLOAT

// internal vsnprintf
static int _vsnprintf(out_fct_type out, char* buffer, const size_t maxlen, const char* format, va_list va)
{
    unsigned int flags, width, precision, n;
    size_t idx = 0U;

    if(!buffer)
    {
        // use null output function
        out = _out_null;
    }

    while(*format)
    {
        // format specifier?  %[flags][width][.precision][length]
        if(*format != '%')
        {
            // no
            out(*format, buffer, idx++, maxlen);
            format++;
            continue;
        }
        else
        {
            // yes, evaluate it
            format++;
        }

        // evaluate flags
        flags = 0U;
        do
        {
            switch(*format)
            {
            case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
            case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
            case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
            case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
            case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
            default :                                   n = 0U; break;
            }
        }while(n);

        // evaluate width field
        width = 0U;
        if(_is_digit(*format))
        {
            width = _atoi(&format);
        }
        else if(*format == '*')
        {
            const int w = va_arg(va, int);
            if(w < 0)
            {
                flags |= FLAGS_LEFT;    // reverse padding
                width = (unsigned int)-w;
            }
            else
            {
                width = (unsigned int)w;
            }
            format++;
        }

        // evaluate precision field
        precision = 0U;
        if(*format == '.')
        {
            flags |= FLAGS_PRECISION;
            format++;
            if(_is_digit(*format))
            {
                precision = _atoi(&format);
            }
            else if(*format == '*')
            {
                const int prec = (int)va_arg(va, int);
                precision = prec > 0 ? (unsigned int)prec : 0U;
                format++;
            }
        }

        // evaluate length field
        switch(*format)
        {
        case 'l':
            flags |= FLAGS_LONG;
            format++;
            if(*format == 'l')
            {
                flags |= FLAGS_LONG_LONG;
                format++;
            }
            break;
        case 'h':
            flags |= FLAGS_SHORT;
            format++;
            if(*format == 'h')
            {
                flags |= FLAGS_CHAR;
                format++;
            }
            break;
        #if defined(PRINTF_SUPPORT_PTRDIFF_T)
        case 't':
            flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
            format++;
            break;
        #endif
        case 'j':
            flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
            format++;
            break;
        case 'z':
            flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
            format++;
            break;
        default:
            break;
        }

        // evaluate specifier
        switch(*format)
        {
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case 'X':
        case 'o':
        case 'b':
        {
            // set the base
            unsigned int base;
            if(*format == 'x' || *format == 'X')
            {
                base = 16U;
            }
            else if(*format == 'o')
            {
                base = 8U;
            }
            else if(*format == 'b')
            {
                base = 2U;
            }
            else
            {
                base = 10U;
                flags &= ~FLAGS_HASH;   // no hash for dec format
            }
            // uppercase
            if(*format == 'X')
            {
                flags |= FLAGS_UPPERCASE;
            }

            // no plus or space flag for u, x, X, o, b
            if((*format != 'i') && (*format != 'd'))
            {
                flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
            }

            // ignore '0' flag when precision is given
            if(flags & FLAGS_PRECISION)
            {
                flags &= ~FLAGS_ZEROPAD;
            }

            // convert the integer
            if((*format == 'i') || (*format == 'd'))
            {
                // signed
                if(flags & FLAGS_LONG_LONG)
                {
                    #if defined(PRINTF_SUPPORT_LONG_LONG)
                    const long long value = va_arg(va, long long);
                    idx = _ntoa_long_long(out, buffer, idx, maxlen, (unsigned long long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
                    #endif
                }
                else if(flags & FLAGS_LONG)
                {
                    const long value = va_arg(va, long);
                    idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
                }
                else
                {
                    const int value = (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int) : va_arg(va, int);
                    idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned int)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
                }
            }
            else
            {
                // unsigned
                if(flags & FLAGS_LONG_LONG)
                {
                    #if defined(PRINTF_SUPPORT_LONG_LONG)
                    idx = _ntoa_long_long(out, buffer, idx, maxlen, va_arg(va, unsigned long long), false, base, precision, width, flags);
                    #endif
                }
                else if (flags & FLAGS_LONG)
                {
                    idx = _ntoa_long(out, buffer, idx, maxlen, va_arg(va, unsigned long), false, base, precision, width, flags);
                }
                else
                {
                    const unsigned int value = (flags & FLAGS_CHAR) ? (unsigned char)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int) : va_arg(va, unsigned int);
                    idx = _ntoa_long(out, buffer, idx, maxlen, value, false, base, precision, width, flags);
                }
            }
            format++;
            break;
        }
        #if defined(PRINTF_SUPPORT_FLOAT)
        case 'f':
        case 'F':
            if(*format == 'F') flags |= FLAGS_UPPERCASE;
            idx = _ftoa(out, buffer, idx, maxlen, va_arg(va, double), precision, width, flags);
            format++;
            break;
        #if defined(PRINTF_SUPPORT_EXPONENTIAL)
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            if((*format == 'g') || (*format == 'G')) flags |= FLAGS_ADAPT_EXP;
            if((*format == 'E') || (*format == 'G')) flags |= FLAGS_UPPERCASE;
            idx = _etoa(out, buffer, idx, maxlen, va_arg(va, double), precision, width, flags);
            format++;
            break;
        #endif  // PRINTF_SUPPORT_EXPONENTIAL
        #endif  // PRINTF_SUPPORT_FLOAT
        case 'c':
        {
            unsigned int l = 1U;
            // pre padding
            if(!(flags & FLAGS_LEFT))
            {
                while(l++ < width)
                {
                    out(' ', buffer, idx++, maxlen);
                }
            }
            // char output
            out((char)va_arg(va, int), buffer, idx++, maxlen);
            // post padding
            if(flags & FLAGS_LEFT)
            {
                while(l++ < width)
                {
                    out(' ', buffer, idx++, maxlen);
                }
            }
            format++;
            break;
        }

        case 's':
        {
            const char* p = va_arg(va, char*);
            unsigned int l = _strnlen_s(p, precision ? precision : (size_t)-1);
            // pre padding
            if(flags & FLAGS_PRECISION)
            {
                l = (l < precision ? l : precision);
            }
            if(!(flags & FLAGS_LEFT))
            {
                while(l++ < width)
                {
                    out(' ', buffer, idx++, maxlen);
                }
            }
            // string output
            while((*p != 0) && (!(flags & FLAGS_PRECISION) || precision--))
            {
                out(*(p++), buffer, idx++, maxlen);
            }
            // post padding
            if(flags & FLAGS_LEFT)
            {
                while(l++ < width)
                {
                    out(' ', buffer, idx++, maxlen);
                }
            }
            format++;
            break;
        }

        case 'p':
        {
            width = sizeof(void*) * 2U;
            flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
            #if defined(PRINTF_SUPPORT_LONG_LONG)
            const bool is_ll = sizeof(uintptr_t) == sizeof(long long);
            if(is_ll)
            {
                idx = _ntoa_long_long(out, buffer, idx, maxlen, (uintptr_t)va_arg(va, void*), false, 16U, precision, width, flags);
            }
            else
            {
            #endif
                idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned long)((uintptr_t)va_arg(va, void*)), false, 16U, precision, width, flags);
            #if defined(PRINTF_SUPPORT_LONG_LONG)
            }
            #endif
            format++;
            break;
        }

        case '%':
            out('%', buffer, idx++, maxlen);
            format++;
            break;

        default:
            out(*format, buffer, idx++, maxlen);
            format++;
            break;
        }
    }

    // termination
    out((char)0, buffer, idx < maxlen ? idx : maxlen - 1U, maxlen);

    // return written chars without terminating \0
    return (int)idx;
}

///////////////////////////////////////////////////////////////////////////////

int printf(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    char buffer[1];
    const int ret = _vsnprintf(_out_char, buffer, (size_t)-1, format, va);
    va_end(va);
    return ret;
}

int sprintf(char* buffer, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    const int ret = _vsnprintf(_out_buffer, buffer, (size_t)-1, format, va);
    va_end(va);
    return ret;
}

int snprintf(char* buffer, size_t count, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    const int ret = _vsnprintf(_out_buffer, buffer, count, format, va);
    va_end(va);
    return ret;
}

int vprintf(const char* format, va_list va)
{
    char buffer[1];
    return _vsnprintf(_out_char, buffer, (size_t)-1, format, va);
}

int vsnprintf(char* buffer, size_t count, const char* format, va_list va)
{
    return _vsnprintf(_out_buffer, buffer, count, format, va);
}

int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    const out_fct_wrap_type out_fct_wrap = { out, arg };
    const int ret = _vsnprintf(_out_fct, (char*)(uintptr_t)&out_fct_wrap, (size_t)-1, format, va);
    va_end(va);
    return ret;
}

#endif

///////////////////////////////////////////////////////////////////////////////

#if USE_STDIO_SSCANF

int bk_sscanf(const char* buffer, const char* format, ...)
{
    #if 1
    va_list ap;
    va_start(ap, format);
    int len = sscanf(buffer, format, ap);
    va_end(ap);
    return len;
    #else
    return 0;
    #endif
}

#else

#define _BSCANF_CHECK(x)            if (!(x)) goto exit;
#define _BSCANF_CONSUME_WSPACE()    while(isspace(*buf_ptr)) {buf_ptr++;}
#define _BSCANF_MATCH()             _BSCANF_CHECK(*buf_ptr == *fmt_ptr);
#define _BSCANF_CHECK_NULL(ptr)     _BSCANF_CHECK(NULL != ptr);
#define _BSCANF_CHECK_STRING()      _BSCANF_CHECK(0 != max_width);
#define _BSCANF_CHECK_BUFFER()      _BSCANF_CHECK('\0' != *buffer);
#define _BSCANF_CHECK_STRTONUM()    _BSCANF_CHECK(buf_ptr != end_ptr);

int bk_sscanf(const char* buffer, const char* format, ...)
{
    /* Our return value. On a conversion error, we return this immediately. */
    int num_args_set = 0;
    /* We use these to index into our buffer and format string. */
    const char* buf_ptr = buffer;
    const char* fmt_ptr = format;
    /* Variadic arguments -- pointers in which we put our conversion results. */
    va_list args;
    long* long_ptr;
    int* int_ptr;
    short* short_ptr;
    unsigned long* ulong_ptr;
    unsigned short* ushort_ptr;
    unsigned int* uint_ptr;
    double* double_ptr;
    float* float_ptr;
    char* char_ptr;
    //wchar_t* wchar_ptr;
    /* These are useful variables when doing string to number conversion. */
    char* end_ptr;
    int base;
    /* These are flags that are used by different conversion specifiers. */
    int is_suppressed = 0;
    size_t max_width = 0;
    char length_mod = '\0';

    /* Return a special value when one of the arguments is NULL. */
    if(NULL == buffer || NULL == format) return -1;

    va_start(args, format);

    while('\0' != *fmt_ptr)
    {
        /* We ignore spaces before specifiers. */
        if(isspace(*fmt_ptr))
        {
            /* Any whitespace in the format consumes all of the whitespace in the buffer. */
            _BSCANF_CONSUME_WSPACE();
            fmt_ptr++;
            continue;
        }

        if('%' == *fmt_ptr)
        {
            /* Handle conversion specifier. */
            fmt_ptr++;

            /* Check for assignment-suppressing character. */
            if('*' == *fmt_ptr)
            {
                is_suppressed = 1;
                fmt_ptr++;
            }
            else
            {
                is_suppressed = 0;
            }

            /* Check for maximum field width. */
            if(isdigit(*fmt_ptr))
            {
                max_width = strtoul(fmt_ptr, &end_ptr, 0);
                /* Check if the sequence is a number > 0. */
                _BSCANF_CHECK(fmt_ptr != end_ptr);
                _BSCANF_CHECK(max_width > 0);
                fmt_ptr = end_ptr;
            }

            /* Check for a length modifier. */
            if('h' == *fmt_ptr || 'l' == *fmt_ptr)
            {
                length_mod = *fmt_ptr;
                fmt_ptr++;
            }
            else
            {
                length_mod = '\0';
            }

            /* Handle the conversion format specifier. */
            if('n' == *fmt_ptr)
            {
                /* 'n': number of characters read so far. */
                /* 'n' conversion specifiers DO NOT consume whitespace. */
                /* Technically undefined, but just stop here for safety. */
                _BSCANF_CHECK(!is_suppressed);

                if('l' == length_mod)
                {
                    long_ptr = va_arg(args, long*);
                    _BSCANF_CHECK_NULL(long_ptr);
                    *long_ptr = (long)(buf_ptr - buffer);
                }
                else if('h' == length_mod)
                {
                    short_ptr = va_arg(args, short*);
                    _BSCANF_CHECK_NULL(short_ptr);
                    *short_ptr = (short)(buf_ptr - buffer);
                }
                else
                {
                    int_ptr = va_arg(args, int*);
                    _BSCANF_CHECK_NULL(int_ptr);
                    *int_ptr = (int)(buf_ptr - buffer);
                }

                fmt_ptr++;
                num_args_set++;
                continue;
            }

            /* All other specifiers move the buffer pointer, so check that it's not NUL. */
            _BSCANF_CHECK_BUFFER();

            if('%' == *fmt_ptr)
            {
                /* '%': match literal %. */
                _BSCANF_CONSUME_WSPACE();
                _BSCANF_MATCH();
                buf_ptr++;
            }
            else if('c' == *fmt_ptr || 's' == *fmt_ptr)
            {
                /* 'c'/'s': match a character sequence/string. */
                /* String conversion requires a width. */
                _BSCANF_CHECK_STRING();

                /* 'c' conversion specifiers DO NOT consume whitespace. */
                if('c' != *fmt_ptr)
                {
                    _BSCANF_CONSUME_WSPACE();
                }

                if(is_suppressed)
                {
                    /* Consume the character (string) and ignore it in this case. */
                    for(; max_width > 0; max_width--)
                    {
                        buf_ptr++;

                        if(*buf_ptr == '\0' || (isspace(*buf_ptr) && 's' == *fmt_ptr))
                        {
                            break;
                        }
                    }

                    fmt_ptr++;
                    continue;
                }
                else if('l' == length_mod)
                {
                    va_arg(args, wchar_t*);
                    //wchar_ptr = va_arg(args, wchar_t*);
                    _BSCANF_CHECK_NULL(char_ptr);
                    /* TODO: Implementation. */
                    _BSCANF_CHECK(0);
                }
                else
                {
                    char_ptr = va_arg(args, char*);
                    _BSCANF_CHECK_NULL(char_ptr);

                    for(; max_width > 0; max_width--)
                    {
                        *char_ptr = *buf_ptr;

                        if(*buf_ptr == '\0' || (isspace(*buf_ptr) && 's' == *fmt_ptr))
                        {
                            break;
                        }

                        char_ptr++;
                        buf_ptr++;
                    }

                    /* Strings are null-terminated. */
                    if('s' == *fmt_ptr)
                    {
                        *char_ptr = '\0';
                    }

                    num_args_set++;
                }
            }
            else if('[' == *fmt_ptr)
            {
                /* TODO: '[': match a non-empty sequence of characters from a set. */
                _BSCANF_CHECK(0);
                /* String conversion requires a width. */
                _BSCANF_CHECK_STRING();
                /* '[' conversion specifiers DO NOT consume whitespace. */
            }
            else if('i' == *fmt_ptr || 'd' == *fmt_ptr)
            {
                /* 'i'/'d': match a integer/decimal integer. */
                _BSCANF_CONSUME_WSPACE();
                base = ('d' == *fmt_ptr) * 10;

                if(is_suppressed)
                {
                    /* Consume the integer and ignore it in this case. */
                    strtol(buf_ptr, &end_ptr, base);
                }
                else if('l' == length_mod)
                {
                    long_ptr = va_arg(args, long*);
                    _BSCANF_CHECK_NULL(long_ptr);
                    *long_ptr = (long) strtol(buf_ptr, &end_ptr, base);
                }
                else if('h' == length_mod)
                {
                    short_ptr = va_arg(args, short*);
                    _BSCANF_CHECK_NULL(short_ptr);
                    *short_ptr = (short)(strtol(buf_ptr, &end_ptr, base));
                }
                else
                {
                    int_ptr = va_arg(args, int*);
                    _BSCANF_CHECK_NULL(int_ptr);
                    *int_ptr = (int)(strtol(buf_ptr, &end_ptr, base));
                }

                _BSCANF_CHECK_STRTONUM();
                buf_ptr = end_ptr;
                num_args_set++;
            }
            else if('g' == *fmt_ptr || 'e' == *fmt_ptr || 'f' == *fmt_ptr ||
                    'G' == *fmt_ptr || 'E' == *fmt_ptr || 'F' == *fmt_ptr)
            {
                /* 'g'/'e'/'f': match a float in strtod form. */
                /* TODO: 'a': match a float in C99 binary floating-point form. */
                _BSCANF_CONSUME_WSPACE();

                if(is_suppressed)
                {
                    /* Consume the float and ignore it in this case. */
                    strtod((char*)buf_ptr, &end_ptr);
                }
                else if('l' == length_mod)
                {
                    double_ptr = va_arg(args, double*);
                    _BSCANF_CHECK_NULL(double_ptr);
                    *double_ptr = (double)(strtod((char*)buf_ptr, &end_ptr));
                }
                else
                {
                    float_ptr = va_arg(args, float*);
                    _BSCANF_CHECK_NULL(float_ptr);
                    *float_ptr = (float)(strtod((char*)buf_ptr, &end_ptr));
                }

                _BSCANF_CHECK_STRTONUM();
                buf_ptr = end_ptr;
                num_args_set++;
            }
            else if('u' == *fmt_ptr || 'o' == *fmt_ptr || 'x' == *fmt_ptr || 'X' == *fmt_ptr)
            {
                /* 'u'/'o'/'x': match a unsigned decimal/octal/hexadecimal integer */
                _BSCANF_CONSUME_WSPACE();

                base = ('u' == *fmt_ptr) * 10 + ('o' == *fmt_ptr) * 8 + ('x' == *fmt_ptr || 'X' == *fmt_ptr) * 16;

                if(is_suppressed)
                {
                    /* Consume the unsigned integer and ignore it in this case. */
                    strtoul(buf_ptr, &end_ptr, base);
                }
                else if('l' == length_mod)
                {
                    ulong_ptr = va_arg(args, unsigned long*);
                    _BSCANF_CHECK_NULL(ulong_ptr);
                    *ulong_ptr = (unsigned long) strtoul(buf_ptr, &end_ptr, base);
                }
                else if('h' == length_mod)
                {
                    ushort_ptr = va_arg(args, unsigned short*);
                    _BSCANF_CHECK_NULL(ushort_ptr);
                    *ushort_ptr = (unsigned short)(strtoul(buf_ptr, &end_ptr, base));
                }
                else
                {
                    uint_ptr = va_arg(args, unsigned int*);
                    _BSCANF_CHECK_NULL(uint_ptr);
                    *uint_ptr = (unsigned int)(strtoul(buf_ptr, &end_ptr, base));
                }

                _BSCANF_CHECK_STRTONUM();
                buf_ptr = end_ptr;
                num_args_set++;
            }
            else
            {
                /* Unknown conversion specifier. */
                _BSCANF_CHECK(0);
            }

            /* TODO: 'p': match a (implementation-defined) pointer. */
        }
        else
        {
            /* Match character with that in buffer. */
            _BSCANF_MATCH();
            buf_ptr++;
        }

        /* Get the next format specifier. */
        fmt_ptr++;
    }

exit:
    va_end(args);

    return num_args_set;
}

#endif
