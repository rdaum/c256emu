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

#include "cpu/system_bus_device.h"

#include <cmath>
#include <iomanip>

Address::Address(uint32_t addr) : bank_(addr >> 16), offset_(addr & 0x00ffff) {}

bool Address::operator<(const Address &o) const { return AsInt() < o.AsInt(); }

bool Address::operator==(const Address &o) const {
  return o.bank_ == bank_ && o.offset_ == offset_;
}

Address &Address::operator=(const Address &o) {
  bank_ = o.bank_;
  offset_ = o.offset_;
  return *this;
}

// static
Address Address::SumOffsetToAddressNoWrapAround(const Address &addr,
                                                int16_t offset) {
  uint8_t newBank = addr.bank_;
  uint16_t newOffset;
  auto offsetSum = (uint32_t)(addr.offset_ + offset);
  if (offsetSum >= BANK_SIZE_BYTES) {
    ++newBank;
    newOffset = static_cast<uint16_t>(offsetSum - BANK_SIZE_BYTES);
  } else {
    newOffset = addr.offset_ + offset;
  }
  return {newBank, newOffset};
}

Address Address::WithOffset(int16_t offset) const {
  return SumOffsetToAddress((const Address &)*this, offset);
}

Address Address::WithOffsetNoWrapAround(int16_t offset) const {
  return SumOffsetToAddressNoWrapAround((const Address &)*this, offset);
}

Address Address::WithOffsetWrapAround(int16_t offset) const {
  return SumOffsetToAddressWrapAround((const Address &)*this, offset);
}

bool Address::InRange(const Address &start, const Address &end) const {
  uint32_t val32 = AsInt();
  return val32 >= start.AsInt() && val32 <= end.AsInt();
}

size_t Address::Size(const Address &begin, const Address &end) {
  return end.AsInt() - begin.AsInt();
}

uint32_t Address::AsInt() const { return (bank_ << 16) | offset_; }

std::ostream &operator<<(std::ostream &out, const Address &addr) {
  out << "0x" << std::hex << (int)addr.bank_ << ":" << std::hex << std::setw(4)
      << std::setfill('0') << addr.offset_;
  return out;
}

// static
Address Address::SumOffsetToAddressWrapAround(const Address &addr,
                                              int16_t offset) {
  return Address(addr.bank_, addr.offset_ + offset);
}

// static
Address Address::SumOffsetToAddress(const Address &addr, int16_t offset2) {
  // This wraps around by default
  // TODO figure out when to wrap around and when not to
  return SumOffsetToAddressWrapAround(addr, offset2);
}

// static
bool Address::OffsetsAreOnDifferentPages(uint16_t offset1, uint16_t offset2) {
  int pageOfFirst = static_cast<int>(std::floor(offset1 / PAGE_SIZE_BYTES));
  int pageOfSecond = static_cast<int>(std::floor(offset2 / PAGE_SIZE_BYTES));
  return pageOfFirst != pageOfSecond;
}
