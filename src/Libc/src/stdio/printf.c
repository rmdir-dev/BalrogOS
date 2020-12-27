#include "stdio.h"
#include "string.h"
#include <stdarg.h>
#include <limits.h>
#include "Kernel/IO/Terminal/terminal_io.h"

static size_t int_to_string(int i, char* str)
{
    size_t size = 0;
    size_t pushed = 0;
    char buffer[10];

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
        int to_char;
        while(i != 0)
        {
            to_char = i % 10;
            buffer[pushed] = '0' + to_char;
            pushed++;
            i /= 10;
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

static int print_string(const char* str, size_t size)
{
    terminal_write(str, size);
    return 1;
}

int printf(const char* __restrict format, ...)
{
    va_list parameters;
    va_start(parameters, format);

    int written = 0;

    size_t index = 0;

    while(format[index])
    {
        size_t maxsize = INT_MAX - written;

        if(format[index] == '%')
        {
            //go to next character (check what we want to print)
            index++;

            switch (format[index])
            {
                //Character
            case 'c':
                {
                    char c = (char) va_arg(parameters, int);
                    if(!maxsize)
                    {
                        //TODO error
                        return -1;
                    }
                    if(!print_string(&c, 1))
                    {
                        //TODO error
                        return -1;
                    }
                    written++;
                }
                break;

                //String
            case 's':
                {
                    const char* str = va_arg(parameters, const char*);
                    size_t length = strlen(str);
                    if(maxsize < length)
                    {
                        //TODO error
                        return -1;
                    }
                    if(!print_string(str, length))
                    {
                        //TODO error
                        return -1;
                    }
                    written += length;
                }
                break;

                //digit
            case 'd':
                {
                    int i = va_arg(parameters, int);
                    char str[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; 
                    size_t length = int_to_string(i, str);
                    if(maxsize < length)
                    {
                        //TODO error
                        return -1;
                    }
                    if(!print_string(str, length))
                    {
                        //TODO error
                        return -1;
                    }
                    written += length;
                }
                break;

                //float
            case 'f':
                /* TODO */
                break;
            
            default:
                break;
            }
            index++;
        } else 
        {
            size_t begining_index = index;
            const char* str = &format[begining_index];

            while(format[index] != '%' && format[index] != 0)
            {
                index++;
                if(maxsize < (index - begining_index))
                {
                    return -1;
                }
            }

            if(!print_string(str, index - begining_index))
            {
                return -1;
            }
        }
    }

    va_end(parameters);
    return written;
}