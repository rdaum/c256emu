#include "bus/register_utils.h"

#include <glog/logging.h>

bool ReadRegister(uint32_t addr,
                  const std::vector<Reg>& registers,
                  uint8_t* v) {
  for (const auto& reg : registers) {
    if (addr >= reg.reg_addr && addr < reg.reg_addr + reg.byte_width) {
      ptrdiff_t o = addr - reg.reg_addr;
      CHECK(o < 4);
      uint8_t* r_ptr = static_cast<uint8_t*>(reg.reg_ptr) + o;
      *v = *r_ptr;
      return true;
    }
  }
  return false;
}

bool StoreRegister(uint32_t addr,
                   uint8_t v,
                   const std::vector<Reg>& registers) {
  for (const auto& reg : registers) {
    if (addr >= reg.reg_addr && addr < reg.reg_addr + reg.byte_width) {
      ptrdiff_t o = addr - reg.reg_addr;
      CHECK(o < 4);
      uint8_t* r_ptr = static_cast<uint8_t*>(reg.reg_ptr) + o;
      *r_ptr = v;
      return true;
    }
  }
  return false;
}
