#pragma once

#include <string>
#include <glog/logging.h>

#include "cpu.h"

extern void LoadFromHex(const std::string &filename, SystemBus *system_bus);
extern void LoadFromBin(const std::string &filename,
                        uint32_t base_address, SystemBus *system_bus);
