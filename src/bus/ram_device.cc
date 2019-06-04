#include "bus/ram_device.h"

RAMDevice::RAMDevice(const Address &base_address, const Address &end_address)
    : region_(SystemBusDevice::MemoryRegion{
          base_address.AsInt(), end_address.AsInt(),
          new uint8_t[Address::Size(base_address, end_address)]}) {
  bzero(region_.mem, region_.end_address - region_.start_address);
}

void RAMDevice::StoreByte(const Address &addr, uint8_t v, uint8_t **address) {
  // This should probably never be called.
  CHECK(false);

  // If we ever need to do it, we can do this...
  uint32_t offset = addr.AsInt() - region_.start_address;
  if (address)
    *address = &region_.mem[offset];
  region_.mem[offset] = v;
}

uint8_t RAMDevice::ReadByte(const Address &addr, uint8_t **address) {
  // This should probably never be called.
  CHECK(false);
  // If we ever need to do it, we can do this...
  int32_t offset = addr.AsInt() - region_.start_address;
  if (address)
    *address = &region_.mem[offset];
  return region_.mem[offset];
}

bool RAMDevice::DecodeAddress(const Address &from_addr, Address &to_addr) {
  return false;
//  if (from_addr.InRange(Address(region_.start_address),
//                        Address(region_.end_address))) {
//    to_addr = from_addr;
//    return true;
//  }
//  return false;
}

RAMDevice::~RAMDevice() { delete[] region_.mem; }

size_t RAMDevice::Size() const {
  return region_.end_address - region_.start_address;
}

std::vector<SystemBusDevice::MemoryRegion> RAMDevice::GetMemoryRegions() {
  return {region_};
}

SystemBusDevice::MemoryRegion RAMDevice::region() const { return region_; }
