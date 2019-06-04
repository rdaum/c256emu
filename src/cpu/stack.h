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

#ifndef __STACK__
#define __STACK__

#include <stdint.h>

#include "cpu/system_bus.h"

#define STACK_POINTER_DEFAULT 0x1FF

class Stack {
 public:
  explicit Stack(SystemBus* system_bus_);
  Stack(SystemBus*, uint16_t stack_pointer);

  void Push8Bit(uint8_t value);
  void Push16Bit(uint16_t value);

  uint8_t Pull8Bit();
  uint16_t Pull16Bit();

  uint16_t stack_pointer() const;

 private:
  SystemBus* system_bus_;
  Address stack_address_;
};

#endif
