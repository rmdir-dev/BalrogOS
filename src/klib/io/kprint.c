#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include "balrog_os/drivers/screen/vga_driver.h"
#include "klib/io/kprint.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/debug/klog.h"
#include "balrog_os/drivers/serial/serial.h"
#include "balrog_os/memory/kheap.h"

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

int __print_string(const char* str, size_t size, enum klog_logging_level log_level)
{
    if (log_level == KDB_NONE || log_level >= debug_get_mode())
    {
        vga_write(str, size);
    }

    klog_write(log_level, str, size);
    return 1;
}

static int __print_data(const char* str, size_t size, size_t maxsize, enum klog_logging_level log_level)
{
    if(maxsize < size)
    {
        return 0;
    }
    return __print_string(str, size, log_level);
}

static size_t __sprint_data(char* out, size_t pos, const char* str, size_t size, size_t maxsize)
{
    for(size_t i = 0; i < size && pos + 1 < maxsize; i++)
    {
        out[pos++] = str[i];
    }

    return pos;
}

int __ksprint(char* out, size_t maxsize, const char* format, va_list parameters)
{
    size_t pos = 0;
    size_t index = 0;
    size_t base_index = 0;

    if (maxsize == 0)
    {
        return 0;
    }

    while(format[index] != 0)
    {
        size_t length = 0;

        if(format[index] != '%')
        {
            while(format[index] && format[index] != '%')
            {
                index++;
                length++;
            }

            pos = __sprint_data(out, pos, &format[base_index], length, maxsize);
        } else
        {
            char tmp_str[128];
            index++;

            switch (format[index])
            {
            case 'b':
                {
                    long nbr = va_arg(parameters, long);
                    pos = __sprint_data(out, pos, "b", 1, maxsize);
                    length = __int_to_string(nbr, 2, tmp_str, 0);
                    pos = __sprint_data(out, pos, tmp_str, length, maxsize);
                    index++;
                }
                break;
            case 'd':
                {
                    int nbr = va_arg(parameters, int);
                    length = __int_to_string(nbr, 10, tmp_str, 1);
                    pos = __sprint_data(out, pos, tmp_str, length, maxsize);
                    index++;
                }
                break;
            case 'l':
                {
                    long nbr = va_arg(parameters, long);
                    length = __int_to_string(nbr, 10, tmp_str, 1);
                    pos = __sprint_data(out, pos, tmp_str, length, maxsize);
                    index++;
                }
                break;
            case 'u':
                {
                    long nbr = va_arg(parameters, unsigned long);
                    length = __int_to_string(nbr, 10, tmp_str, 0);
                    pos = __sprint_data(out, pos, tmp_str, length, maxsize);
                    index++;
                }
                break;
            case 'x': case 'p':
                {
                    unsigned long nbr = va_arg(parameters, unsigned long);
                    pos = __sprint_data(out, pos, "x", 1, maxsize);
                    length = __int_to_string(nbr, 16, tmp_str, 0);
                    pos = __sprint_data(out, pos, tmp_str, length, maxsize);
                    index++;
                }
                break;

            case 'c':
                char c = (char) va_arg(parameters, int);
                pos = __sprint_data(out, pos, &c, 1, maxsize);
                index++;
                break;
            case 's':
                {
                    const char* arg = va_arg(parameters, const char*);
                    pos = __sprint_data(out, pos, arg, strlen(arg), maxsize);
                    index++;
                }
                break;

            default:
                pos = __sprint_data(out, pos, "%", 1, maxsize);
                break;
            }
        }
        base_index = index;
    }

    out[pos] = 0;

    return pos;
}

int ksprint(char* out, size_t maxsize, const char* format, ...)
{
    va_list parameters;
    va_start(parameters, format);
    int size = __ksprint(out, maxsize, format, parameters);
    va_end(parameters);

    return size;
}

int kprint(const char* __restrict format, ...)
{
    va_list parameters;
    va_start(parameters, format);
    char str[512];
    int size = __ksprint(str, 512, format, parameters);
    __print_string(str, size, KDB_NONE);
    va_end(parameters);

    return size;
}