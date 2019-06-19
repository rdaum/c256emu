#include "bus/c256_system_bus.h"

#include "bus/ch376_sd.h"
#include "bus/int_controller.h"
#include "bus/keyboard.h"
#include "bus/math_copro.h"
#include "bus/rtc.h"
#include "bus/vdma.h"
#include "bus/vicky.h"

C256SystemBus::C256SystemBus(System* sys) {
  math_co_ = std::make_unique<MathCoprocessor>();
  int_controller_ = std::make_unique<InterruptController>(sys);
  // TODO: Timers 0x160 - 0x17f
  // TODO: SDMA: 0x180-0x19f
  keyboard_ = std::make_unique<Keyboard>(sys, int_controller_.get());
  vicky_ = std::make_unique<Vicky>(sys, int_controller_.get());
  vdma_ = std::make_unique<VDMA>(vicky_->vram(), int_controller_.get());
  rtc_ = std::make_unique<Rtc>();
  sd_ = std::make_unique<CH376SD>(int_controller_.get(), ".");
  InitBus();
}

C256SystemBus::~C256SystemBus() {}

bool C256SystemBus::IsIoDeviceAddress(void* context, cpuaddr_t addr) {
  return ((addr & 0xFF0000) == 0xAF0000) || (addr >= 0x100 && addr <= 0x1FF);
}

void C256SystemBus::IoRead(void* context,
                           cpuaddr_t addr,
                           uint8_t* data,
                           uint32_t size) {
  C256SystemBus* self = (C256SystemBus*)context;
  if ((addr & 0xFF0000) == 0xAF0000) {
    addr &= 0xFFFF;
    if (addr >= 0xE808 && addr <= 0xE810)
      *data = self->sd_->ReadByte(addr);
    else if (addr == 0x1060 || addr == 0x1064)
      *data = self->keyboard_->ReadByte(addr);
    else if (addr >= 0x800 && addr <= 0x80F)
      *data = self->rtc_->ReadByte(addr);
    else if (addr >= 0x400 && addr <= 0x04ff)
      *data = self->vdma_->ReadByte(addr);
    else
      *data = self->vicky_->ReadByte(addr);
  } else if (addr >= 0x100 && addr < 0x1A0) {
    if (addr < 0x130)
      *data = self->math_co_->ReadByte(addr);
    else if (addr >= 0x140 && addr <= 0x14F)
      *data = self->int_controller_->ReadByte(addr);
  }
}

void C256SystemBus::IoWrite(void* context,
                            cpuaddr_t addr,
                            const uint8_t* data,
                            uint32_t size) {
  C256SystemBus* self = (C256SystemBus*)context;
  if ((addr & 0xFF0000) == 0xAF0000) {
    addr &= 0xFFFF;
    if (addr >= 0xE808 && addr <= 0xE810)
      self->sd_->StoreByte(addr, *data);
    else if (addr == 0x1060 || addr == 0x1064)
      self->keyboard_->StoreByte(addr, *data);
    else if (addr >= 0x800 && addr <= 0x80F)
      self->rtc_->StoreByte(addr, *data);
    else if (addr >= 0x400 && addr <= 0x04ff)
      self->vdma_->StoreByte(addr, *data);
    else
      self->vicky_->StoreByte(addr, *data);
  } else if (addr >= 0x100 && addr < 0x1A0) {
    if (addr < 0x130)
      self->math_co_->StoreByte(addr, *data);
    else if (addr >= 0x140 && addr <= 0x14F)
      self->int_controller_->StoreByte(addr, *data);
  }
}

void C256SystemBus::InitBus() {
  Init(12, 24, pages);

  constexpr uint32_t kPagesPer64k = 16;
  // Init memory map
  for (uint32_t i = 0; i < 256; i++) {
    for (uint32_t j = 0; j < kPagesPer64k; j++) {
      auto& p = pages[i * kPagesPer64k + j];
      p.ptr = 0;
      p.flags = 0;
      p.io_mask = 0;
      p.io_eq = 1;
      p.cycles_per_access = 1;

      // IO is mapped at 00:01xx and AF:xxxx
      if (i == 0xAF) {
        p.io_mask = 0;
        p.io_eq = 0;
      }
      if (i == 0 && j == 0) {
        p.io_mask = 0xFF00;
        p.io_eq = 0x100;
      }
      if (i >= 0xF0)
        p.flags = Page::kReadOnly;
    }
  }

  // Map the various regions
  Map(0, ram_, 0x200000);
  Map(0xB00000, vicky_->vram(), 0x400000);
  // Map(sysflash.get(), 0xF00000);
  // Map(userflash.get(), 0xF80000);

  io_devices.context = this;
  io_devices.read = &IoRead;
  io_devices.write = &IoWrite;
  io_devices.is_io_device_address = &IsIoDeviceAddress;
  // IRQ controller should be asserting and deasserting the IRQs
  io_devices.irq_taken = [](void*, uint32_t) {};

  vicky_->InitPages(&pages[0xAF * kPagesPer64k]);
}
