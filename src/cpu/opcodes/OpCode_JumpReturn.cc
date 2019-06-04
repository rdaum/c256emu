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

#include "cpu/cpu_65816.h"

/**
 * This file contains the implementation for all OpCodes
 * that deal with jumps, calls and returns.
 */

void Cpu65816::executeJumpReturn(OpCode &opCode) {
  switch (opCode.code()) {
  case (0x20): // JSR Absolute
  {
    stack_.Push16Bit(program_address_.offset_ + 2);
    uint16_t destinationAddress = getAddressOfOpCodeData(opCode).offset_;
    program_address_ = Address(program_address_.bank_, destinationAddress);
    total_cycles_counter_ += 6;
    break;
  }
  case (0x22): // JSR Absolute Long
  {
    stack_.Push8Bit(program_address_.bank_);
    stack_.Push16Bit(program_address_.offset_ + 3);
    program_address_ = getAddressOfOpCodeData(opCode);
    total_cycles_counter_ += 8;
    break;
  }
  case (0xFC): // JSR Absolute Indexed Indirect, X
  {
    Address destinationAddress = getAddressOfOpCodeData(opCode);
    stack_.Push8Bit(program_address_.bank_);
    stack_.Push16Bit(program_address_.offset_ + 2);
    program_address_ = destinationAddress;
    total_cycles_counter_ += 8;
    break;
  }
  case (0x4C): // JMP Absolute
  {
    uint16_t destinationAddress = getAddressOfOpCodeData(opCode).offset_;
    program_address_ = Address(program_address_.bank_, destinationAddress);
    total_cycles_counter_ += 3;
    break;
  }
  case (0x6C): // JMP Absolute Indirect
  {
    program_address_ = getAddressOfOpCodeData(opCode);
    total_cycles_counter_ += 5;
#ifdef EMU_65C02
    total_cycles_counter_ += 1;
#endif
    break;
  }
  case (0x7C): // JMP Absolute Indexed Indirect, X
  {
    program_address_ = getAddressOfOpCodeData(opCode);
    total_cycles_counter_ += 6;
    break;
  }
  case (0x5C): // JMP Absolute Long
  {
    program_address_ = getAddressOfOpCodeData(opCode);
    total_cycles_counter_ += 4;
    break;
  }
  case (0xDC): // JMP Absolute Indirect Long
  {
    program_address_ = getAddressOfOpCodeData(opCode);
    total_cycles_counter_ += 6;
    break;
  }
  case (0x6B): // RTL
  {
    uint16_t newOffset = stack_.Pull16Bit() + 1;
    uint8_t newBank = stack_.Pull8Bit();

    Address returnAddress(newBank, newOffset);
    program_address_ = returnAddress;
    total_cycles_counter_ += 6;
    break;
  }
  case (0x60): // RTS
  {
    Address returnAddress(program_address_.bank_, stack_.Pull16Bit() + 1);
    program_address_ = returnAddress;
    total_cycles_counter_ += 6;
    break;
  }
  default: {
    LOG_UNEXPECTED_OPCODE(opCode);
  }
  }
}
