#include "bus/opl_2.h"


OPL2::OPL2() :
opl_left_(std::make_unique<CEmuopl>(44100, true, false)),
opl_right_(std::make_unique<CEmuopl>(44100, true, false))
{

}

void OPL2::StoreByte(const Address &addr, uint8_t v, uint8_t **address) {
  if (addr.InRange(kOplLeftBase, kOplLeftTop)) {
    opl_left_->write(addr.offset_ - kOplLeftBase.offset_, v);
    return;
  }
  if (addr.InRange(kOplRightBase, kOplRightTop)) {
    opl_right_->write(addr.offset_ - kOplLeftBase.offset_, v);
    return;
  }
  // TODO: 'both', what is?
}

uint8_t OPL2::ReadByte(const Address &addr, uint8_t **address) {
  return 0;
}

bool OPL2::DecodeAddress(const Address &from_addr, Address &to_addr) {
  if (from_addr.InRange(kOplLeftBase, kOplBothTop)) {
    to_addr = from_addr;
    return true;
  }
  return false;
}

void OPL2::Start() {
  opl_left_->init();
  opl_right_->init();

  // TODO hook into audio buffer mixer // SDL
}
