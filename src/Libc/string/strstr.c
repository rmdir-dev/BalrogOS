#include <string.h>

char* strstr(char* fullString, char* substring)
{
    size_t fullLength = strlen(fullString);
    size_t subLength = strlen(substring);

    if(fullLength < subLength)
    {
        return NULL;
    }

    for(size_t i = 0; i < fullLength; i++)
    {
        if(fullString[i] == substring[0])
        {
            for(size_t j = 0; j < subLength; j++)
            {
                if(fullString[i + j] != substring[j])
                {
                    break;
                }
                if(j == subLength - 1)
                {
                    return &fullString[i];
                }
            }
        }
    }

    return NULL;
}