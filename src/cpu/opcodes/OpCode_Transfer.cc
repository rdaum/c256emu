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
 * This file contains the implementation for transfer OpCodes.
 * These are OpCodes which transfer one register value to another.
 */

void Cpu65816::executeTransfer(OpCode &opCode) {
  switch (opCode.code()) {
  case (0xA8): // TAY
  {
    if ((accumulatorIs8BitWide() && indexIs8BitWide()) ||
        (accumulatorIs16BitWide() && indexIs8BitWide())) {
      uint8_t lower8BitsOfA = Binary::lower8BitsOf(a_);
      Binary::setLower8BitsOf16BitsValue(&y_, lower8BitsOfA);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(lower8BitsOfA);
    } else {
      y_ = a_;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(a_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0xAA): // TAX
  {
    if ((accumulatorIs8BitWide() && indexIs8BitWide()) ||
        (accumulatorIs16BitWide() && indexIs8BitWide())) {
      uint8_t lower8BitsOfA = Binary::lower8BitsOf(a_);
      Binary::setLower8BitsOf16BitsValue(&x_, lower8BitsOfA);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(lower8BitsOfA);
    } else {
      x_ = a_;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(a_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x5B): // TCD
  {
    dp_ = a_;
    cpu_status_.updateSignAndZeroFlagFrom16BitValue(dp_);
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x7B): // TDC
  {
    a_ = dp_;
    cpu_status_.updateSignAndZeroFlagFrom16BitValue(a_);
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x1B): // TCS
  {
    uint16_t currentStackPointer = stack_.stack_pointer();
    if (cpu_status_.emulation_flag) {
      Binary::setLower8BitsOf16BitsValue(&currentStackPointer,
                                         Binary::lower8BitsOf(a_));
    } else {
      currentStackPointer = a_;
    }
    stack_ = Stack(&system_bus_, currentStackPointer);

    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x3B): // TSC
  {
    a_ = stack_.stack_pointer();
    cpu_status_.updateSignAndZeroFlagFrom16BitValue(a_);
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0xBA): // TSX
  {
    uint16_t stackPointer = stack_.stack_pointer();
    if (indexIs8BitWide()) {
      uint8_t stackPointerLower8Bits = Binary::lower8BitsOf(stackPointer);
      Binary::setLower8BitsOf16BitsValue(&x_, stackPointerLower8Bits);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(stackPointerLower8Bits);
    } else {
      x_ = stackPointer;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(x_);
    }

    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x8A): // TXA
  {
    if (accumulatorIs8BitWide() && indexIs8BitWide()) {
      uint8_t value = Binary::lower8BitsOf(x_);
      Binary::setLower8BitsOf16BitsValue(&a_, value);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
    } else if (accumulatorIs8BitWide() && indexIs16BitWide()) {
      uint8_t value = Binary::lower8BitsOf(x_);
      Binary::setLower8BitsOf16BitsValue(&a_, value);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
    } else if (accumulatorIs16BitWide() && indexIs8BitWide()) {
      uint8_t value = Binary::lower8BitsOf(x_);
      a_ = value;
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
    } else {
      a_ = x_;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(a_);
    }

    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x98): // TYA
  {
    if (accumulatorIs8BitWide() && indexIs8BitWide()) {
      uint8_t value = Binary::lower8BitsOf(y_);
      Binary::setLower8BitsOf16BitsValue(&a_, value);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
    } else if (accumulatorIs8BitWide() && indexIs16BitWide()) {
      uint8_t value = Binary::lower8BitsOf(y_);
      Binary::setLower8BitsOf16BitsValue(&a_, value);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
    } else if (accumulatorIs16BitWide() && indexIs8BitWide()) {
      uint8_t value = Binary::lower8BitsOf(y_);
      a_ = value;
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
    } else {
      a_ = y_;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(a_);
    }

    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x9A): // TXS
  {
    if (cpu_status_.emulation_flag) {
      uint16_t newStackPointer = 0x100;
      newStackPointer |= Binary::lower8BitsOf(x_);
      stack_ = Stack(&system_bus_, newStackPointer);
    } else if (!cpu_status_.emulation_flag && indexIs8BitWide()) {
      stack_ = Stack(&system_bus_, Binary::lower8BitsOf(x_));
    } else if (!cpu_status_.emulation_flag && indexIs16BitWide()) {
      stack_ = Stack(&system_bus_, x_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x9B): // TXY
  {
    if (indexIs8BitWide()) {
      uint8_t value = Binary::lower8BitsOf(x_);
      Binary::setLower8BitsOf16BitsValue(&y_, value);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
    } else {
      y_ = x_;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(y_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0xBB): // TYX
  {
    if (indexIs8BitWide()) {
      uint8_t value = Binary::lower8BitsOf(y_);
      Binary::setLower8BitsOf16BitsValue(&x_, value);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
    } else {
      x_ = y_;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(x_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }

  default: {
    LOG_UNEXPECTED_OPCODE(opCode);
  }
  }
}
