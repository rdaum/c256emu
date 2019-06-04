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
 * This file contains implementations for all OpCodes that didn't fall into
 * other categories.
 */

void Cpu65816::executeMisc(OpCode& opCode) {
  switch (opCode.code()) {
    case (0xEB):  // XBA
    {
      uint8_t lowerA = Binary::lower8BitsOf(a_);
      uint8_t higherA = Binary::higher8BitsOf(a_);
      a_ = higherA | (((uint16_t)(lowerA)) << 8);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(higherA);
      addToProgramAddressAndCycles(1, 3);
      break;
    }
    case (0xDB):  // STP
    {
      reset();
      program_address_.offset_ += 1;
      total_cycles_counter_ += 3;
      break;
    }
    case (0xCB):  // WAI
    {
      SetRDYPin(false);

      program_address_.offset_ += 1;
      total_cycles_counter_ += 3;
      break;
    }
    case (0x42):  // WDM
    {
      program_address_.offset_ += 2;
      total_cycles_counter_ += 2;
      break;
    }
    case (0xEA):  // NOP
    {
      program_address_.offset_ += 1;
      total_cycles_counter_ += 2;
      break;
    }
    case (0x44):  // MVP
    {
      Address addressOfOpCodeData = getAddressOfOpCodeData(opCode);
      uint8_t destinationBank = system_bus_.ReadByte(addressOfOpCodeData);
      addressOfOpCodeData.offset_ += 1;
      uint8_t sourceBank = system_bus_.ReadByte(addressOfOpCodeData);

      Address sourceAddress(sourceBank, x_);
      Address destinationAddress(destinationBank, y_);

      while (a_ != 0xFFFF) {
        uint8_t toTransfer = system_bus_.ReadByte(sourceAddress);
        system_bus_.StoreByte(destinationAddress, toTransfer);

        sourceAddress.offset_ -= 1;
        destinationAddress.offset_ -= 1;
        a_--;

        total_cycles_counter_ += 7;
      }
      db_ = destinationBank;

      program_address_.offset_ += 3;
      break;
    }
    case (0x54):  // MVN
    {
      Address addressOfOpCodeData = getAddressOfOpCodeData(opCode);
      uint8_t destinationBank = system_bus_.ReadByte(addressOfOpCodeData);
      addressOfOpCodeData.offset_ += 1;
      uint8_t sourceBank = system_bus_.ReadByte(addressOfOpCodeData);

      Address sourceAddress(sourceBank, x_);
      Address destinationAddress(destinationBank, y_);

      while (a_ != 0xFFFF) {
        uint8_t toTransfer = system_bus_.ReadByte(sourceAddress);
        system_bus_.StoreByte(destinationAddress, toTransfer);

        sourceAddress.offset_ += 1;
        destinationAddress.offset_ += 1;
        a_--;

        total_cycles_counter_ += 7;
      }
      db_ = destinationBank;

      program_address_.offset_ += 3;
      break;
    }
    default: { LOG_UNEXPECTED_OPCODE(opCode); }
  }
}
