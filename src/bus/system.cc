#include "bus/system.h"

#include <gflags/gflags.h>

#include "bus/ch376_sd.h"
#include "bus/int_controller.h"
#include "bus/keyboard.h"
#include "bus/loader.h"
#include "bus/math_copro.h"
#include "bus/rtc.h"
#include "bus/vicky.h"

namespace {

constexpr double kVickyTargetFps = 60;
constexpr auto kVickyFrameDelayDuration =
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::seconds(1)) /
    kVickyTargetFps;
constexpr auto kVickyFrameDelayDurationNs =
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        kVickyFrameDelayDuration);
constexpr uint64_t kTargetClockRate = 14318000;
constexpr int kRasterLinesPerSecond = kVickyBitmapHeight * kVickyTargetFps;

DEFINE_bool(turbo, false, "Enable turbo mode; do not throttle to 60fps/14mhz");

}  // namespace

class C256SystemBus : public SystemBus {
 public:
  explicit C256SystemBus(System *sys) {
    math_co_ = std::make_unique<MathCoprocessor>();
    int_controller_ = std::make_unique<InterruptController>(sys);
    // TODO: Timers 0x160 - 0x17f
    // TODO: SDMA: 0x180-0x19f
    keyboard_ = std::make_unique<Keyboard>(sys, int_controller_.get());
    vicky_ = std::make_unique<Vicky>(sys, int_controller_.get());
    rtc_ = std::make_unique<Rtc>();
    sd_ = std::make_unique<CH376SD>(int_controller_.get(), ".");
    InitBus();
  }
  virtual ~C256SystemBus() = default;

  InterruptController *int_controller() const { return int_controller_.get(); }
  Vicky *vicky() const { return vicky_.get(); }

 private:
  void InitBus();
  static bool IsIoDeviceAddress(void *context, cpuaddr_t addr);
  static void IoRead(void *context, cpuaddr_t addr, uint8_t *data,
                     uint32_t size);
  static void IoWrite(void *context, cpuaddr_t addr, const uint8_t *data,
                      uint32_t size);

  std::unique_ptr<MathCoprocessor> math_co_;
  std::unique_ptr<InterruptController> int_controller_;
  std::unique_ptr<Vicky> vicky_;
  std::unique_ptr<Keyboard> keyboard_;
  std::unique_ptr<Rtc> rtc_;
  std::unique_ptr<CH376SD> sd_;
  Page pages[4096];
  uint8_t ram_[0x400000];
};

bool C256SystemBus::IsIoDeviceAddress(void *context, cpuaddr_t addr) {
  return ((addr & 0xFF0000) == 0xAF0000) || (addr >= 0x100 && addr <= 0x1FF);
}
void C256SystemBus::IoRead(void *context, cpuaddr_t addr, uint8_t *data,
                           uint32_t size) {
  C256SystemBus *self = (C256SystemBus *)context;
  if ((addr & 0xFF0000) == 0xAF0000) {
    addr &= 0xFFFF;
    if (addr >= 0xE808 && addr <= 0xE810)
      *data = self->sd_->ReadByte(addr);
    else if (addr == 0x1060 || addr == 0x1064)
      *data = self->keyboard_->ReadByte(addr);
    else if (addr >= 0x800 && addr <= 0x80F)
      *data = self->rtc_->ReadByte(addr);
    else
      *data = self->vicky_->ReadByte(addr);
  } else if (addr >= 0x100 && addr < 0x1A0) {
    if (addr < 0x130)
      *data = self->math_co_->ReadByte(addr);
    else if (addr >= 0x140 && addr <= 0x14F)
      *data = self->int_controller_->ReadByte(addr);
  }
}
void C256SystemBus::IoWrite(void *context, cpuaddr_t addr, const uint8_t *data,
                            uint32_t size) {
  C256SystemBus *self = (C256SystemBus *)context;
  if ((addr & 0xFF0000) == 0xAF0000) {
    addr &= 0xFFFF;
    if (addr >= 0xE808 && addr <= 0xE810)
      self->sd_->StoreByte(addr, *data);
    else if (addr == 0x1060 || addr == 0x1064)
      self->keyboard_->StoreByte(addr, *data);
    else if (addr >= 0x800 && addr <= 0x80F)
      self->rtc_->StoreByte(addr, *data);
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
      auto &p = pages[i * kPagesPer64k + j];
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
      if (i >= 0xF0) p.flags = Page::kReadOnly;
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
  io_devices.irq_taken = [](void *, uint32_t) {};

  vicky_->InitPages(&pages[0xAF * kPagesPer64k]);
}

System::System()
    : system_bus_(std::make_unique<C256SystemBus>(this)),
      cpu_(system_bus_.get()),
      debug_(&cpu_, &events_, system_bus_.get(), true) {}

System::~System() {}

void System::LoadHex(const std::string &kernel_hex_file) {
  // GAVIN copies up to 512k flash mem to kernel mem.
  LoadFromHex(kernel_hex_file, system_bus_.get());
}

void System::LoadBin(const std::string &kernel_bin_file, uint32_t addr) {
  // GAVIN copies up to 512k flash mem to kernel mem.
  LoadFromBin(kernel_bin_file, addr, system_bus_.get());
}

void System::Initialize() {
  // Copy Bank 18 to Bank 0
  LOG(INFO) << "Copying flash bank 18 to bank 0...";
  for (int i = 0; i < 1 << 16; i++) {
    system_bus_->WriteByte(i, system_bus_->ReadByte(0x180000 + i));
  }

  LOG(INFO) << "Starting Vicky...";

  // Fire up Vicky
  system_bus_->vicky()->Start();

  LOG(INFO) << "Starting CPU...";

  // Lower the reset pin.
  cpu_.PowerOn();
}

void System::Sys(uint32_t address) {
  cpu_.cpu_state.ip = address & 0xFFFF;
  cpu_.cpu_state.code_segment_base = address & 0xFF0000;
}

DebugInterface *System::GetDebugInterface() { return &debug_; }

void System::DrawNextLine() {
  system_bus_->vicky()->RenderLine();
  ScheduleNextScanline();

  bool frame_end = system_bus_->vicky()->is_vertical_end();
  if (frame_end) {
    current_frame_++;
    system_bus_->int_controller()->RaiseFrameStart();

    if (!FLAGS_turbo) {
      auto sleep_time = next_frame_clock - frame_clock;
      std::this_thread::sleep_for(sleep_time);
    }

    auto now = std::chrono::high_resolution_clock::now();
    if (profile_ && current_frame_ % 60 == 0) {
      auto profile_now_time = now;
      auto profile_time_past = profile_now_time - profile_previous_time;
      uint64_t profile_cycles_taken =
          cpu_.cpu_state.cycle - profile_last_cycles;

      double time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           profile_time_past)
                           .count();
      double mhz_equiv = profile_cycles_taken / 1000.0 / time_ms;
      LOG(INFO) << "average " << 1000.0 / (time_ms / 60.0) << "fps; "
                << profile_cycles_taken / 60.0 / 480.0 << "cycles per line; "
                << mhz_equiv << "mhz equiv;";
      profile_last_cycles = cpu_.cpu_state.cycle;
      profile_previous_time = profile_now_time;
    }
    frame_clock = now;
    next_frame_clock += kVickyFrameDelayDurationNs;
  }
}

void System::ScheduleNextScanline() {
  total_scanlines_++;
  events_.ScheduleNoLock(
      (kTargetClockRate * total_scanlines_) / kRasterLinesPerSecond,
      std::bind(&System::DrawNextLine, this));
}

void System::Run(bool profile) {
  total_scanlines_ = 0;
  profile_ = profile;
  cpu_.cpu_state.cycle = 0;
  current_frame_ = 0;
  profile_last_cycles = 0;
  profile_previous_time = frame_clock = next_frame_clock =
      std::chrono::high_resolution_clock::now();
  next_frame_clock += kVickyFrameDelayDurationNs;

  events_.Start(&cpu_.cpu_state.event_cycle, cpu_.cpu_state.cycle_stop);
  ScheduleNextScanline();
  cpu_.Emulate(&events_);
}

uint16_t System::ReadTwoBytes(uint32_t addr) { return cpu_.PeekU16(addr); }

uint16_t System::ReadByte(uint32_t addr) { return system_bus_->ReadByte(addr); }

void System::StoreByte(uint32_t addr, uint8_t val) {
  system_bus_->WriteByte(addr, val);
}

void System::RaiseIRQ() { cpu_.cpu_state.SetInterruptSource(1); }

void System::ClearIRQ() { cpu_.cpu_state.ClearInterruptSource(1); }

void System::Start(bool profile) { Run(profile); }

void System::Stop() {
  events_.Schedule(0, [this]() { cpu_.cpu_state.cycle_stop = 0; });
}
