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
#include "cpu/interrupt.h"



/**
 * This file contains the implementation for TSB and TRB OpCodes
 */

void Cpu65816::execute8BitTSB(OpCode& opCode) {
  const Address addressOfOpCodeData = getAddressOfOpCodeData(opCode);
  uint8_t value = system_bus_.ReadByte(addressOfOpCodeData);
  uint8_t lowerA = Binary::lower8BitsOf(a_);
  const uint8_t result = value | lowerA;
  system_bus_.StoreByte(addressOfOpCodeData, result);

  if ((value & lowerA) == 0)
    cpu_status_.zero_flag = true;
  else
    cpu_status_.zero_flag = 0;
}

void Cpu65816::execute16BitTSB(OpCode& opCode) {
  const Address addressOfOpCodeData = getAddressOfOpCodeData(opCode);
  uint16_t value = system_bus_.ReadWord(addressOfOpCodeData);
  const uint16_t result = value | a_;
  system_bus_.StoreWord(addressOfOpCodeData, result);

  if ((value & a_) == 0)
    cpu_status_.zero_flag = true;
  else
    cpu_status_.zero_flag = 0;
}

void Cpu65816::execute8BitTRB(OpCode& opCode) {
  const Address addressOfOpCodeData = getAddressOfOpCodeData(opCode);
  uint8_t value = system_bus_.ReadByte(addressOfOpCodeData);
  uint8_t lowerA = Binary::lower8BitsOf(a_);
  const uint8_t result = value & ~lowerA;
  system_bus_.StoreByte(addressOfOpCodeData, result);

  if ((value & lowerA) == 0)
    cpu_status_.zero_flag = true;
  else
    cpu_status_.zero_flag = 0;
}

void Cpu65816::execute16BitTRB(OpCode& opCode) {
  const Address addressOfOpCodeData = getAddressOfOpCodeData(opCode);
  uint16_t value = system_bus_.ReadWord(addressOfOpCodeData);
  const uint16_t result = value & ~a_;
  system_bus_.StoreWord(addressOfOpCodeData, result);

  if ((value & a_) == 0)
    cpu_status_.zero_flag = true;
  else
    cpu_status_.zero_flag = 0;
}

void Cpu65816::executeTSBTRB(OpCode& opCode) {
  switch (opCode.code()) {
    case (0x0C):  // TSB Absolute
    {
      if (accumulatorIs8BitWide()) {
        execute8BitTSB(opCode);
      } else {
        execute16BitTSB(opCode);
        total_cycles_counter_ += 2;
      }
      addToProgramAddressAndCycles(3, 6);
      break;
    }
    case (0x04):  // TSB Direct Page
    {
      if (accumulatorIs8BitWide()) {
        execute8BitTSB(opCode);
      } else {
        execute16BitTSB(opCode);
        total_cycles_counter_ += 2;
      }
      if (Binary::lower8BitsOf(dp_) != 0) {
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(2, 5);
      break;
    }
    case (0x1C):  // TRB Absolute
    {
      if (accumulatorIs8BitWide()) {
        execute8BitTRB(opCode);
      } else {
        execute16BitTRB(opCode);
        total_cycles_counter_ += 2;
      }
      addToProgramAddressAndCycles(3, 6);
      break;
    }
    case (0x14):  // TRB Direct Page
    {
      if (accumulatorIs8BitWide()) {
        execute8BitTRB(opCode);
      } else {
        execute16BitTRB(opCode);
        total_cycles_counter_ += 2;
      }
      if (Binary::lower8BitsOf(dp_) != 0) {
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(2, 5);
      break;
    }
    default: { LOG_UNEXPECTED_OPCODE(opCode); }
  }
}
