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

#include "cpu/cpu_status.h"

#include "cpu/binary.h"
#include <glog/logging.h>

/* **********************************************
 *
 * Flags:
 *
 *  b flag: the break flag
 *  c flag: the carry flag
 *  d flag: the decimal mode flag
 *  e flag: the emulation mode flag
 *  i flag: the interrupt disable flag
 *  m flag: the accumulator and memory width flag
 *  n flag: the negative flag
 *  v flag: the overflow flag
 *  x flag: the index register width flag
 *  z flag: the zero flag
 *
 * **********************************************/

#define STATUS_CARRY 0x01
#define STATUS_ZERO 0x02
#define STATUS_INTERRUPT_DISABLE 0x04
#define STATUS_DECIMAL 0x08

// In emulation mode
#define STATUS_BREAK 0X10
// In native mode (x = 0, 16 bit)
#define STATUS_INDEX_WIDTH 0X10
// Only used in native mode
#define STATUS_ACCUMULATOR_WIDTH 0X20

#define STATUS_OVERFLOW 0X40
#define STATUS_SIGN 0X80

uint8_t CpuStatus::register_value() const {
  uint8_t value = 0;
  if (carry_flag)
    value |= STATUS_CARRY;
  if (zero_flag)
    value |= STATUS_ZERO;
  if (interrupt_disable_flag)
    value |= STATUS_INTERRUPT_DISABLE;
  if (decimal_flag)
    value |= STATUS_DECIMAL;
  if (emulation_flag && break_flag)
    value |= STATUS_BREAK;
  if (!emulation_flag && index_width_flag)
    value |= STATUS_INDEX_WIDTH;
  if (!emulation_flag && accumulator_width_flag)
    value |= STATUS_ACCUMULATOR_WIDTH;
  if (overflow_flag)
    value |= STATUS_OVERFLOW;
  if (sign_flag)
    value |= STATUS_SIGN;

  return value;
}

void CpuStatus::resetPRegister(uint8_t value) {
  if (value & STATUS_CARRY)
    carry_flag = false;

  if (value & STATUS_ZERO)
    zero_flag = false;

  if (value & STATUS_INTERRUPT_DISABLE)
    interrupt_disable_flag = 0;

  if (value & STATUS_DECIMAL)
    decimal_flag = false;

  if (emulation_flag) {
    if (value & STATUS_BREAK)
      break_flag = false;
  } else {
    if (value & STATUS_INDEX_WIDTH)
      index_width_flag = false;
  }

  if (!emulation_flag && (value & STATUS_ACCUMULATOR_WIDTH))
    accumulator_width_flag = false;

  if (value & STATUS_OVERFLOW)
    overflow_flag = false;

  if (value & STATUS_SIGN)
    sign_flag = false;
}

void CpuStatus::setPRegister(uint8_t value) {
  if (value & STATUS_CARRY)
    carry_flag = true;

  if (value & STATUS_ZERO)
    zero_flag = true;

  if (value & STATUS_INTERRUPT_DISABLE)
    interrupt_disable_flag = 1;

  if (value & STATUS_DECIMAL)
    decimal_flag = true;

  if (emulation_flag) {
    if (value & STATUS_BREAK)
      break_flag = true;
  } else {
    if (value & STATUS_INDEX_WIDTH)
      index_width_flag = true;
  }

  if (!emulation_flag && (value & STATUS_ACCUMULATOR_WIDTH))
    accumulator_width_flag = true;

  if (value & STATUS_OVERFLOW)
    overflow_flag = true;

  if (value & STATUS_SIGN)
    sign_flag = true;
}

void CpuStatus::setRegisterValue(uint8_t value) {
  carry_flag = (value & STATUS_CARRY) != 0;
  zero_flag = (value & STATUS_ZERO) != 0;
  interrupt_disable_flag = (value & STATUS_INTERRUPT_DISABLE) != 0;
  decimal_flag = (value & STATUS_DECIMAL) != 0;

  if (emulation_flag) {
    break_flag = (value & STATUS_BREAK) != 0;
  } else {
    index_width_flag = (value & STATUS_INDEX_WIDTH) != 0;
  }

  accumulator_width_flag =
      !emulation_flag && (value & STATUS_ACCUMULATOR_WIDTH);

  overflow_flag = (value & STATUS_OVERFLOW) != 0;
  sign_flag = (value & STATUS_SIGN) != 0;
}

void CpuStatus::updateZeroFlagFrom8BitValue(uint8_t value) {
  if (Binary::is8bitValueZero(value))
    zero_flag = true;
  else
    zero_flag = 0;
}

void CpuStatus::updateZeroFlagFrom16BitValue(uint16_t value) {
  if (Binary::is16bitValueZero(value))
    zero_flag = true;
  else
    zero_flag = 0;
}

void CpuStatus::updateSignFlagFrom8BitValue(uint8_t value) {
  if (Binary::is8bitValueNegative(value))
    sign_flag = true;
  else
    sign_flag = false;
}

void CpuStatus::updateSignFlagFrom16BitValue(uint16_t value) {
  if (Binary::is16bitValueNegative(value))
    sign_flag = true;
  else
    sign_flag = false;
}

void CpuStatus::updateSignAndZeroFlagFrom8BitValue(uint8_t value) {
  updateSignFlagFrom8BitValue(value);
  updateZeroFlagFrom8BitValue(value);
}

void CpuStatus::updateSignAndZeroFlagFrom16BitValue(uint16_t value) {
  updateSignFlagFrom16BitValue(value);
  updateZeroFlagFrom16BitValue(value);
}
