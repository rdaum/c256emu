#pragma once

#include <string>

#include "cpu/system_bus_device.h"

class SystemBus;

extern void LoadFromHex(const std::string &filename, SystemBus *system_bus);
extern void LoadFromBin(const std::string &filename,
                        const Address &base_address, SystemBus *system_bus);
