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
 * This file contains implementations for all Increment and Decrement OpCodes.
 */

void Cpu65816::execute8BitDecInMemory(OpCode &opCode) {
  Address opCodeDataAddress = getAddressOfOpCodeData(opCode);
  uint8_t value = system_bus_.ReadByte(opCodeDataAddress);
  value--;
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
  system_bus_.StoreByte(opCodeDataAddress, value);
}

void Cpu65816::execute16BitDecInMemory(OpCode &opCode) {
  Address opCodeDataAddress = getAddressOfOpCodeData(opCode);
  uint16_t value = system_bus_.ReadWord(opCodeDataAddress);
  value--;
  cpu_status_.updateSignAndZeroFlagFrom16BitValue(value);
  system_bus_.StoreWord(opCodeDataAddress, value);
}

void Cpu65816::execute8BitIncInMemory(OpCode &opCode) {
  Address opCodeDataAddress = getAddressOfOpCodeData(opCode);
  uint8_t value = system_bus_.ReadByte(opCodeDataAddress);
  value++;
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);
  system_bus_.StoreByte(opCodeDataAddress, value);
}

void Cpu65816::execute16BitIncInMemory(OpCode &opCode) {
  Address opCodeDataAddress = getAddressOfOpCodeData(opCode);
  uint16_t value = system_bus_.ReadWord(opCodeDataAddress);
  value++;
  cpu_status_.updateSignAndZeroFlagFrom16BitValue(value);
  system_bus_.StoreWord(opCodeDataAddress, value);
}

void Cpu65816::executeINCDEC(OpCode &opCode) {
  switch (opCode.code()) {
  case (0x1A): // INC Accumulator
  {
    if (accumulatorIs8BitWide()) {
      uint8_t lowerA = Binary::lower8BitsOf(a_);
      lowerA++;
      Binary::setLower8BitsOf16BitsValue(&a_, lowerA);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(lowerA);
    } else {
      a_++;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(a_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0xEE): // INC Absolute
  {
    if (accumulatorIs8BitWide()) {
      execute8BitIncInMemory(opCode);
    } else {
      execute16BitIncInMemory(opCode);
      total_cycles_counter_ += 2;
    }
    addToProgramAddressAndCycles(3, 6);
    break;
  }
  case (0xE6): // INC Direct Page
  {
    if (accumulatorIs8BitWide()) {
      execute8BitIncInMemory(opCode);
    } else {
      execute16BitIncInMemory(opCode);
      total_cycles_counter_ += 2;
    }
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 5);
    break;
  }
  case (0xFE): // INC Absolute Indexed, X
  {
    if (accumulatorIs8BitWide()) {
      execute8BitIncInMemory(opCode);
    } else {
      execute16BitIncInMemory(opCode);
      total_cycles_counter_ += 2;
    }
#ifdef EMU_65C02
    if (!opCodeAddressingCrossesPageBoundary(opCode)) {
      subtractFromCycles(1);
    }
#endif
    addToProgramAddressAndCycles(3, 7);
  } break;
  case (0xF6): // INC Direct Page Indexed, X
  {
    if (accumulatorIs8BitWide()) {
      execute8BitIncInMemory(opCode);
    } else {
      execute16BitIncInMemory(opCode);
      total_cycles_counter_ += 2;
    }
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 6);
    break;
  }
  case (0x3A): // DEC Accumulator
  {
    if (accumulatorIs8BitWide()) {
      uint8_t lowerA = Binary::lower8BitsOf(a_);
      lowerA--;
      Binary::setLower8BitsOf16BitsValue(&a_, lowerA);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(lowerA);
    } else {
      a_--;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(a_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0xCE): // DEC Absolute
  {
    if (accumulatorIs8BitWide()) {
      execute8BitDecInMemory(opCode);
    } else {
      execute16BitDecInMemory(opCode);
      total_cycles_counter_ += 2;
    }
    addToProgramAddressAndCycles(3, 6);
    break;
  }
  case (0xC6): // DEC Direct Page
  {
    if (accumulatorIs8BitWide()) {
      execute8BitDecInMemory(opCode);
    } else {
      execute16BitDecInMemory(opCode);
      total_cycles_counter_ += 2;
    }
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 5);
    break;
  }
  case (0xDE): // DEC Absolute Indexed, X
  {
    if (accumulatorIs8BitWide()) {
      execute8BitDecInMemory(opCode);
    } else {
      execute16BitDecInMemory(opCode);
      total_cycles_counter_ += 2;
    }
#ifdef EMU_65C02
    if (!opCodeAddressingCrossesPageBoundary(opCode)) {
      subtractFromCycles(1);
    }
#endif
    addToProgramAddressAndCycles(3, 7);
    break;
  }
  case (0xD6): // DEC Direct Page Indexed, X
  {
    if (accumulatorIs8BitWide()) {
      execute8BitDecInMemory(opCode);
    } else {
      execute16BitDecInMemory(opCode);
      total_cycles_counter_ += 2;
    }
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    addToProgramAddressAndCycles(2, 6);
    break;
  }
  case (0xC8): // INY
  {
    if (indexIs8BitWide()) {
      uint8_t lowerY = Binary::lower8BitsOf(y_);
      lowerY++;
      Binary::setLower8BitsOf16BitsValue(&y_, lowerY);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(lowerY);
    } else {
      y_++;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(y_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0xE8): // INX
  {
    if (indexIs8BitWide()) {
      uint8_t lowerX = Binary::lower8BitsOf(x_);
      lowerX++;
      Binary::setLower8BitsOf16BitsValue(&x_, lowerX);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(lowerX);
    } else {
      x_++;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(x_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x88): // DEY
  {
    if (indexIs8BitWide()) {
      uint8_t lowerY = Binary::lower8BitsOf(y_);
      lowerY--;
      Binary::setLower8BitsOf16BitsValue(&y_, lowerY);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(lowerY);
    } else {
      y_--;
      cpu_status_.updateSignAndZeroFlagFrom16BitValue(y_);
    }
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0xCA): // DEX
  {
    if (indexIs8BitWide()) {
      uint8_t lowerX = Binary::lower8BitsOf(x_);
      lowerX--;
      Binary::setLower8BitsOf16BitsValue(&x_, lowerX);
      cpu_status_.updateSignAndZeroFlagFrom8BitValue(lowerX);
    } else {
      x_--;
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
