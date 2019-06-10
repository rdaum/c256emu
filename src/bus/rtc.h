#pragma once

#include <stdint.h>
#include <glog/logging.h>

class Rtc {
 public:
  void StoreByte(uint32_t addr, uint8_t v);

  uint8_t ReadByte(uint32_t addr);
};