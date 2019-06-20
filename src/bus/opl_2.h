#pragma once

#include <adplug/emuopl.h>
#include <glog/logging.h>

#include <memory>

constexpr uint32_t kOplLeftBase = 0xe500;
constexpr uint32_t kOplLeftTop = 0xe5ff;
constexpr uint32_t kOplRightBase = 0xe600;
constexpr uint32_t kOplRightTop = 0xe6ff;
constexpr uint32_t kOplBothBase = 0xe700;
constexpr uint32_t kOplBothTop = 0xe7ff;

class OPL2 {
 public:
  OPL2();

  void Start();

  // SystemBusDevice implementation
  void StoreByte(uint32_t addr, uint8_t v);
  uint8_t ReadByte(uint32_t addr);

 private:
  std::unique_ptr<CEmuopl> opl_left_;
  std::unique_ptr<CEmuopl> opl_right_;
};