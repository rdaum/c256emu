#pragma once

#include "cpu/cpu_65816.h"

class Rtc : public SystemBusDevice {
 public:
  void StoreByte(const Address& addr, uint8_t v, uint8_t** address) override;

  uint8_t ReadByte(const Address& addr, uint8_t** address) override;

  bool DecodeAddress(const Address& from_addr, Address& to_addr) override;
};