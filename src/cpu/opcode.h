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

#ifndef OPCODE_HPP
#define OPCODE_HPP

#include <stdint.h>

#include "cpu/addressing.h"

class Cpu65816;

class OpCode {
 public:
  OpCode(uint8_t code,
         const char* const name,
         const AddressingMode& addressingMode)
      : code_(code),
        name_(name),
        addressing_mode_(addressingMode),
        executor_(0) {}

  OpCode(uint8_t code,
         const char* const name,
         const AddressingMode& addressingMode,
         void (Cpu65816::*executor)(OpCode&))
      : code_(code),
        name_(name),
        addressing_mode_(addressingMode),
        executor_(executor) {}

  uint8_t code() const { return code_; }

  const char* name() const { return name_; }

  AddressingMode addressing_mode() const { return addressing_mode_; }

  bool execute(Cpu65816& cpu) {
    if (executor_ != 0) {
      OpCode opCode = *this;
      (cpu.*executor_)(opCode);
      return true;
    }
    return false;
  }

private:
  uint8_t code_;
  const char* const name_;
  AddressingMode addressing_mode_;
  void (Cpu65816::*executor_)(OpCode&);
};

#endif  // OPCODE_HPP
