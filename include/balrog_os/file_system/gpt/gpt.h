#pragma once

#include "balrog_os/file_system/fs_devices.h"
#include "balrog_os/file_system/gpt/gpt_guid.h"
#include "balrog_os/file_system/gpt/gpt_struct.h"

int gpt_init(fs_device_t *dev);

int gpt_find_by_index(fs_device_t *dev,  uint32_t index);

int gpt_find_by_guid(fs_device_t *dev,  const uint8_t* guid);