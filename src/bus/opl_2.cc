#include "bus/opl_2.h"

OPL2::OPL2()
    : opl_left_(std::make_unique<CEmuopl>(44100, true, false)),
      opl_right_(std::make_unique<CEmuopl>(44100, true, false)) {}

void OPL2::StoreByte(uint32_t addr, uint8_t v) {
  if (addr >= kOplLeftBase && addr <= kOplLeftTop) {
    opl_left_->write(addr - kOplLeftBase, v);
    return;
  }
  if (addr >= kOplRightBase && addr <= kOplRightTop) {
    opl_right_->write(addr - kOplLeftBase, v);
    return;
  }
  // TODO: 'both', what is?
}

uint8_t OPL2::ReadByte(uint32_t addr) {
  return 0;
}

void OPL2::Start() {
  opl_left_->init();
  opl_right_->init();

  // TODO hook into an audio buffer mixer
}
