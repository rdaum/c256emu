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
 * This file contains the implementation for all STA OpCodes.
 */

void Cpu65816::executeSTA(OpCode &opCode) {
  Address dataAddress = getAddressOfOpCodeData(opCode);
  if (accumulatorIs8BitWide()) {
    system_bus_.StoreByte(dataAddress, Binary::lower8BitsOf(a_));
  } else {
    system_bus_.StoreWord(dataAddress, a_);
    total_cycles_counter_ += 1;
  }

  switch (opCode.code()) {
  case (0x8D): // STA Absolute
  {
    program_address_.offset_ += 3;
    total_cycles_counter_ += 4;
    break;
  }
  case (0x8F): // STA Absolute Long
  {
    program_address_.offset_ += 4;
    total_cycles_counter_ += 5;
    break;
  }
  case (0x85): // STA Direct Page
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 3;
    break;
  }
  case (0x92): // STA Direct Page Indirect
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 5;
    break;
  }
  case (0x87): // STA Direct Page Indirect Long
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 6;
    break;
  }
  case (0x9D): // STA Absolute Indexed, X
  {
    program_address_.offset_ += 3;
    total_cycles_counter_ += 5;
    break;
  }
  case (0x9F): // STA Absolute Long Indexed, X
  {
    program_address_.offset_ += 4;
    total_cycles_counter_ += 5;
    break;
  }
  case (0x99): // STA Absolute Indexed, Y
  {
    program_address_.offset_ += 3;
    total_cycles_counter_ += 5;
    break;
  }
  case (0x95): // STA Direct Page Indexed, X
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 4;
    break;
  }
  case (0x81): // STA Direct Page Indexed Indirect, X
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 6;
    break;
  }
  case (0x91): // STA Direct Page Indirect Indexed, Y
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 6;
    break;
  }
  case (0x97): // STA Direct Page Indirect Long Indexed, Y
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 6;
    break;
  }
  case (0x83): // STA Stack Relative
  {
    program_address_.offset_ += 2;
    total_cycles_counter_ += 4;
    break;
  }
  case (0x93): // STA Stack Relative Indirect Indexed, Y
  {
    program_address_.offset_ += 2;
    total_cycles_counter_ += 7;
    break;
  }
  default: {
    LOG_UNEXPECTED_OPCODE(opCode);
  }
  }
}
