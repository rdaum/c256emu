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
 * This file contains the implementation for all ADC OpCodes.
 */

void Cpu65816::execute8BitADC(OpCode &opCode) {
  Address dataAddress = getAddressOfOpCodeData(opCode);
  uint8_t value = system_bus_.ReadByte(dataAddress);
  uint8_t accumulator = Binary::lower8BitsOf(a_);
  uint8_t carryValue = cpu_status_.carry_flag ? 1 : 0;

  uint16_t result16Bit = accumulator + value + carryValue;

  // Is there a carry out of the penultimate bit, redo the sum with 7 bits value
  // and find out.
  accumulator &= 0x7F;
  value &= 0x7F;
  uint8_t partialResult = accumulator + value + carryValue;
  // Is bit 8 set?
  bool carryOutOfPenultimateBit = partialResult & 0x80;

  // Is there a carry out of the last bit, check bit 8 for that
  bool carryOutOfLastBit = result16Bit & 0x0100;

  bool overflow = carryOutOfLastBit ^ carryOutOfPenultimateBit;
  if (overflow)
    cpu_status_.overflow_flag = true;
  else
    cpu_status_.overflow_flag = false;

  if (carryOutOfLastBit)
    cpu_status_.carry_flag = 1;
  else
    cpu_status_.carry_flag = 0;

  uint8_t result8Bit = Binary::lower8BitsOf(result16Bit);
  // Update sign and zero flags
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(result8Bit);
  // Store the 8 bit result in the accumulator
  Binary::setLower8BitsOf16BitsValue(&a_, result8Bit);
}

void Cpu65816::execute16BitADC(OpCode &opCode) {
  Address dataAddress = getAddressOfOpCodeData(opCode);
  uint16_t value = system_bus_.ReadWord(dataAddress);
  uint16_t accumulator = a_;
  uint16_t carryValue = cpu_status_.carry_flag ? 1 : 0;

  uint32_t result32Bit = accumulator + value + carryValue;

  // Is there a carry out of the penultimate bit, redo the sum with 15 bits
  // value and find out.
  accumulator &= 0x7FFF;
  value &= 0x7FFF;
  uint16_t partialResult = accumulator + value + carryValue;
  // Is bit 8 set?
  bool carryOutOfPenultimateBit = partialResult & 0x8000;

  // Is there a carry out of the last bit, check bit 16 for that
  bool carryOutOfLastBit = result32Bit & 0x010000;

  bool overflow = carryOutOfLastBit ^ carryOutOfPenultimateBit;
  if (overflow)
    cpu_status_.overflow_flag = true;
  else
    cpu_status_.overflow_flag = false;

  if (carryOutOfLastBit)
    cpu_status_.carry_flag = 1;
  else
    cpu_status_.carry_flag = 0;

  uint16_t result16Bit = Binary::lower16BitsOf(result32Bit);
  // Update sign and zero flags
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(result16Bit);
  // Store the 16 bit result in the accumulator
  a_ = result16Bit;
}

void Cpu65816::execute8BitBCDADC(OpCode &opCode) {
  Address dataAddress = getAddressOfOpCodeData(opCode);
  uint8_t value = system_bus_.ReadByte(dataAddress);
  uint8_t accumulator = Binary::lower8BitsOf(a_);

  uint8_t result = 0;
  bool carry =
      Binary::bcdSum8Bit(value, accumulator, &result, cpu_status_.carry_flag);
  if (carry)
    cpu_status_.carry_flag = 1;
  else
    cpu_status_.carry_flag = 0;

  Binary::setLower8BitsOf16BitsValue(&a_, result);
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(result);
}

void Cpu65816::execute16BitBCDADC(OpCode &opCode) {
  Address dataAddress = getAddressOfOpCodeData(opCode);
  uint16_t value = system_bus_.ReadWord(dataAddress);
  uint16_t accumulator = a_;

  uint16_t result = 0;
  bool carry =
      Binary::bcdSum16Bit(value, accumulator, &result, cpu_status_.carry_flag);
  if (carry)
    cpu_status_.carry_flag = 1;
  else
    cpu_status_.carry_flag = 0;

  a_ = result;
  cpu_status_.updateSignAndZeroFlagFrom8BitValue(result);
}

void Cpu65816::executeADC(OpCode &opCode) {
  if (accumulatorIs8BitWide()) {
    if (cpu_status_.decimal_flag)
      execute8BitBCDADC(opCode);
    else
      execute8BitADC(opCode);
  } else {
    if (cpu_status_.decimal_flag)
      execute16BitBCDADC(opCode);
    else
      execute16BitADC(opCode);
    total_cycles_counter_ += 1;
  }

// All OpCodes take one more cycle on 65C02 in decimal mode
#ifdef EMU_65C02
  if (cpu_status_.decimal_flag) {
    total_cycles_counter_ += 1;
  }
#endif

  switch (opCode.code()) {
  case (0x69): // ADC Immediate
  {
    if (accumulatorIs16BitWide()) {
      program_address_.offset_ += 1;
    }
    program_address_.offset_ += 2;
    total_cycles_counter_ += 2;
    break;
  }
  case (0x6D): // ADC Absolute
  {
    program_address_.offset_ += 3;
    total_cycles_counter_ += 4;
    break;
  }
  case (0x6F): // ADC Absolute Long
  {
    program_address_.offset_ += 4;
    total_cycles_counter_ += 5;
    break;
  }
  case (0x65): // ADC Direct Page
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 3;
    break;
  }
  case (0x72): // ADC Direct Page Indirect
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 5;
    break;
  }
  case (0x67): // ADC Direct Page Indirect Long
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 2;
    total_cycles_counter_ += 6;
    break;
  }
  case (0x7D): // ADC Absolute Indexed, X
  {
    if (opCodeAddressingCrossesPageBoundary(opCode)) {
      total_cycles_counter_ += 1;
    }

    program_address_.offset_ += 3;
    total_cycles_counter_ += 4;
    break;
  }
  case (0x7F): // ADC Absolute Long Indexed, X
  {
    program_address_.offset_ += 4;
    total_cycles_counter_ += 5;
    break;
  }
  case (0x79): // ADC Absolute Indexed Y
  {
    if (opCodeAddressingCrossesPageBoundary(opCode)) {
      total_cycles_counter_ += 1;
    }
    program_address_.offset_ += 3;
    total_cycles_counter_ += 4;
    break;
  }
  case (0x75): // ADC Direct Page Indexed, X
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    program_address_.offset_ += 2;
    total_cycles_counter_ += 4;
    break;
  }
  case (0x61): // ADC Direct Page Indexed Indirect, X
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    program_address_.offset_ += 2;
    total_cycles_counter_ += 6;
    break;
  }
  case (0x71): // ADC Direct Page Indirect Indexed, Y
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    if (opCodeAddressingCrossesPageBoundary(opCode)) {
      total_cycles_counter_ += 1;
    }
    program_address_.offset_ += 2;
    total_cycles_counter_ += 5;
    break;
  }
  case (0x77): // ADC Direct Page Indirect Long Indexed, Y
  {
    if (Binary::lower8BitsOf(dp_) != 0) {
      total_cycles_counter_ += 1;
    }
    program_address_.offset_ += 2;
    total_cycles_counter_ += 6;
    break;
  }
  case (0x63): // ADC Stack Relative
  {
    program_address_.offset_ += 2;
    total_cycles_counter_ += 4;
    break;
  }
  case (0x73): // ADC Stack Relative Indirect Indexed, Y
  {
    program_address_.offset_ += 2;
    total_cycles_counter_ += 7;
    break;
  }
  default: {
    LOG_UNEXPECTED_OPCODE(opCode);
  }
  }
}
