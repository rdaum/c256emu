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

#include <cmath>

#include "cpu/cpu_65816.h"



/**
 * This file contains implementations for all LDX OpCodes.
 */

void Cpu65816::executeLDX8Bit(OpCode& opCode) {
  Address opCodeDataAddress = getAddressOfOpCodeData(opCode);
  uint8_t value = system_bus_.ReadByte(opCodeDataAddress);
  Binary::setLower8BitsOf16BitsValue(&x_, value);
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
}

void Cpu65816::executeLDX16Bit(OpCode& opCode) {
  Address opCodeDataAddress = getAddressOfOpCodeData(opCode);
  x_ = system_bus_.ReadWord(opCodeDataAddress);
  cpu_status_.updateSignAndZeroFlagFrom16BitValue(x_);
}

void Cpu65816::executeLDX(OpCode& opCode) {
  if (indexIs16BitWide()) {
    executeLDX16Bit(opCode);
    total_cycles_counter_ += 1;
  } else {
    executeLDX8Bit(opCode);
  }

  switch (opCode.code()) {
    case (0xA2):  // LDX Immediate
    {
      if (indexIs16BitWide()) {
        program_address_.offset_ += 1;
      }
      addToProgramAddressAndCycles(2, 2);
      break;
    }
    case (0xAE):  // LDX Absolute
    {
      addToProgramAddressAndCycles(3, 4);
      break;
    }
    case (0xA6):  // LDX Direct Page
    {
      if (Binary::lower8BitsOf(dp_) != 0) {
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(2, 3);
      break;
    }
    case (0xBE):  // LDX Absolute Indexed, Y
    {
      if (opCodeAddressingCrossesPageBoundary(opCode)) {
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(3, 4);
      break;
    }
    case (0xB6):  // LDX Direct Page Indexed, Y
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
