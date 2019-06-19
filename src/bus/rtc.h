#pragma once

#include <glog/logging.h>
#include <stdint.h>

class Rtc {
 public:
  void StoreByte(uint32_t addr, uint8_t v);

  uint8_t ReadByte(uint32_t addr);
};