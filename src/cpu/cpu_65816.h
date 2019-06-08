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

#ifndef __CPU_65816__
#define __CPU_65816__

#include <atomic>
#include <cstdint>

#include <glog/logging.h>

#include "cpu/addressing.h"
#include "cpu/binary.h"
#include "cpu/cpu_status.h"
#include "cpu/interrupt.h"
#include "cpu/opcode.h"
#include "cpu/stack.h"
#include "cpu/system_bus.h"

// Macro used by OpCode methods when an unrecognized OpCode is being executed.
#define LOG_UNEXPECTED_OPCODE(opCode)                                          \
  LOG(ERROR) << "Unexpected OpCode: " << opCode.name();

class Cpu65816Debugger;

class Cpu65816 {
  friend class Cpu65816Debugger;

public:
  Cpu65816(SystemBus &, EmulationModeInterrupts *, NativeModeInterrupts *);

  void SetRESPin(bool value);
  void SetRDYPin(bool value);

  void SetIRQPin(bool value) { pins_.IRQ = value; }
  void SetNMIPin(bool value) { pins_.NMI = value; }
  void SetABORTPin(bool value) { pins_.ABORT = value; }

  bool ExecuteNextInstruction();

  void Jump(const Address &address);

  Address program_address() const;
  const Stack *stack() const;
  const CpuStatus *cpu_status() const;

  inline uint64_t total_cycles_counter() const { return total_cycles_counter_; }

  uint16_t a() const { return a_; }
  uint16_t x() const { return x_; }
  uint16_t y() const { return y_; }

  void set_trace_log(bool trace_log) { trace_log_ = trace_log; }
private:
  bool accumulatorIs8BitWide() const;
  bool accumulatorIs16BitWide() const;
  bool indexIs8BitWide() const;
  bool indexIs16BitWide() const;

  uint16_t indexWithXRegister() const;
  uint16_t indexWithYRegister() const;

  Address getAddressOfOpCodeData(OpCode &) const;
  bool opCodeAddressingCrossesPageBoundary(OpCode &) const;

  void addToProgramAddressAndCycles(int, int);

  // OpCode Table.
  static OpCode OP_CODE_TABLE[];

  // OpCodes handling routines.
  // Implementations for these methods can be found in the corresponding
  // OpCode_XXX.cpp file.
  void executeORA(OpCode &);
  void executeORA8Bit(OpCode &);
  void executeORA16Bit(OpCode &);
  void executeStack(OpCode &);
  void executeStatusReg(OpCode &);
  void executeMemoryROL(OpCode &);
  void executeAccumulatorROL();
  void executeROL(OpCode &);
  void executeMemoryROR(OpCode &);
  void executeAccumulatorROR();
  void executeROR(OpCode &);
  void executeInterrupt(OpCode &);
  void executeJumpReturn(OpCode &);
  void execute8BitSBC(OpCode &);
  void execute16BitSBC(OpCode &);
  void execute8BitBCDSBC(OpCode &);
  void execute16BitBCDSBC(OpCode &);
  void executeSBC(OpCode &);
  void execute8BitADC(OpCode &);
  void execute16BitADC(OpCode &);
  void execute8BitBCDADC(OpCode &);
  void execute16BitBCDADC(OpCode &);
  void executeADC(OpCode &);
  void executeSTA(OpCode &);
  void executeSTX(OpCode &);
  void executeSTY(OpCode &);
  void executeSTZ(OpCode &);
  void executeTransfer(OpCode &);
  void executeMemoryASL(OpCode &);
  void executeAccumulatorASL();
  void executeASL(OpCode &);
  void executeAND8Bit(OpCode &);
  void executeAND16Bit(OpCode &);
  void executeAND(OpCode &);
  void executeLDA8Bit(OpCode &);
  void executeLDA16Bit(OpCode &);
  void executeLDA(OpCode &);
  void executeLDX8Bit(OpCode &);
  void executeLDX16Bit(OpCode &);
  void executeLDX(OpCode &);
  void executeLDY8Bit(OpCode &);
  void executeLDY16Bit(OpCode &);
  void executeLDY(OpCode &);
  void executeEOR8Bit(OpCode &);
  void executeEOR16Bit(OpCode &);
  void executeEOR(OpCode &);
  int executeBranchShortOnCondition(bool, OpCode &);
  int executeBranchLongOnCondition(bool, OpCode &);
  void executeBranch(OpCode &);
  void execute8BitCMP(OpCode &);
  void execute16BitCMP(OpCode &);
  void executeCMP(OpCode &);
  void execute8BitDecInMemory(OpCode &);
  void execute16BitDecInMemory(OpCode &);
  void execute8BitIncInMemory(OpCode &);
  void execute16BitIncInMemory(OpCode &);
  void executeINCDEC(OpCode &);
  void execute8BitCPX(OpCode &);
  void execute16BitCPX(OpCode &);
  void execute8BitCPY(OpCode &);
  void execute16BitCPY(OpCode &);
  void executeCPXCPY(OpCode &);
  void execute8BitTSB(OpCode &);
  void execute16BitTSB(OpCode &);
  void execute8BitTRB(OpCode &);
  void execute16BitTRB(OpCode &);
  void executeTSBTRB(OpCode &);
  void execute8BitBIT(OpCode &);
  void execute16BitBIT(OpCode &);
  void executeBIT(OpCode &);
  void executeMemoryLSR(OpCode &);
  void executeAccumulatorLSR();
  void executeLSR(OpCode &);
  void executeMisc(OpCode &);

  void reset();

  SystemBus &system_bus_;
  EmulationModeInterrupts *emulation_mode_interrupts_;
  NativeModeInterrupts *native_mode_interrupts_;

  // Accumulator register
  uint16_t a_ = 0;
  // X index register
  uint16_t x_ = 0;
  // Y index register
  uint16_t y_ = 0;
  // Status register
  CpuStatus cpu_status_;
  // Data bank register
  uint8_t db_ = 0;
  // Direct page register
  uint16_t dp_ = 0;

  struct {
    // Reset to true means low power mode (do nothing) (should jump indirect via
    // 0x00FFFC)
    bool RES = true;
    // Ready to false means CPU is waiting for an NMI/IRQ/ABORT/RESET
    bool RDY = false;

    // nmi true execute nmi vector (0x00FFEA)
    bool NMI = false;
    // irq true exucute irq vector (0x00FFEE)
    bool IRQ = false;
    // abort true execute abort vector (0x00FFE8)
    bool ABORT = false;

  } pins_;

  Stack stack_;

  // Address of the current OpCode
  Address program_address_{0x00, 0x0000};

  // Total number of cycles
  std::atomic<uint64_t> total_cycles_counter_ = 0;

  bool trace_log_ = false;
};

#endif
