#pragma once

#include <cstdint>
#include <vector>

struct Reg {
  uint32_t reg_addr;
  void* reg_ptr;
  uint8_t byte_width;
};

bool ReadRegister(uint32_t addr, const std::vector<Reg>& registers, uint8_t* v);

bool StoreRegister(uint32_t addr, uint8_t v, const std::vector<Reg>& registers);
