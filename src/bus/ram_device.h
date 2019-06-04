#pragma once

#include <glog/logging.h>
#include <memory>
#include <vector>

#include "cpu/cpu_65816.h"

class RAMDevice : public SystemBusDevice {
public:
  RAMDevice(const Address &base_address, const Address &end_address);
  ~RAMDevice() override;

  // SystemBusDevice implementation
  std::vector<MemoryRegion> GetMemoryRegions() override;
  void StoreByte(const Address &addr, uint8_t v, uint8_t **address) override;
  uint8_t ReadByte(const Address &addr, uint8_t **address) override;
  bool DecodeAddress(const Address &from_addr, Address &to_addr) override;

  size_t Size() const;

  SystemBusDevice::MemoryRegion region() const;

private:
  SystemBusDevice::MemoryRegion region_;
};
