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
 * that deal directly with the status register.
 */

void Cpu65816::executeStatusReg(OpCode& opCode) {
  switch (opCode.code()) {
    case (0xC2):  // REP #const
    {
      uint8_t value = system_bus_.ReadByte(getAddressOfOpCodeData(opCode));
      cpu_status_.resetPRegister(value);
      addToProgramAddressAndCycles(2, 3);
      break;
    }
    case (0x38):  // SEC
    {
      cpu_status_.carry_flag = 1;
      addToProgramAddressAndCycles(1, 2);
      break;
    }
    case (0xF8):  // SED
    {
      cpu_status_.decimal_flag = 1;
      addToProgramAddressAndCycles(1, 2);
      break;
    }
    case (0x78):  // SEI
    {
      cpu_status_.interrupt_disable_flag = 1;
      addToProgramAddressAndCycles(1, 2);
      break;
    }
    case (0x58):  // CLI
    {
      cpu_status_.interrupt_disable_flag = 0;
      addToProgramAddressAndCycles(1, 2);
      break;
    }
    case (0xE2):  // SEP
    {
      uint8_t value = system_bus_.ReadByte(getAddressOfOpCodeData(opCode));
      if (cpu_status_.emulation_flag) {
        // In emulation mode status bits 4 and 5 are not affected
        // 0xCF = 11001111
        value &= 0xCF;
      }
      cpu_status_.setPRegister(value);

      addToProgramAddressAndCycles(2, 3);
      break;
    }
    case (0x18):  // CLC
    {
      cpu_status_.carry_flag = 0;
      addToProgramAddressAndCycles(1, 2);
      break;
    }
    case (0xD8):  // CLD
    {
      cpu_status_.decimal_flag = 0;
      addToProgramAddressAndCycles(1, 2);
      break;
    }
    case (0xB8):  // CLV
    {
      cpu_status_.overflow_flag = false;
      addToProgramAddressAndCycles(1, 2);
      break;
    }
    case (0xFB):  // XCE
    {
      bool oldCarry = cpu_status_.carry_flag;
      bool oldEmulation = cpu_status_.emulation_flag;
      if (oldCarry)
        cpu_status_.emulation_flag = true;
      else
        cpu_status_.emulation_flag = false;
      if (oldEmulation)
        cpu_status_.carry_flag = 1;
      else
        cpu_status_.carry_flag = 0;

      x_ &= 0xFF;
      y_ &= 0xFF;

      if (cpu_status_.emulation_flag) {
        cpu_status_.accumulator_width_flag = true;
        cpu_status_.index_width_flag = true;
      } else {
        cpu_status_.accumulator_width_flag = false;
        cpu_status_.index_width_flag = false;
      }

      // New stack
      stack_ = Stack(&system_bus_);

      addToProgramAddressAndCycles(1, 2);
      break;
    }
    default: { LOG_UNEXPECTED_OPCODE(opCode); }
  }
}
