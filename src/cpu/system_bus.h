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

#ifndef __SYSTEMBUS__
#define __SYSTEMBUS__

#include <stdint.h>

#include <set>
#include <vector>

#include "cpu/system_bus_device.h"

class SystemBus {
public:
  virtual ~SystemBus() = default;

  void RegisterDevice(SystemBusDevice *device);
  void StoreByte(const Address &addr, uint8_t v);
  void StoreWord(const Address &addr, uint16_t v);
  void StoreLong(const Address &addr, uint32_t v);
  uint8_t ReadByte(const Address &addr);
  uint16_t ReadWord(const Address &addr);
  uint32_t ReadLong(const Address &addr);
  Address ReadAddressAt(const Address &addr);

private:
  bool MemoryRegionforAddress(uint32_t address,
                              SystemBusDevice::MemoryRegion *region) const;
  SystemBusDevice *DeviceForAddress(const Address &, Address &) const;

  std::vector<SystemBusDevice::MemoryRegion> memory_regions_;
  std::vector<SystemBusDevice *> devices_;
};

#endif
