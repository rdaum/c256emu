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
 * This file contains the implementation for all CMP OpCodes
 */

void Cpu65816::execute8BitCMP(OpCode &opCode) {
  Address valueAddress = getAddressOfOpCodeData(opCode);
  uint8_t value = system_bus_.ReadByte(valueAddress);
  uint8_t result = Binary::lower8BitsOf(a_) - value;
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(result);
  if (Binary::lower8BitsOf(a_) >= value) {
    cpu_status_.carry_flag = 1;
  } else {
    cpu_status_.carry_flag = 0;
  }
}

void Cpu65816::execute16BitCMP(OpCode &opCode) {
  Address valueAddress = getAddressOfOpCodeData(opCode);
  uint16_t value = system_bus_.ReadWord(valueAddress);
  uint16_t result = a_ - value;
  cpu_status_.updateSignAndZeroFlagFrom16BitValue(result);
  if (a_ >= value) {
    cpu_status_.carry_flag = 1;
  } else {
    cpu_status_.carry_flag = 0;
  }
}

void Cpu65816::executeCMP(OpCode &opCode) {
  if (accumulatorIs8BitWide()) {
    execute8BitCMP(opCode);
  } else {
    execute16BitCMP(opCode);
    total_cycles_counter_ += 1;
  }

  switch (opCode.code()) {
  case (0xC9): // CMP Immediate
  {
    if (accumulatorIs16BitWide()) {
      program_address_.offset_ += 1;
    }
    addToProgramAddressAndCycles(2, 2);
    break;
  }
  case (0xCD): // CMP Absolute
  {
    addToProgramAddressAndCycles(3, 4);
    break;
  }
  case (0xCF): // CMP Absolute Long
  {
    addToProgramAddressAndCycles(4, 5);
    break;
  }
  case (0xC5): // CMP Direct Page
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 3);
    break;
  }
  case (0xD2): // CMP Direct Page Indirect
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 5);
    break;
  }
  case (0xC7): // CMP Direct Page Indirect Long
  {
    addToProgramAddressAndCycles(2, 6);
    break;
  }
  case (0xDD): // CMP Absolute Indexed, X
  {
    if (opCodeAddressingCrossesPageBoundary(opCode)) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(3, 4);
    break;
  }
  case (0xDF): // CMP Absolute Long Indexed, X
  {
    addToProgramAddressAndCycles(4, 5);
    break;
  }
  case (0xD9): // CMP Absolute Indexed, Y
  {
    if (opCodeAddressingCrossesPageBoundary(opCode)) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(3, 4);
    break;
  }
  case (0xD5): // CMP Direct Page Indexed, X
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 4);
    break;
  }
  case (0xC1): // CMP Direct Page Indexed Indirect, X
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 6);
    break;
  }
  case (0xD1): // CMP Direct Page Indexed Indirect, Y
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    if (opCodeAddressingCrossesPageBoundary(opCode)) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 5);
    break;
  }
  case (0xD7): // CMP Direct Page Indirect Long Indexed, Y
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 6);
    break;
  }
  case (0xC3): // CMP Stack Relative
  {
    addToProgramAddressAndCycles(2, 4);
    break;
  }
  case (0xD3): // CMP Stack Relative Indirect Indexed, Y
  {
    addToProgramAddressAndCycles(2, 7);
    break;
  }
  default: {
    LOG_UNEXPECTED_OPCODE(opCode);
  }
  }
}
