#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include "BalrogOS/Drivers/Screen/vga_driver.h"

static size_t int_to_string(unsigned long val, uint8_t base, char* str, uint8_t isSigned)
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

static int print_pointer(void* ptr)
{
    //TODO
}

static int print_string(const char* str, size_t size)
{
    vga_write(str, size);
    return 1;
}

static int print_data(const char* str, size_t size, size_t maxsize)
{
    if(maxsize < size)
    {
        return 0;
    }
    return print_string(str, size);
}

int printf(const char* __restrict format, ...)
{
    va_list parameters;
    va_start(parameters, format);

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
            print_data(&format[base_index], length, maxsize);
        } else 
        {
            index++;

            switch (format[index])
            {
            case 'b':
                {
                    long nbr = va_arg(parameters, long);
                    putchar('b');
                    char str[128];
                    length = int_to_string(nbr, 2, str, 0);
                    print_data(str, length, maxsize);
                    index++;
                }
                break;
            case 'd':
                {
                    long nbr = va_arg(parameters, long);
                    char str[128];
                    length = int_to_string(nbr, 10, str, 1);
                    print_data(str, length, maxsize);
                    index++;
                }
                break;
            case 'u':
                {
                    long nbr = va_arg(parameters, unsigned long);
                    char str[128];
                    length = int_to_string(nbr, 10, str, 0);
                    print_data(str, length, maxsize);
                    index++;
                }
                break;
            case 'x': case 'p':
                {
                    unsigned long nbr = va_arg(parameters, unsigned long);
                    putchar('x');
                    char str[128];
                    length = int_to_string(nbr, 16, str, 0);
                    print_data(str, length, maxsize);
                    index++;
                }
                break;

            case 'c':
                length = 1;
                char c = (char) va_arg(parameters, int);
                print_data(&c, 1, maxsize);
                index++;
                break;
            case 's':
                {
                    const char* str = va_arg(parameters, const char*);
                    length = strlen(str);
                    print_data(str, length, maxsize);
                    index++;
                }
                break;
            
            default:
                length = 1;
                print_string("%", length);
                //index--;
                break;
            }
        }
        base_index = index;
        written += length;
    }

    va_end(parameters);
    return written;
}