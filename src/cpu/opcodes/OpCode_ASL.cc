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

#define DO_ASL_8_BIT(value)                                                    \
  {                                                                            \
    bool newCarry = value & 0x80;                                              \
    value = value << 1;                                                        \
    if (newCarry)                                                              \
      cpu_status_.carry_flag = 1;                                              \
    else                                                                       \
      cpu_status_.carry_flag = 0;                                              \
    cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);                     \
  }

#define DO_ASL_16_BIT(value)                                                   \
  {                                                                            \
    bool newCarry = value & 0x8000;                                            \
    value = value << 1;                                                        \
    if (newCarry)                                                              \
      cpu_status_.carry_flag = 1;                                              \
    else                                                                       \
      cpu_status_.carry_flag = 0;                                              \
    cpu_status_.updateSignAndZeroFlagFrom16BitValue(value);                    \
  }

/**
 * This file contains implementations for all ASL OpCodes.
 */

void Cpu65816::executeMemoryASL(OpCode &opCode) {
  Address opCodeDataAddress = getAddressOfOpCodeData(opCode);

  if (accumulatorIs8BitWide()) {
    uint8_t value = system_bus_.ReadByte(opCodeDataAddress);
    DO_ASL_8_BIT(value);
    system_bus_.StoreByte(opCodeDataAddress, value);
  } else {
    uint16_t value = system_bus_.ReadWord(opCodeDataAddress);
    DO_ASL_16_BIT(value);
    system_bus_.StoreWord(opCodeDataAddress, value);
  }
}

void Cpu65816::executeAccumulatorASL() {
  if (accumulatorIs8BitWide()) {
    uint8_t value = Binary::lower8BitsOf(a_);
    DO_ASL_8_BIT(value);
    Binary::setLower8BitsOf16BitsValue(&a_, value);
  } else {
    DO_ASL_16_BIT(a_);
  }
}

void Cpu65816::executeASL(OpCode &opCode) {
  switch (opCode.code()) {
  case (0x0A): // ASL Accumulator
  {
    executeAccumulatorASL();
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x0E): // ASL Absolute
  {
    if (accumulatorIs16BitWide()) {
      total_cycles_counter_ += 2;
    }

    executeMemoryASL(opCode);
    addToProgramAddressAndCycles(3, 6);
    break;
  }
  case (0x06): // ASL Direct Page
  {
    if (accumulatorIs16BitWide()) {
      total_cycles_counter_ += 2;
    }
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    executeMemoryASL(opCode);
    addToProgramAddressAndCycles(2, 5);
    break;
  }
  case (0x1E): // ASL Absolute Indexed, X
  {
    if (accumulatorIs16BitWide()) {
      total_cycles_counter_ += 2;
    }
#ifdef EMU_65C02
    if (!opCodeAddressingCrossesPageBoundary(opCode)) {
      subtractFromCycles(1);
    }
#endif
    executeMemoryASL(opCode);
    addToProgramAddressAndCycles(3, 7);
    break;
  }
  case (0x16): // ASL Direct Page Indexed, X
  {
    if (accumulatorIs16BitWide()) {
      total_cycles_counter_ += 2;
    }
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    executeMemoryASL(opCode);
    addToProgramAddressAndCycles(2, 6);
    break;
  }
  default: {
    LOG_UNEXPECTED_OPCODE(opCode);
  }
  }
}
