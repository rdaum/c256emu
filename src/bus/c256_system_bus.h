#pragma once

#include "cpu/65816/cpu_65c816.h"

class MathCoprocessor;
class Vicky;
class I8042;
class Rtc;
class CH376SD;
class InterruptController;
class System;
class VDMA;

class C256SystemBus : public SystemBus {
 public:
  explicit C256SystemBus(System* sys);
  virtual ~C256SystemBus();

  InterruptController* int_controller() const { return int_controller_.get(); }
  Vicky* vicky() const { return vicky_.get(); }
  VDMA* vdma() const { return vdma_.get(); }
  I8042* keyboard() const { return keyboard_.get(); }

 private:
  void InitBus();
  static bool IsIoDeviceAddress(void* context, cpuaddr_t addr);
  static void IoRead(void* context,
                     cpuaddr_t addr,
                     uint8_t* data,
                     uint32_t size);
  static void IoWrite(void* context,
                      cpuaddr_t addr,
                      const uint8_t* data,
                      uint32_t size);

  std::unique_ptr<MathCoprocessor> math_co_;
  std::unique_ptr<InterruptController> int_controller_;
  std::unique_ptr<Vicky> vicky_;
  std::unique_ptr<VDMA> vdma_;
  std::unique_ptr<I8042> keyboard_;
  std::unique_ptr<Rtc> rtc_;
  std::unique_ptr<CH376SD> sd_;
  Page pages[4096];
  uint8_t ram_[0x400000];
};
