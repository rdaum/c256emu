#include "bus/int_controller.h"

#include "bus/system.h"
#include "int_controller.h"

namespace {

constexpr Address kIntPendingReg0(0x00, 0x0140);
constexpr Address kIntPendingReg1(0x00, 0x0141);
constexpr Address kIntPendingReg2(0x00, 0x0142);

constexpr Address kIntPolReg0(0x00, 0x0144);
constexpr Address kIntPolReg1(0x00, 0x0145);
constexpr Address kIntPolReg2(0x00, 0x0146);

constexpr Address kIntEdgeReg0(0x00, 0x0147);
constexpr Address kIntEdgeReg1(0x00, 0x0148);
constexpr Address kIntEdgeReg2(0x00, 0x0149);

constexpr Address kIntMaskReg0(0x00, 0x014C);
constexpr Address kIntMaskReg1(0x00, 0x014D);
constexpr Address kIntMaskReg2(0x00, 0x014E);

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

  pending_reg0_.ints.UNUSED = false;
  polarity_reg0_.ints.UNUSED = false;
  edge_reg0_.ints.UNUSED = false;
  mask_reg0_.ints.UNUSED = false;

  pending_reg2_.ints.UNUSED0 = false;
  pending_reg2_.ints.UNUSED1 = false;
  polarity_reg2_.ints.UNUSED0 = false;
  polarity_reg2_.ints.UNUSED1 = false;
  edge_reg2_.ints.UNUSED0 = false;
  edge_reg2_.ints.UNUSED1 = false;
  mask_reg2_.ints.UNUSED0 = false;
  mask_reg2_.ints.UNUSED1 = false;
}

void InterruptController::RaiseFrameStart() {
  if (!pending_reg0_.ints.vicky0) {
    pending_reg0_.ints.vicky0 = true;
    sys_->RaiseIRQ();
  }
}

void InterruptController::RaiseKeyboard() {
  if (!pending_reg1_.ints.kbd) {
    pending_reg1_.ints.kbd = true;
    sys_->RaiseIRQ();
  }
}

void InterruptController::LowerKeyboard() {
  pending_reg1_.ints.kbd = false;
  if (!pending_reg1_.ints.Pending()) {
    sys_->ClearIRQ();
  }
}

void InterruptController::RaiseCH376() {
  if (!pending_reg1_.ints.ch376) {
    pending_reg1_.ints.ch376 = true;
    //    sys_->RaiseIRQ(); // For some reason this causes issues, and seems
    //    unnecessary.
  }
}

void InterruptController::LowerCH376() {
  pending_reg1_.ints.ch376 = false;
  if (!pending_reg1_.ints.Pending()) {
    sys_->ClearIRQ();
  }
}

bool InterruptController::DecodeAddress(const Address &from_addr,
                                        Address &to_addr) {
  if (from_addr.InRange(kIntPendingReg0, kIntMaskReg2)) {
    to_addr = from_addr;
    return true;
  }
  return false;
}

void InterruptController::StoreByte(const Address &addr, uint8_t v,
                                    uint8_t **address) {
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
  if (!pending_reg0_.ints.Pending() && !pending_reg1_.ints.Pending() &&
      !pending_reg2_.ints.Pending()) {
    sys_->ClearIRQ();
  }
}

uint8_t InterruptController::ReadByte(const Address &addr, uint8_t **address) {
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
