/*
 * This file is part of the 65816 Emulator Library.
 * Copyright (c) 2018 Francesco Rigoni.
 *
 * https://github.com/FrancescoRigoni/Lib65816
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "cpu/system_bus.h"

#include <algorithm>
#include <cmath>

void SystemBus::RegisterDevice(SystemBusDevice *device) {
  devices_.push_back({device});
  std::vector<SystemBusDevice::MemoryRegion> handled_regions =
      device->GetMemoryRegions();
  std::copy(handled_regions.begin(), handled_regions.end(),
            std::inserter(memory_regions_, memory_regions_.end()));
}

void SystemBus::StoreByte(const Address &addr, uint8_t v) {
  SystemBusDevice::MemoryRegion mem_region;
  const uint32_t address = addr.AsInt();
  if (MemoryRegionforAddress(address, &mem_region)) {
    uint32_t offset = address - mem_region.start_address;
    mem_region.mem[offset] = v;
    return;
  }

  Address decodedAddress;
  SystemBusDevice *device = DeviceForAddress(addr, decodedAddress);
  if (device) {
    device->StoreByte(decodedAddress, v);
    return;
  }
}

void SystemBus::StoreWord(const Address &addr, uint16_t v) {
  SystemBusDevice::MemoryRegion mem_region;
  const uint32_t address = addr.AsInt();
  if (MemoryRegionforAddress(address, &mem_region)) {
    uint32_t offset = address - mem_region.start_address;
    *(uint16_t *)(&mem_region.mem[offset]) = v;
    return;
  }

  Address decoded_address;
  SystemBusDevice *device = DeviceForAddress(addr, decoded_address);
  if (device) {
    uint8_t *physical_address = nullptr;
    uint8_t least_significant_byte = (uint8_t)(v & 0xFF);
    uint8_t most_significant_byte = (uint8_t)((v & 0xFF00) >> 8);
    device->StoreByte(decoded_address, least_significant_byte,
                      &physical_address);
    if (physical_address) {
      physical_address[1] = most_significant_byte;
      return;
    }
    device->StoreByte(decoded_address.WithOffset(1), most_significant_byte,
                      &physical_address);
    return;
  }
}

void SystemBus::StoreLong(const Address &addr, uint32_t v) {
  SystemBusDevice::MemoryRegion mem_region;
  const uint32_t address = addr.AsInt();
  if (MemoryRegionforAddress(address, &mem_region)) {
    uint32_t offset = address - mem_region.start_address;
    *(uint32_t *)(&mem_region.mem[offset]) = v;
    return;
  }

  Address decoded_address;
  SystemBusDevice *device = DeviceForAddress(addr, decoded_address);
  if (device) {
    uint8_t bytes[]{
        (uint8_t)(v & 0x000000ff),
        (uint8_t)((v & 0x0000ff00) >> 8),
        (uint8_t)((v & 0x00ff0000) >> 16),
        (uint8_t)((v & 0xff000000) >> 24),
    };
    // TODO optimize with &address
    device->StoreByte(decoded_address, bytes[0]);
    device->StoreByte(decoded_address.WithOffset(1), bytes[1]);
    device->StoreByte(decoded_address.WithOffset(2), bytes[2]);
    device->StoreByte(decoded_address.WithOffset(3), bytes[3]);
  }
}

uint8_t SystemBus::ReadByte(const Address &addr) {
  SystemBusDevice::MemoryRegion mem_region;
  const uint32_t address = addr.AsInt();
  if (MemoryRegionforAddress(address, &mem_region)) {
    uint32_t offset = address - mem_region.start_address;
    return mem_region.mem[offset];
  }

  Address decoded_address;
  SystemBusDevice *device = DeviceForAddress(addr, decoded_address);
  if (device) {
    return device->ReadByte(decoded_address);
  }
  return 0;
}

uint16_t SystemBus::ReadWord(const Address &addr) {
  SystemBusDevice::MemoryRegion mem_region;
  const uint32_t address = addr.AsInt();
  if (MemoryRegionforAddress(address, &mem_region)) {
    uint32_t offset = address - mem_region.start_address;
    return *(uint16_t *)(&mem_region.mem[offset]);
  }

  Address decoded_address;
  SystemBusDevice *device = DeviceForAddress(addr, decoded_address);
  if (device) {
    uint8_t *physical_address = nullptr;
    uint8_t least_significant_byte =
        device->ReadByte(decoded_address, &physical_address);
    uint8_t most_significant_byte;

    if (physical_address) {
      most_significant_byte = physical_address[1];
    } else {
      most_significant_byte = device->ReadByte(decoded_address.WithOffset(1));
    }
    uint16_t value =
        ((uint16_t)most_significant_byte << 8) | least_significant_byte;
    return value;
  }
  return 0;
}

uint32_t SystemBus::ReadLong(const Address &addr) {
  SystemBusDevice::MemoryRegion mem_region;
  const uint32_t address = addr.AsInt();
  if (MemoryRegionforAddress(address, &mem_region)) {
    uint32_t offset = address - mem_region.start_address;
    return *(uint32_t *)(&mem_region.mem[offset]);
  }

  Address decoded_address;
  SystemBusDevice *device = DeviceForAddress(addr, decoded_address);
  if (device) {
    uint8_t bytes[]{
        device->ReadByte(decoded_address),
        device->ReadByte(decoded_address.WithOffset(1)),
        device->ReadByte(decoded_address.WithOffset(2)),
        device->ReadByte(decoded_address.WithOffset(3)),
    };
    return bytes[0] | bytes[1] << 8 | bytes[2] << 16 | bytes[3] << 24;
  }
  return 0;
}

Address SystemBus::ReadAddressAt(const Address &addr) {
  SystemBusDevice::MemoryRegion mem_region;
  uint32_t address = addr.AsInt();
  if (MemoryRegionforAddress(address, &mem_region)) {
    uint32_t offset = address - mem_region.start_address;
    struct addr_store {
      uint32_t value : 24;
    };
    addr_store *addr24 =
        reinterpret_cast<struct addr_store *>(&mem_region.mem[offset]);
    return Address(addr24->value);
  }

  Address decoded_address{0x00, 0x0000};
  SystemBusDevice *device = DeviceForAddress(addr, decoded_address);
  if (device) {
    uint8_t *physical_address;
    // Read offset
    uint8_t least_significant_byte =
        device->ReadByte(decoded_address, &physical_address);
    uint8_t most_significant_byte;
    uint8_t bank;

    if (physical_address) {
      most_significant_byte = physical_address[1];
      bank = physical_address[2];
    } else {
      most_significant_byte = device->ReadByte(decoded_address.WithOffset(1));
      // Read bank
      bank = device->ReadByte(decoded_address.WithOffset(2));
    }
    uint16_t offset =
        (uint16_t)most_significant_byte << 8 | least_significant_byte;
    return Address(bank, offset);
  }
  return decoded_address;
}

SystemBusDevice *SystemBus::DeviceForAddress(const Address &address,
                                             Address &decoded_address) const {
  for (SystemBusDevice *device : devices_) {
    if (device->DecodeAddress(address, decoded_address)) {
      return device;
    }
  }
  return nullptr;
}

bool SystemBus::MemoryRegionforAddress(
    uint32_t address, SystemBusDevice::MemoryRegion *region) const {
  // Currently O(N), so lots of regions could be non-optimal. Currently not
  // worth binary type search.
  for (const auto &r : memory_regions_) {
    if (address >= r.start_address && address <= r.end_address) {
      *region = r;
      return true;
    }
  }
  return false;
}
