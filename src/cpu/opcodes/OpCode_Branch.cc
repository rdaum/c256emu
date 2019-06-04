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
 * This file contains the implementation for all branch OpCodes
 */
int Cpu65816::executeBranchShortOnCondition(bool condition, OpCode &opCode) {
  uint8_t opCycles = 2;
  uint8_t destination = system_bus_.ReadByte(getAddressOfOpCodeData(opCode));
  // This is the address of the next instruction
  uint16_t actualDestination;
  if (condition) {
    // One extra cycle if the branch is taken
    opCycles++;
    uint16_t destination16;
    if (Binary::is8bitValueNegative(destination)) {
      destination16 = 0xFF00 | destination;
    } else {
      destination16 = destination;
    }
    actualDestination = program_address_.offset_ + 2 + destination16;
    // Emulation mode requires 1 extra cycle on page boundary crossing
    if (Address::OffsetsAreOnDifferentPages(program_address_.offset_,
                                            actualDestination) &&
        cpu_status_.emulation_flag) {
      opCycles++;
    }
  } else {
    actualDestination = program_address_.offset_ + 2;
  }
  Address newProgramAddress(program_address_.bank_, actualDestination);
  program_address_ = newProgramAddress;
  return opCycles;
}

int Cpu65816::executeBranchLongOnCondition(bool condition, OpCode &opCode) {
  if (condition) {
    uint16_t destination = system_bus_.ReadWord(getAddressOfOpCodeData(opCode));
    program_address_.offset_ += 3 + destination;
  }
  // CPU cycles: 4
  return 4;
}

void Cpu65816::executeBranch(OpCode &opCode) {
  switch (opCode.code()) {
  case (0xD0): // BNE
  {
    total_cycles_counter_ +=
        executeBranchShortOnCondition(!cpu_status_.zero_flag, opCode);
    break;
  }
  case (0xF0): // BEQ
  {
    total_cycles_counter_ +=
        executeBranchShortOnCondition(cpu_status_.zero_flag, opCode);
    break;
  }
  case (0x90): // BCC
  {
    total_cycles_counter_ +=
        executeBranchShortOnCondition(!cpu_status_.carry_flag, opCode);
    break;
  }
  case (0xB0): // BCS
  {
    total_cycles_counter_ +=
        executeBranchShortOnCondition(cpu_status_.carry_flag, opCode);
    break;
  }
  case (0x10): // BPL
  {
    int cycles = executeBranchShortOnCondition(!cpu_status_.sign_flag, opCode);
    total_cycles_counter_ += cycles;
    break;
  }
  case (0x30): // BMI
  {
    total_cycles_counter_ +=
        executeBranchShortOnCondition(cpu_status_.sign_flag, opCode);
    break;
  }
  case (0x50): // BVC
  {
    total_cycles_counter_ +=
        executeBranchShortOnCondition(!cpu_status_.overflow_flag, opCode);
    break;
  }
  case (0x70): // BVS
  {
    total_cycles_counter_ +=
        executeBranchShortOnCondition(cpu_status_.overflow_flag, opCode);
    break;
  }
  case (0x80): // BRA
  {
    total_cycles_counter_ += executeBranchShortOnCondition(true, opCode);
    break;
  }
  case (0x82): // BRL
  {
    total_cycles_counter_ += executeBranchLongOnCondition(true, opCode);
    break;
  }
  default: {
    LOG_UNEXPECTED_OPCODE(opCode);
  }
  }
}
