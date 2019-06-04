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

#ifndef CPUSTATUS_H
#define CPUSTATUS_H

#include <cstdint>

struct CpuStatus {
  uint8_t register_value() const;
  void resetPRegister(uint8_t value);
  void setPRegister(uint8_t value);
  void setRegisterValue(uint8_t);

  void updateZeroFlagFrom8BitValue(uint8_t);
  void updateZeroFlagFrom16BitValue(uint16_t);
  void updateSignFlagFrom8BitValue(uint8_t);
  void updateSignFlagFrom16BitValue(uint16_t);
  void updateSignAndZeroFlagFrom8BitValue(uint8_t);
  void updateSignAndZeroFlagFrom16BitValue(uint16_t);

  bool carry_flag = false;
  bool zero_flag = false;
  bool interrupt_disable_flag = false;
  bool decimal_flag = false;
  bool break_flag = false;
  bool accumulator_width_flag = false;
  bool index_width_flag = false;
  bool emulation_flag = true;  // CPU Starts in emulation mode
  bool overflow_flag = false;
  bool sign_flag = false;
};

#endif  // CPUSTATUS_H
