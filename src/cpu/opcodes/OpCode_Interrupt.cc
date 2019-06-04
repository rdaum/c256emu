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
#include "cpu/interrupt.h"



/**
 * This file contains the implementation for all OpCodes
 * that deal with interrupts.
 */

void Cpu65816::executeInterrupt(OpCode& opCode) {
  switch (opCode.code()) {
    case (0x00):  // BRK
    {
      if (cpu_status_.emulation_flag) {
        stack_.Push16Bit(static_cast<uint16_t>(program_address_.offset_ + 2));
        cpu_status_.break_flag = true;
        stack_.Push8Bit(cpu_status_.register_value());
        cpu_status_.interrupt_disable_flag = 1;
#ifdef EMU_65C02
        cpu_status_.decimal_flag = 0;
#endif
        program_address_ = Address(0x00, emulation_mode_interrupts_->brkIrq);
        total_cycles_counter_ += 7;
      } else {
        stack_.Push8Bit(program_address_.bank_);
        stack_.Push16Bit(static_cast<uint16_t>(program_address_.offset_ + 2));
        stack_.Push8Bit(cpu_status_.register_value());
        cpu_status_.interrupt_disable_flag = 1;
        cpu_status_.decimal_flag = 0;
        Address newAddress(0x00, native_mode_interrupts_->brk);
        program_address_ = newAddress;
        total_cycles_counter_ += 8;
      }
      break;
    }
    case (0x02):  // COP
    {
      if (cpu_status_.emulation_flag) {
        stack_.Push16Bit(static_cast<uint16_t>(program_address_.offset_ + 2));
        stack_.Push8Bit(cpu_status_.register_value());
        cpu_status_.interrupt_disable_flag = 1;
        program_address_=
            Address(0x00, emulation_mode_interrupts_->coProcessorEnable);
        total_cycles_counter_ += 7;
      } else {
        stack_.Push8Bit(program_address_.bank_);
        stack_.Push16Bit(static_cast<uint16_t>(program_address_.offset_ + 2));
        stack_.Push8Bit(cpu_status_.register_value());
        cpu_status_.interrupt_disable_flag = 1;
        program_address_ = Address(0x00, native_mode_interrupts_->coProcessorEnable);
        total_cycles_counter_ += 8;
      }
      cpu_status_.decimal_flag = 0;
      break;
    }
    case (0x40):  // RTI
    {
      // Note: The picture in the 65816 programming manual about this looks
      // wrong. This implementation follows the text instead.
      cpu_status_.setRegisterValue(stack_.Pull8Bit());

      if (cpu_status_.emulation_flag) {
        Address newProgramAddress(program_address_.bank_, stack_.Pull16Bit());
        program_address_ = newProgramAddress;
        total_cycles_counter_ += 6;
      } else {
        uint16_t offset = stack_.Pull16Bit();
        uint8_t bank = stack_.Pull8Bit();
        Address newProgramAddress(bank, offset);
        program_address_ = newProgramAddress;
        total_cycles_counter_ += 7;
      }
      break;
    }
    default: { LOG_UNEXPECTED_OPCODE(opCode); }
  }
}
