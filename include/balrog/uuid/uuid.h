#pragma once

#include <stdint.h>

#define UUID_LEN        16  // the bytes on disk
#define UUID_TEXT_LEN   37  // 36 characters and the null byte

void uuid_to_str(const uint8_t* uuid, char* out);

int str_to_uuid(uint8_t* uuid, const char* in);