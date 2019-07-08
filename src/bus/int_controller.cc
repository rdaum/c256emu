#include "bus/int_controller.h"

#include "int_controller.h"
#include "system.h"

namespace {

constexpr uint32_t kIntPendingReg0 = 0x0140;
constexpr uint32_t kIntPendingReg1 = 0x0141;
constexpr uint32_t kIntPendingReg2 = 0x0142;

constexpr uint32_t kIntPolReg0 = 0x0144;
constexpr uint32_t kIntPolReg1 = 0x0145;
constexpr uint32_t kIntPolReg2 = 0x0146;

constexpr uint32_t kIntEdgeReg0 = 0x0147;
constexpr uint32_t kIntEdgeReg1 = 0x0148;
constexpr uint32_t kIntEdgeReg2 = 0x0149;

constexpr uint32_t kIntMaskReg0 = 0x014C;
constexpr uint32_t kIntMaskReg1 = 0x014D;
constexpr uint32_t kIntMaskReg2 = 0x014E;

} // namespace

InterruptController::InterruptController(System *sys) : sys_(sys) {
  pending_reg0_.val = 0;
  pending_reg1_.val = 0;
  pending_reg2_.val = 0;
  polarity_reg0_.val = 0;
  polarity_reg1_.val = 0;
  polarity_reg2_.val = 0;
  edge_reg0_.val = 0;
  edge_reg1_.val = 0;
  edge_reg2_.val = 0;
  mask_reg0_.val = 0;
  mask_reg1_.val = 0;
  mask_reg2_.val = 0;

  pending_reg2_.ints.UNUSED0 = false;
  pending_reg2_.ints.UNUSED1 = false;
  polarity_reg2_.ints.UNUSED0 = false;
  polarity_reg2_.ints.UNUSED1 = false;
  edge_reg2_.ints.UNUSED0 = false;
  edge_reg2_.ints.UNUSED1 = false;
  mask_reg2_.ints.UNUSED0 = false;
  mask_reg2_.ints.UNUSED1 = false;
}

bool InterruptController::AnyPending() const {
  return pending_reg1_.ints.Pending() || pending_reg2_.ints.Pending() ||
         pending_reg0_.ints.Pending();
}

void InterruptController::SetFrameStart(bool level) {
  bool old_pending = AnyPending();
  pending_reg0_.ints.vicky0 = level;
  if (level && !old_pending) sys_->RaiseIRQ();
  else if (!AnyPending() && old_pending) sys_->ClearIRQ();
}

void InterruptController::SetKeyboard(bool level) {
  bool old_pending = AnyPending();
  pending_reg1_.ints.kbd = level;
  if (level && !old_pending) sys_->RaiseIRQ();
  else if (!AnyPending() && old_pending) sys_->ClearIRQ();
}

void InterruptController::SetMouse(bool level) {
  bool old_pending = AnyPending();
  pending_reg0_.ints.mouse = level;
  if (level && !old_pending) sys_->RaiseIRQ();
  else if (!AnyPending() && old_pending) sys_->ClearIRQ();
}


void InterruptController::SetCH376(bool level) {
  // TODO: fix the fact that the CH376 emulation never seems to drop the irq
//  bool old_pending = AnyPending();
  pending_reg1_.ints.ch376 = level;
//  if (level && !old_pending) sys_->RaiseIRQ();
//  else if (!AnyPending() && old_pending) sys_->ClearIRQ();
}

void InterruptController::SetVDMATransfer(bool level) {
  bool old_pending = AnyPending();
  pending_reg2_.ints.gavin_dma = level;
  if (level && !old_pending) sys_->RaiseIRQ();
  else if (!AnyPending() && old_pending) sys_->ClearIRQ();
}

void InterruptController::StoreByte(uint32_t addr, uint8_t v) {
  bool old_pending = AnyPending();
  if (addr == kIntPendingReg0) {
    pending_reg0_.val &= ~v;
  } else if (addr == kIntPendingReg1) {
    pending_reg1_.val &= ~v;
  } else if (addr == kIntPendingReg2) {
    pending_reg2_.val &= ~v;
  } else if (addr == kIntPolReg0) {
    polarity_reg0_.val &= ~v;
  } else if (addr == kIntPolReg1) {
    polarity_reg1_.val &= ~v;
  } else if (addr == kIntPolReg2) {
    polarity_reg2_.val &= ~v;
  } else if (addr == kIntEdgeReg0) {
    edge_reg0_.val &= ~v;
  } else if (addr == kIntEdgeReg1) {
    edge_reg1_.val &= ~v;
  } else if (addr == kIntEdgeReg2) {
    edge_reg2_.val &= ~v;
  } else if (addr == kIntMaskReg0) {
    mask_reg0_.val &= ~v;
  } else if (addr == kIntMaskReg1) {
    mask_reg1_.val &= ~v;
  } else if (addr == kIntMaskReg2) {
    mask_reg2_.val &= ~v;
  }
  if (old_pending && !AnyPending()) {
    sys_->ClearIRQ();
  }
}

uint8_t InterruptController::ReadByte(uint32_t addr) {
  if (addr == kIntPendingReg0) {
    return pending_reg0_.val;
  } else if (addr == kIntPendingReg1) {
    return pending_reg1_.val;
  } else if (addr == kIntPendingReg2) {
    return pending_reg2_.val;
  } else if (addr == kIntPolReg0) {
    return polarity_reg0_.val;
  } else if (addr == kIntPolReg1) {
    return polarity_reg1_.val;
  } else if (addr == kIntPolReg2) {
    return polarity_reg2_.val;
  } else if (addr == kIntEdgeReg0) {
    return edge_reg0_.val;
  } else if (addr == kIntEdgeReg1) {
    return edge_reg1_.val;
  } else if (addr == kIntEdgeReg2) {
    return edge_reg2_.val;
  } else if (addr == kIntMaskReg0) {
    return mask_reg0_.val;
  } else if (addr == kIntMaskReg1) {
    return mask_reg1_.val;
  } else if (addr == kIntMaskReg2) {
    return mask_reg2_.val;
  }
  return 0;
}
