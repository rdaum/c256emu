#pragma once

#include <glog/logging.h>

#include <string>

#include "cpu.h"

class Loader {
public:
  explicit Loader(SystemBus *system_bus) : system_bus_(system_bus) {}

  void LoadFromHex(const std::string &filename);
  void LoadFromBin(const std::string &filename, uint32_t base_address);
  void LoadFromO65(const std::string &filename, uint32_t reloc_address,
                   bool verbose = true);

private:
  SystemBus *system_bus_;
};