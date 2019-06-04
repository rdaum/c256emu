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

#define DO_ROL_8_BIT(value)                                                    \
  {                                                                            \
    bool carryWasSet = cpu_status_.carry_flag;                                 \
    bool carryWillBeSet = (value & 0x80) != 0;                                 \
    value = value << 1;                                                        \
    if (carryWasSet)                                                           \
      Binary::setBitIn8BitValue(&value, 0);                                    \
    else                                                                       \
      Binary::clearBitIn8BitValue(&value, 0);                                  \
    if (carryWillBeSet)                                                        \
      cpu_status_.carry_flag = 1;                                              \
    else                                                                       \
      cpu_status_.carry_flag = 0;                                              \
    cpu_status_.updateSignAndZeroFlagFrom8BitValue(value);                     \
  }

#define DO_ROL_16_BIT(value)                                                   \
  {                                                                            \
    bool carryWasSet = cpu_status_.carry_flag;                                 \
    bool carryWillBeSet = (value & 0x8000) != 0;                               \
    value = value << 1;                                                        \
    if (carryWasSet)                                                           \
      Binary::setBitIn16BitValue(&value, 0);                                   \
    else                                                                       \
      Binary::clearBitIn16BitValue(&value, 0);                                 \
    if (carryWillBeSet)                                                        \
      cpu_status_.carry_flag = 1;                                              \
    else                                                                       \
      cpu_status_.carry_flag = 0;                                              \
    cpu_status_.updateSignAndZeroFlagFrom16BitValue(value);                    \
  }

/**
 * This file contains implementations for all ROL OpCodes.
 */
void Cpu65816::executeMemoryROL(OpCode &opCode) {
  Address opCodeDataAddress = getAddressOfOpCodeData(opCode);

  if (accumulatorIs8BitWide()) {
    uint8_t value = system_bus_.ReadByte(opCodeDataAddress);
    DO_ROL_8_BIT(value);
    system_bus_.StoreByte(opCodeDataAddress, value);
  } else {
    uint16_t value = system_bus_.ReadWord(opCodeDataAddress);
    DO_ROL_16_BIT(value);
    system_bus_.StoreWord(opCodeDataAddress, value);
  }
}

void Cpu65816::executeAccumulatorROL() {
  if (accumulatorIs8BitWide()) {
    uint8_t value = Binary::lower8BitsOf(a_);
    DO_ROL_8_BIT(value);
    Binary::setLower8BitsOf16BitsValue(&a_, value);
  } else {
    uint16_t value = a_;
    DO_ROL_16_BIT(value);
    a_ = value;
  }
}

void Cpu65816::executeROL(OpCode &opCode) {
  switch (opCode.code()) {
  case (0x2A): // ROL accumulator
  {
    executeAccumulatorROL();
    addToProgramAddressAndCycles(1, 2);
    break;
  }
  case (0x2E): // ROL #addr
  {
    executeMemoryROL(opCode);
    if (accumulatorIs8BitWide()) {
      addToProgramAddressAndCycles(3, 6);
    } else {
      addToProgramAddressAndCycles(3, 8);
    }
    break;
  }
  case (0x26): // ROL Direct Page
  {
    executeMemoryROL(opCode);
    int opCycles = Binary::lower8BitsOf(dp_) != 0 ? 1 : 0;
    if (accumulatorIs8BitWide()) {
      addToProgramAddressAndCycles(2, 5 + opCycles);
    } else {
      addToProgramAddressAndCycles(2, 7 + opCycles);
    }
    break;
  }
  case (0x3E): // ROL Absolute Indexed, X
  {
    executeMemoryROL(opCode);
#ifdef EMU_65C02
    short opCycles = opCodeAddressingCrossesPageBoundary(opCode) ? 0 : -1;
#else
    short opCycles = 0;
#endif
    if (accumulatorIs8BitWide()) {
      addToProgramAddressAndCycles(3, 7 + opCycles);
    } else {
      addToProgramAddressAndCycles(3, 9 + opCycles);
    }
    break;
  }
  case (0x36): // ROL Direct Page Indexed, X
  {
    executeMemoryROL(opCode);
    int opCycles = Binary::lower8BitsOf(dp_) != 0 ? 1 : 0;
    if (accumulatorIs8BitWide()) {
      addToProgramAddressAndCycles(2, 6 + opCycles);
    } else {
      addToProgramAddressAndCycles(2, 8 + opCycles);
    }
    break;
  }
  default: {
    LOG_UNEXPECTED_OPCODE(opCode);
  }
  }
}
