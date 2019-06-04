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
 * This file contains implementations for all CPX and CPY OpCodes.
 */

void Cpu65816::execute8BitCPX(OpCode& opCode) {
  uint8_t value = system_bus_.ReadByte(getAddressOfOpCodeData(opCode));
  uint8_t result = Binary::lower8BitsOf(x_) - value;
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(result);
  if (Binary::lower8BitsOf(x_) >= value)
    cpu_status_.carry_flag = 1;
  else
    cpu_status_.carry_flag = 0;
}

void Cpu65816::execute16BitCPX(OpCode& opCode) {
  uint16_t value = system_bus_.ReadWord(getAddressOfOpCodeData(opCode));
  uint16_t result = x_ - value;
  cpu_status_.updateSignAndZeroFlagFrom16BitValue(result);
  if (x_ >= value)
    cpu_status_.carry_flag = 1;
  else
    cpu_status_.carry_flag = 0;
}

void Cpu65816::execute8BitCPY(OpCode& opCode) {
  uint8_t value = system_bus_.ReadByte(getAddressOfOpCodeData(opCode));
  uint8_t result = Binary::lower8BitsOf(y_) - value;
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(result);
  if (Binary::lower8BitsOf(y_) >= value)
    cpu_status_.carry_flag = 1;
  else
    cpu_status_.carry_flag = 0;
}

void Cpu65816::execute16BitCPY(OpCode& opCode) {
  uint16_t value = system_bus_.ReadWord(getAddressOfOpCodeData(opCode));
  uint16_t result = y_ - value;
  cpu_status_.updateSignAndZeroFlagFrom16BitValue(result);
  if (y_ >= value)
    cpu_status_.carry_flag = 1;
  else
    cpu_status_.carry_flag = 0;
}

void Cpu65816::executeCPXCPY(OpCode& opCode) {
  switch (opCode.code()) {
    case (0xE0):  // CPX Immediate
    {
      if (indexIs8BitWide()) {
        execute8BitCPX(opCode);
      } else {
        execute16BitCPX(opCode);
        program_address_.offset_ += 1;
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(2, 2);
      break;
    }
    case (0xEC):  // CPX Absolute
    {
      if (indexIs8BitWide()) {
        execute8BitCPX(opCode);
      } else {
        execute16BitCPX(opCode);
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(3, 4);
      break;
    }
    case (0xE4):  // CPX Direct Page
    {
      if (indexIs8BitWide()) {
        execute8BitCPX(opCode);
      } else {
        execute16BitCPX(opCode);
        total_cycles_counter_ += 1;
      }
      if (Binary::lower8BitsOf(dp_) != 0) {
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(2, 3);
      break;
    }
    case (0xC0):  // CPY Immediate
    {
      if (indexIs8BitWide()) {
        execute8BitCPY(opCode);
      } else {
        execute16BitCPY(opCode);
        program_address_.offset_ += 1;
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(2, 2);
      break;
    }
    case (0xCC):  // CPY Absolute
    {
      if (indexIs8BitWide()) {
        execute8BitCPY(opCode);
      } else {
        execute16BitCPY(opCode);
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(3, 4);
      break;
    }
    case (0xC4):  // CPY Direct Page
    {
      if (indexIs8BitWide()) {
        execute8BitCPY(opCode);
      } else {
        execute16BitCPY(opCode);
        total_cycles_counter_ += 1;
      }
      if (Binary::lower8BitsOf(dp_) != 0) {
        total_cycles_counter_ += 1;
      }
      addToProgramAddressAndCycles(2, 3);
      break;
    }
    default: { LOG_UNEXPECTED_OPCODE(opCode); }
  }
}
