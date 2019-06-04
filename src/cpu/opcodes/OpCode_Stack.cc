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
 * This file contains the implementation for every stack related OpCode.
 */
void Cpu65816::executeStack(OpCode &opCode) {
  Address opCodeDataAddress = getAddressOfOpCodeData(opCode);
  switch (opCode.code()) {
  case 0xF4: // PEA
  {
    uint16_t operand = system_bus_.ReadWord(opCodeDataAddress);
    stack_.Push16Bit(operand);
    addToProgramAddressAndCycles(3, 5);
    break;
  }
  case 0xD4: // PEI
  {
    uint16_t operand = system_bus_.ReadWord(opCodeDataAddress);
    stack_.Push16Bit(operand);
    int opCycles = Binary::lower8BitsOf(dp_) != 0 ? 1 : 0;
    addToProgramAddressAndCycles(2, 6 + opCycles);
    break;
  }
  case 0x62: // PER
  {
    int opCodeSize = 3;
    uint16_t operand = system_bus_.ReadWord(opCodeDataAddress);
    uint16_t sum = operand + opCodeSize + program_address_.offset_;
    stack_.Push16Bit(sum);
    addToProgramAddressAndCycles(3, 6);
    break;
  }
  case (0x48): // PHA
  {
    if (accumulatorIs8BitWide()) {
      stack_.Push8Bit(Binary::lower8BitsOf(a_));
      addToProgramAddressAndCycles(1, 4);
    } else {
      stack_.Push16Bit(a_);
      addToProgramAddressAndCycles(1, 3);
    }
    break;
  }
  case (0x8B): // PHB
  {
    stack_.Push8Bit(db_);
    addToProgramAddressAndCycles(1, 3);
    break;
  }
  case (0x0B): // PHD
  {
    stack_.Push16Bit(dp_);
    addToProgramAddressAndCycles(1, 4);
    break;
  }
  case (0x4B): // PHK
  {
    stack_.Push8Bit(program_address_.bank_);
    addToProgramAddressAndCycles(1, 3);
    break;
  }
  case (0x08): // PHP
  {
    stack_.Push8Bit(cpu_status_.register_value());
    addToProgramAddressAndCycles(1, 3);
    break;
  }
  case (0xDA): // PHX
  {
    if (indexIs8BitWide()) {
      stack_.Push8Bit(Binary::lower8BitsOf(x_));
      addToProgramAddressAndCycles(1, 3);
    } else {
      stack_.Push16Bit(x_);
      addToProgramAddressAndCycles(1, 4);
    }
    break;
  }
  case (0x5A): // PHY
  {
    if (indexIs8BitWide()) {
      stack_.Push8Bit(Binary::lower8BitsOf(y_));
      addToProgramAddressAndCycles(1, 3);
    } else {
      stack_.Push16Bit(y_);
      addToProgramAddressAndCycles(1, 4);
    }
    break;
  }
  case (0x68): // PLA
  {
    if (accumulatorIs8BitWide()) {
      Binary::setLower8BitsOf16BitsValue(&a_, stack_.Pull8Bit());
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(a_);
      addToProgramAddressAndCycles(1, 4);
    } else {
      a_ = stack_.Pull16Bit();
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(a_);
      addToProgramAddressAndCycles(1, 5);
    }
    break;
  }
  case (0xAB): // PLB
  {
    db_ = stack_.Pull8Bit();
    cpu_status_.updateSignAndZeroFlagFrom8BitValue(db_);
    addToProgramAddressAndCycles(1, 4);
    break;
  }
  case (0x2B): // PLD
  {
    dp_ = stack_.Pull16Bit();
    cpu_status_.updateSignAndZeroFlagFrom16BitValue(dp_);
    addToProgramAddressAndCycles(1, 5);
    break;
  }
  case (0x28): // PLP
  {
    cpu_status_.setRegisterValue(stack_.Pull8Bit());
    addToProgramAddressAndCycles(1, 4);
    break;
  }
  case (0xFA): // PLX
  {
    if (indexIs8BitWide()) {
      uint8_t value = stack_.Pull8Bit();
      Binary::setLower8BitsOf16BitsValue(&x_, value);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
      addToProgramAddressAndCycles(1, 4);
    } else {
      x_ = stack_.Pull16Bit();
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(x_);
      addToProgramAddressAndCycles(1, 5);
    }
    break;
  }
  case (0x7A): // PLY
  {
    if (indexIs8BitWide()) {
      uint8_t value = stack_.Pull8Bit();
      Binary::setLower8BitsOf16BitsValue(&y_, value);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
      addToProgramAddressAndCycles(1, 4);
    } else {
      y_ = stack_.Pull16Bit();
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(y_);
      addToProgramAddressAndCycles(1, 5);
    }
    break;
  }
  default: {
    LOG_UNEXPECTED_OPCODE(opCode);
  }
  }
}
