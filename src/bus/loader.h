#pragma once

#include <glog/logging.h>

#include <string>

#include "cpu.h"

class Loader {
public:
  explicit Loader(SystemBus *system_bus) : system_bus_(system_bus) {}

  bool LoadFromHex(const std::string &filename);
  bool LoadFromBin(const std::string &filename, uint32_t base_address);
  bool LoadFromO65(const std::string &filename, uint32_t reloc_address,
                   bool verbose = true);

private:
  SystemBus *system_bus_;
};