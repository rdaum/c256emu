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

#ifndef SYSBUS_DEVICE_H
#define SYSBUS_DEVICE_H

#include <cstdint>
#include <iostream>
#include <vector>

#define BANK_SIZE_BYTES 0x10000
#define HALF_BANK_SIZE_BYTES 0x8000
#define PAGE_SIZE_BYTES 256

class Address {
public:
  Address() = default;
  explicit Address(uint32_t addr);
  constexpr Address(uint8_t bank, uint16_t offset)
      : bank_(bank), offset_(offset){};

  Address &operator=(const Address &o);
  bool operator==(const Address &o) const;
  bool operator<(const Address &o) const;

  static size_t Size(const Address &begin, const Address &end);
  uint32_t AsInt() const;

  static bool OffsetsAreOnDifferentPages(uint16_t offset1, uint16_t offset2);
  static Address SumOffsetToAddress(const Address &addr, int16_t offset2);
  static Address SumOffsetToAddressNoWrapAround(const Address &addr,
                                                int16_t offset);
  static Address SumOffsetToAddressWrapAround(const Address &addr,
                                              int16_t offset);

  bool InRange(const Address &start, const Address &end) const;

  Address WithOffset(int16_t offset) const;
  Address WithOffsetNoWrapAround(int16_t offset) const;
  Address WithOffsetWrapAround(int16_t offset) const;

  void GetBankAndOffset(uint8_t *bank, uint16_t *offset) {
    *bank = bank_;
    *offset = offset_;
  }

  uint8_t bank_;
  uint16_t offset_;
};

std::ostream &operator<<(std::ostream &out, const Address &addr);

template<typename T>
Address operator-(const Address &a, const T i) {
  static_assert(std::is_integral<T>::value, "Integral required");
  return a.WithOffset(-i);
}

template<typename T>
Address operator+(const Address &a, const T i) {
  static_assert(std::is_integral<T>::value, "Integral required");
  return a.WithOffset(i);
}

/**
 Every device (PPU, APU, ...) implements this interface.
 */
class SystemBusDevice {
public:
  struct MemoryRegion {
    uint32_t start_address;
    uint32_t end_address;
    uint8_t *mem;
  };

  virtual ~SystemBusDevice() = default;

  /**
   * Return a vector of all the direct-memory regions this device has, to
   * optimize direct reads and writes.
   */
  virtual std::vector<MemoryRegion> GetMemoryRegions() { return {}; };

  /**
    Stores one byte to the real address represented by the specified virtual
    address. That is: maps the virtual address to the real one and stores one
    byte in it.
    If the device is capable of returning a stable pointer that can be cached
    and re-used, 'address' will be it the address of the byte stored.
   */
  virtual void StoreByte(const Address &, uint8_t, uint8_t **address = 0) = 0;

  /**
    Reads one byte from the real address represented by the specified virtual
    address. That is: maps the virtual address to the real one and reads from
    it.
    If the device is capable of returning a stable pointer that can be cached
    and re-used, 'address' will be it the address of the byte read;
   */
  virtual uint8_t ReadByte(const Address &, uint8_t **address = 0) = 0;

  /**
    Returns true if the address was decoded successfully by this device.
   */
  virtual bool DecodeAddress(const Address &, Address &) = 0;
};

#endif // SYSBUS_DEVICE_H
