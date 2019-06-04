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
 * This file contains the implementation for all STZ OpCodes.
 */

void Cpu65816::executeSTZ(OpCode& opCode) {
  Address dataAddress = getAddressOfOpCodeData(opCode);
  if (accumulatorIs8BitWide()) {
    system_bus_.StoreByte(dataAddress, 0x00);
  } else {
    system_bus_.StoreWord(dataAddress, 0x0000);
    total_cycles_counter_ += 1;
  }

  switch (opCode.code()) {
    case (0x9C):  // STZ Absolute
    {
      program_address_.offset_ += 3;
      total_cycles_counter_ += 4;
      break;
    }
    case (0x64):  // STZ Direct Page
    {
      if (Binary::lower8BitsOf(dp_) != 0) {
        total_cycles_counter_ += 1;
      }

      program_address_.offset_ += 2;
      total_cycles_counter_ += 3;
      break;
    }
    case (0x9E):  // STZ Absolute Indexed, X
    {
      program_address_.offset_ += 3;
      total_cycles_counter_ += 5;
      break;
    }
    case (0x74):  // STZ Direct Page Indexed, X
    {
      if (Binary::lower8BitsOf(dp_) != 0) {
        total_cycles_counter_ += 1;
      }

      program_address_.offset_ += 2;
      total_cycles_counter_ += 4;
      break;
    }
    default: { LOG_UNEXPECTED_OPCODE(opCode); }
  }
}
