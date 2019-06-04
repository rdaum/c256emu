#pragma once

#include <adplug/emuopl.h>
#include <glog/logging.h>
#include <memory>

#include "cpu/cpu_65816.h"

constexpr Address kOplLeftBase(0xaf, 0xe500);
constexpr Address kOplLeftTop(0xaf, 0xe5ff);
constexpr Address kOplRightBase(0xaf, 0xe600);
constexpr Address kOplRightTop(0xaf, 0xe6ff);
constexpr Address kOplBothBase(0xaf, 0xe700);
constexpr Address kOplBothTop(0xaf, 0xe7ff);

class OPL2 : public SystemBusDevice {
public:
  OPL2();

  void Start();

  // SystemBusDevice implementation
  void StoreByte(const Address &addr, uint8_t v, uint8_t **address) override;
  uint8_t ReadByte(const Address &addr, uint8_t **address) override;
  bool DecodeAddress(const Address &from_addr, Address &to_addr) override;

private:
  std::unique_ptr<CEmuopl> opl_left_;
  std::unique_ptr<CEmuopl> opl_right_;
};