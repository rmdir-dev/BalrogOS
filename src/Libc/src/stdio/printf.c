#include "stdio.h"
#include "string.h"
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include "Kernel/IO/tty/tty_io.h"

static size_t int_to_string(int i, uint8_t base, char* str)
{
    size_t size = 0;
    size_t pushed = 0;
    char buffer[10];
    char chars[17] = "0123456789abcdef";

    if(i < 0) 
    {
        str[0] = '-';
        size++;
        i = -i;
    }

    if(i == 0)
    {
        str[0] = '0';
        size++;
    } else 
    {
        while(i != 0)
        {
            buffer[pushed] = chars[i % base];
            pushed++;
            i /= base;
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
    terminal_write(str, size);
    return 1;
}

static int print_data(const char* str, size_t size, size_t maxsize)
{
    if(maxsize < size)
    {
        return 0;
    }
    print_string(str, size);
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
                    int nbr = va_arg(parameters, int);
                    putchar('b');
                    char str[128];
                    length = int_to_string(nbr, 2, str);
                    print_data(str, length, maxsize);
                    index++;
                }
                break;
            case 'd':
                {
                    int nbr = va_arg(parameters, int);
                    char str[128];
                    length = int_to_string(nbr, 10, str);
                    print_data(str, length, maxsize);
                    index++;
                }
                break;
            case 'x':
                {
                    int nbr = va_arg(parameters, int);
                    putchar('x');
                    char str[128];
                    length = int_to_string(nbr, 16, str);
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