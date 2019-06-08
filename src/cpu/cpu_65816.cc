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
#include "cpu_65816.h"

#include <cmath>
#include <iomanip>

Cpu65816::Cpu65816(SystemBus &systemBus,
                   EmulationModeInterrupts *emulationInterrupts,
                   NativeModeInterrupts *nativeInterrupts)
    : system_bus_(systemBus), emulation_mode_interrupts_(emulationInterrupts),
      native_mode_interrupts_(nativeInterrupts), stack_(&system_bus_) {}

Address Cpu65816::program_address() const { return program_address_; }

const Stack *Cpu65816::stack() const { return &stack_; }

const CpuStatus *Cpu65816::cpu_status() const { return &cpu_status_; }

/**
 * Resets the cpu to its initial state.
 * */
void Cpu65816::reset() {
  SetRESPin(true);
  cpu_status_.emulation_flag = true;
  cpu_status_.accumulator_width_flag = true;
  cpu_status_.index_width_flag = true;
  x_ &= 0xFF;
  y_ &= 0xFF;
  dp_ = 0x0;
  stack_ = Stack(&system_bus_);
  program_address_ = Address(0, system_bus_.ReadWord(Address(
                                    0x00, emulation_mode_interrupts_->reset)));
}

void Cpu65816::SetRESPin(bool value) {
  if (value == false && pins_.RES == true) {
    reset();
  }
  pins_.RES = value;
}

void Cpu65816::SetRDYPin(bool value) { pins_.RDY = value; }

bool Cpu65816::ExecuteNextInstruction() {
  if (pins_.RES) {
    return false;
  }
  if ((pins_.IRQ) && (!cpu_status_.interrupt_disable_flag)) {
    /*
    The program bank register (PB, the A16-A23 part of the address bus) is
    pushed onto the hardware stack (65C816/65C802 only when operating in native
    mode). The most significant byte (MSB) of the program counter (PC) is pushed
    onto the stack. The least significant byte (LSB) of the program counter is
    pushed onto the stack. The status register (SR) is pushed onto the stack.
    The interrupt disable flag is set in the status register.
    PB is loaded with $00 (65C816/65C802 only when operating in native mode).
    PC is loaded from the relevant vector (see tables).
    */
    if (!cpu_status_.emulation_flag) {
      stack_.Push8Bit(program_address_.bank_);
      stack_.Push16Bit(program_address_.offset_);
      stack_.Push8Bit(cpu_status_.register_value());
      cpu_status_.interrupt_disable_flag = 1;
      program_address_ =
          Address(0x00, system_bus_.ReadWord(Address(0x00, 0xFFEE)));
    } else {
      stack_.Push16Bit(program_address_.offset_);
      stack_.Push8Bit(cpu_status_.register_value());
      cpu_status_.interrupt_disable_flag = 1;
      program_address_ =
          Address(0x00, system_bus_.ReadWord(Address(0x00, 0xFFFE)));
    }
  }

  // Fetch the instruction
  const uint8_t instruction = system_bus_.ReadByte(program_address_);
  OpCode opCode = OP_CODE_TABLE[instruction];

  if (trace_log_) {
    LOG(INFO) << program_address_ << " :" << opCode.name() << " ("
              << "0x" << std::setfill('0') << std::setw(2) << std::hex
              << (int)opCode.code() << ") "
              << "stack@" << std::hex << stack_.stack_pointer() << ": "
              << stack_.Peek(8);
  }
  return opCode.execute(*this);
}

bool Cpu65816::accumulatorIs8BitWide() const {
  // Accumulator is always 8 bit in emulation mode.
  if (cpu_status_.emulation_flag)
    return true;
  // Accumulator width set to one means 8 bit accumulator.
  else
    return cpu_status_.accumulator_width_flag;
}

bool Cpu65816::accumulatorIs16BitWide() const {
  return !accumulatorIs8BitWide();
}

bool Cpu65816::indexIs8BitWide() const {
  // Index is always 8 bit in emulation mode.
  if (cpu_status_.emulation_flag)
    return true;
  // Index width set to one means 8 bit accumulator.
  else
    return cpu_status_.index_width_flag;
}

bool Cpu65816::indexIs16BitWide() const { return !indexIs8BitWide(); }

void Cpu65816::addToProgramAddressAndCycles(int bytes, int cycles) {
  total_cycles_counter_ += cycles;
  program_address_.offset_ += bytes;
}

uint16_t Cpu65816::indexWithXRegister() const {
  return indexIs8BitWide() ? Binary::lower8BitsOf(x_) : x_;
}

uint16_t Cpu65816::indexWithYRegister() const {
  return indexIs8BitWide() ? Binary::lower8BitsOf(y_) : y_;
}

void Cpu65816::Jump(const Address &address) {
  stack_.Push8Bit(program_address_.bank_);
  stack_.Push16Bit(program_address_.offset_);
  stack_.Push8Bit(cpu_status_.register_value());
  cpu_status_.emulation_flag = false;
  cpu_status_.accumulator_width_flag = false;
  cpu_status_.index_width_flag = true;
  program_address_ = address;
}
