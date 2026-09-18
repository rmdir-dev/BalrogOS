#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include "balrog_os/drivers/screen/vga_driver.h"
#include "klib/io/kprint.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/debug/klog.h"
#include "balrog_os/drivers/serial/serial.h"

static size_t __int_to_string(unsigned long val, uint8_t base, char* str, uint8_t isSigned)
{
    size_t size = 0;
    size_t pushed = 0;
    char buffer[128];
    char chars[17] = "0123456789abcdef";

    if(isSigned && val > INT_MAX) 
    {
        str[0] = '-';
        size++;
        /*
        Convert unsigned int to the equivalent int value.
        */
        val = -((int)val);
    }

    if(val == 0)
    {
        str[0] = '0';
        size++;
    } else 
    {
        while(val != 0)
        {
            buffer[pushed] = chars[val % base];
            pushed++;
            val /= base;
        }
    }

    for(size_t i = pushed; i > 0; i--)
    {
        str[size] = buffer[i - 1];
        size++;
    }
    
    //zero ended
    str[size] = 0;

    return size;
}

static int __print_string(const char* str, size_t size, int debug_only)
{
    if (debug_only == 0)
    {
        vga_write(str, size);
    }

    klog_write(str, size);
    return 1;
}

static int __print_data(const char* str, size_t size, size_t maxsize, int debug_only)
{
    if(maxsize < size)
    {
        return 0;
    }
    return __print_string(str, size, debug_only);
}

int __kernel_print(const char* format, va_list parameters, int debug_only)
{
    int written = 0;

    size_t index = 0;
    size_t base_index = 0;

    while(format[index] != 0)
    {
        size_t maxsize = INT_MAX - written;
        size_t length = 0;

        if(format[index] != '%')
        {
            while(format[index] && format[index] != '%')
            {
                index++;
                length++;
            }
            __print_data(&format[base_index], length, maxsize, debug_only);
        } else
        {
            index++;

            switch (format[index])
            {
            case 'b':
                {
                    long nbr = va_arg(parameters, long);
                    __print_string("b", 1, debug_only);
                    char str[128];
                    length = __int_to_string(nbr, 2, str, 0);
                    __print_data(str, length, maxsize, debug_only);
                    index++;
                }
                break;
            case 'd':
                {
                    long nbr = va_arg(parameters, long);
                    char str[128];
                    length = __int_to_string(nbr, 10, str, 1);
                    __print_data(str, length, maxsize, debug_only);
                    index++;
                }
                break;
            case 'u':
                {
                    long nbr = va_arg(parameters, unsigned long);
                    char str[128];
                    length = __int_to_string(nbr, 10, str, 0);
                    __print_data(str, length, maxsize, debug_only);
                    index++;
                }
                break;
            case 'x': case 'p':
                {
                    unsigned long nbr = va_arg(parameters, unsigned long);
                    __print_string("x", 1, debug_only);
                    char str[128];
                    length = __int_to_string(nbr, 16, str, 0);
                    __print_data(str, length, maxsize, debug_only);
                    index++;
                }
                break;

            case 'c':
                length = 1;
                char c = (char) va_arg(parameters, int);
                __print_data(&c, 1, maxsize, debug_only);
                index++;
                break;
            case 's':
                {
                    const char* str = va_arg(parameters, const char*);
                    length = strlen(str);
                    __print_data(str, length, maxsize, debug_only);
                    index++;
                }
                break;

            default:
                length = 1;
                __print_string("%", length, debug_only);
                break;
            }
        }
        base_index = index;
        written += length;
    }

    return written;
}

int kdbprint(const char* __restrict format, ...)
{
    va_list parameters;
    va_start(parameters, format);
    __kernel_print(format, parameters, 1);
    va_end(parameters);

    // Must return 0, else it breaks debug_output.h macros !
    return 0;
}

int kprint(const char* __restrict format, ...)
{
    va_list parameters;
    va_start(parameters, format);

    int ret = __kernel_print(format, parameters, 0);
    va_end(parameters);

    return ret;
}