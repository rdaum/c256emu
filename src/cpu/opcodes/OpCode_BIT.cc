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
 * This file contains the implementation for all BIT OpCodes
 */

void Cpu65816::execute8BitBIT(OpCode& opCode) {
  const Address addressOfOpCodeData = getAddressOfOpCodeData(opCode);
  uint8_t value = system_bus_.ReadByte(addressOfOpCodeData);
  bool isHighestBitSet = value & 0x80;
  bool isNextToHighestBitSet = value & 0x40;

  if (opCode.addressing_mode() != AddressingMode::Immediate) {
    if (isHighestBitSet)
      cpu_status_.sign_flag = true;
    else
      cpu_status_.sign_flag = false;
    if (isNextToHighestBitSet)
      cpu_status_.overflow_flag = true;
    else
      cpu_status_.overflow_flag = false;
  }
  cpu_status_.updateZeroFlagFrom8BitValue(value & Binary::lower8BitsOf(a_));
}

void Cpu65816::execute16BitBIT(OpCode& opCode) {
  const Address addressOfOpCodeData = getAddressOfOpCodeData(opCode);
  uint16_t value = system_bus_.ReadWord(addressOfOpCodeData);
  bool isHighestBitSet = value & 0x8000;
  bool isNextToHighestBitSet = value & 0x4000;

  if (opCode.addressing_mode() != AddressingMode::Immediate) {
    if (isHighestBitSet)
      cpu_status_.sign_flag = true;
    else
      cpu_status_.sign_flag = false;
    if (isNextToHighestBitSet)
      cpu_status_.overflow_flag = true;
    else
      cpu_status_.overflow_flag = false;
  }
  cpu_status_.updateZeroFlagFrom16BitValue(value & a_);
}

void Cpu65816::executeBIT(OpCode& opCode) {
  if (accumulatorIs8BitWide()) {
    execute8BitBIT(opCode);
  } else {
    execute16BitBIT(opCode);
    total_cycles_counter_ += 1;
  }

  switch (opCode.code()) {
    case (0x89):  // BIT Immediate
    {
      if (accumulatorIs16BitWide()) {
        program_address_.offset_ += 1;
      }
      addToProgramAddressAndCycles(2, 2);
      break;
    }
    case (0x2C):  // BIT Absolute
    {
      addToProgramAddressAndCycles(3, 4);
      break;
    }
    case (0x24):  // BIT Direct Page
    {
      if (Binary::lower8BitsOf(dp_) != 0) {
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(2, 3);
      break;
    }
    case (0x3C):  // BIT Absolute Indexed, X
    {
      if (opCodeAddressingCrossesPageBoundary(opCode)) {
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(3, 4);
      break;
    }
    case (0x34):  // BIT Direct Page Indexed, X
    {
      if (Binary::lower8BitsOf(dp_) != 0) {
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(2, 4);
      break;
    }
    default: { LOG_UNEXPECTED_OPCODE(opCode); }
  }
}
