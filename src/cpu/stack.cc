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

#include <glog/logging.h>

#include "cpu/stack.h"

Stack::Stack(SystemBus* system_bus)
    : system_bus_(system_bus), stack_address_(0x00, STACK_POINTER_DEFAULT) {
  LOG(INFO) << "Initialized at default location " << stack_address_.offset_;
}

Stack::Stack(SystemBus* systemBus, uint16_t stack_pointer)
    : system_bus_(systemBus), stack_address_(0x00, stack_pointer) {
  LOG(INFO) << "Initialized at location " << stack_address_.offset_;
}

void Stack::Push8Bit(uint8_t value) {
  system_bus_->StoreByte(stack_address_, value);
  stack_address_.offset_ += sizeof(uint8_t);
}

void Stack::Push16Bit(uint16_t value) {
  system_bus_->StoreWord(stack_address_, value);
  stack_address_.offset_ += sizeof(uint16_t);
}

uint8_t Stack::Pull8Bit() {
  stack_address_.offset_ -= sizeof(uint8_t);
  return system_bus_->ReadByte(stack_address_);
}

uint16_t Stack::Pull16Bit() {
  stack_address_.offset_ -= sizeof(uint16_t);
  return system_bus_->ReadWord(stack_address_);
}

uint16_t Stack::stack_pointer() const {
  return stack_address_.offset_;
}
