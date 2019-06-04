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

#include "cpu/cpu_65816.h"
#include "cpu/opcode.h"
#include "cpu/system_bus_device.h"

bool Cpu65816::opCodeAddressingCrossesPageBoundary(OpCode &opCode) const {
  switch (opCode.addressing_mode()) {
  case AddressingMode::AbsoluteIndexedWithX: {
    Address initialAddress(
        db_, system_bus_.ReadWord(program_address_.WithOffset(1)));
    // TODO: figure out when to wrap around and when not to, it should not
    // matter in this case but it matters when fetching data
    Address finalAddress =
        Address::SumOffsetToAddress(initialAddress, indexWithXRegister());
    return Address::OffsetsAreOnDifferentPages(initialAddress.offset_,
                                               finalAddress.offset_);
  }
  case AddressingMode::AbsoluteIndexedWithY: {
    Address initialAddress(
        db_, system_bus_.ReadWord(program_address_.WithOffset(1)));
    // TODO: figure out when to wrap around and when not to, it should not
    // matter in this case but it matters when fetching data
    Address finalAddress =
        Address::SumOffsetToAddress(initialAddress, indexWithYRegister());
    return Address::OffsetsAreOnDifferentPages(initialAddress.offset_,
                                               finalAddress.offset_);
  }
  case AddressingMode::DirectPageIndirectIndexedWithY: {
    uint16_t firstStageOffset =
        dp_ + system_bus_.ReadByte(program_address_.WithOffset(1));
    Address firstStageAddress(0x00, firstStageOffset);
    uint16_t secondStageOffset = system_bus_.ReadWord(firstStageAddress);
    Address thirdStageAddress(db_, secondStageOffset);
    // TODO: figure out when to wrap around and when not to, it should not
    // matter in this case but it matters when fetching data
    Address finalAddress =
        Address::SumOffsetToAddress(thirdStageAddress, indexWithYRegister());
    return Address::OffsetsAreOnDifferentPages(thirdStageAddress.offset_,
                                               finalAddress.offset_);
  }
  default: {
    LOG(ERROR) << "Unsupported opCodeAddressingCrossesPageBoundary for "
                  "opCode: "
               << opCode.code();
  }
  }

  return false;
}

Address Cpu65816::getAddressOfOpCodeData(OpCode &opCode) const {
  uint8_t dataAddressBank = 0x0;
  uint16_t dataAddressOffset = 0x0000;

  switch (opCode.addressing_mode()) {
  case AddressingMode::Interrupt:
  case AddressingMode::Accumulator:
  case AddressingMode::Implied:
  case AddressingMode::StackImplied:
    // Not really used, doesn't make any sense since these opcodes do not have
    // operands
    return program_address_;
  case AddressingMode::Immediate:
  case AddressingMode::BlockMove:
    // Blockmove OpCodes have two bytes following them directly
  case AddressingMode::StackAbsolute:
    // Stack absolute is used to push values following the op code onto the
    // stack
  case AddressingMode::ProgramCounterRelative:
    // Program counter relative OpCodes such as all branch instructions have
    // an 8 bit operand following the op code
  case AddressingMode::ProgramCounterRelativeLong:
    // StackProgramCounterRelativeLong is only used by the PER OpCode, it has
    // 16 bit operand
  case AddressingMode::StackProgramCounterRelativeLong:
    program_address_.WithOffset(1).GetBankAndOffset(&dataAddressBank,
                                                    &dataAddressOffset);
    break;
  case AddressingMode::Absolute:
    dataAddressBank = db_;
    dataAddressOffset = system_bus_.ReadWord(program_address_.WithOffset(1));
    break;
  case AddressingMode::AbsoluteLong:
    system_bus_.ReadAddressAt(program_address_ + 1)
        .GetBankAndOffset(&dataAddressBank, &dataAddressOffset);
    break;
  case AddressingMode::AbsoluteIndirect: {
    dataAddressBank = program_address_.bank_;
    Address addressOfOffset(
        0x00, system_bus_.ReadWord(program_address_.WithOffset(1)));
    dataAddressOffset = system_bus_.ReadWord(addressOfOffset);
  } break;
  case AddressingMode::AbsoluteIndirectLong: {
    Address addressOfEffectiveAddress(
        0x00, system_bus_.ReadWord(program_address_.WithOffset(1)));
    system_bus_.ReadAddressAt(addressOfEffectiveAddress)
        .GetBankAndOffset(&dataAddressBank, &dataAddressOffset);
  } break;
  case AddressingMode::AbsoluteIndexedIndirectWithX: {
    Address firstStageAddress(
        program_address_.bank_,
        system_bus_.ReadWord(program_address_.WithOffset(1)));
    Address secondStageAddress =
        firstStageAddress.WithOffsetNoWrapAround(indexWithXRegister());
    dataAddressBank = program_address_.bank_;
    dataAddressOffset = system_bus_.ReadWord(secondStageAddress);
  } break;
  case AddressingMode::AbsoluteIndexedWithX: {
    Address firstStageAddress(
        db_, system_bus_.ReadWord(program_address_.WithOffset(1)));
    Address::SumOffsetToAddressNoWrapAround(firstStageAddress,
                                            indexWithXRegister())
        .GetBankAndOffset(&dataAddressBank, &dataAddressOffset);
    ;
  } break;
  case AddressingMode::AbsoluteLongIndexedWithX: {
    Address firstStageAddress =
        system_bus_.ReadAddressAt(program_address_.WithOffset(1));
    Address::SumOffsetToAddressNoWrapAround(firstStageAddress,
                                            indexWithXRegister())
        .GetBankAndOffset(&dataAddressBank, &dataAddressOffset);
    ;
  } break;
  case AddressingMode::AbsoluteIndexedWithY: {
    Address firstStageAddress(
        db_, system_bus_.ReadWord(program_address_.WithOffset(1)));
    Address::SumOffsetToAddressNoWrapAround(firstStageAddress,
                                            indexWithYRegister())
        .GetBankAndOffset(&dataAddressBank, &dataAddressOffset);
    ;
  } break;
  case AddressingMode::DirectPage: {
    // Direct page/Zero page always refers to bank zero
    dataAddressBank = 0x00;
    if (cpu_status_.emulation_flag) {
      // 6502 uses zero page
      dataAddressOffset = system_bus_.ReadByte(program_address_.WithOffset(1));
    } else {
      // 65816 uses direct page
      dataAddressOffset =
          dp_ + system_bus_.ReadByte(program_address_.WithOffset(1));
    }
  } break;
  case AddressingMode::DirectPageIndexedWithX: {
    dataAddressBank = 0x00;
    dataAddressOffset = dp_ + indexWithXRegister() +
                        system_bus_.ReadByte(program_address_.WithOffset(1));
  } break;
  case AddressingMode::DirectPageIndexedWithY: {
    dataAddressBank = 0x00;
    dataAddressOffset = dp_ + indexWithYRegister() +
                        system_bus_.ReadByte(program_address_.WithOffset(1));
  } break;
  case AddressingMode::DirectPageIndirect: {
    Address firstStageAddress(
        0x00, dp_ + system_bus_.ReadByte(program_address_.WithOffset(1)));
    dataAddressBank = db_;
    dataAddressOffset = system_bus_.ReadWord(firstStageAddress);
  } break;
  case AddressingMode::DirectPageIndirectLong: {
    Address firstStageAddress(
        0x00, dp_ + system_bus_.ReadByte(program_address_.WithOffset(1)));
    system_bus_.ReadAddressAt(firstStageAddress)
        .GetBankAndOffset(&dataAddressBank, &dataAddressOffset);
    ;
  } break;
  case AddressingMode::DirectPageIndexedIndirectWithX: {
    Address firstStageAddress(
        0x00, dp_ + system_bus_.ReadByte(program_address_.WithOffset(1)) +
                  indexWithXRegister());
    dataAddressBank = db_;
    dataAddressOffset = system_bus_.ReadWord(firstStageAddress);
  } break;
  case AddressingMode::DirectPageIndirectIndexedWithY: {
    Address firstStageAddress(
        0x00, dp_ + system_bus_.ReadByte(program_address_.WithOffset(1)));
    uint16_t secondStageOffset = system_bus_.ReadWord(firstStageAddress);
    Address thirdStageAddress(db_, secondStageOffset);
    Address::SumOffsetToAddressNoWrapAround(thirdStageAddress,
                                            indexWithYRegister())
        .GetBankAndOffset(&dataAddressBank, &dataAddressOffset);
  } break;
  case AddressingMode::DirectPageIndirectLongIndexedWithY: {
    Address firstStageAddress(
        0x00, dp_ + system_bus_.ReadByte(program_address_.WithOffset(1)));
    Address secondStageAddress = system_bus_.ReadAddressAt(firstStageAddress);
    Address::SumOffsetToAddressNoWrapAround(secondStageAddress,
                                            indexWithYRegister())
        .GetBankAndOffset(&dataAddressBank, &dataAddressOffset);
  } break;
  case AddressingMode::StackRelative: {
    dataAddressBank = 0x00;
    dataAddressOffset = stack_.stack_pointer() +
                        system_bus_.ReadByte(program_address_.WithOffset(1));
  } break;
  case AddressingMode::StackDirectPageIndirect: {
    dataAddressBank = 0x00;
    dataAddressOffset =
        dp_ + system_bus_.ReadByte(program_address_.WithOffset(1));
  } break;
  case AddressingMode::StackRelativeIndirectIndexedWithY: {
    Address firstStageAddress(
        0x00, stack_.stack_pointer() +
                  system_bus_.ReadByte(program_address_.WithOffset(1)));
    uint16_t secondStageOffset = system_bus_.ReadWord(firstStageAddress);
    Address thirdStageAddress(db_, secondStageOffset);
    Address::SumOffsetToAddressNoWrapAround(thirdStageAddress,
                                            indexWithYRegister())
        .GetBankAndOffset(&dataAddressBank, &dataAddressOffset);
  } break;
  }

  return Address(dataAddressBank, dataAddressOffset);
}
