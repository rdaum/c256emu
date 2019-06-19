#include "system.h"

#include <gflags/gflags.h>

#include "bus/c256_system_bus.h"
#include "bus/int_controller.h"
#include "bus/keyboard.h"
#include "bus/loader.h"
#include "bus/vdma.h"
#include "bus/vicky.h"
#include "gui/gui.h"

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

System::System()
    : system_bus_(std::make_unique<C256SystemBus>(this)),
      gui_(std::make_unique<GUI>(this)),
      cpu_(system_bus_.get()),
      debug_(&cpu_, &events_, system_bus_.get(), true),
      automation_(&cpu_, this, &debug_) {}

System::~System() = default;

void System::LoadHex(const std::string& kernel_hex_file) {
  // GAVIN copies up to 512k flash mem to kernel mem.
  LoadFromHex(kernel_hex_file, system_bus_.get());
}

void System::LoadBin(const std::string& kernel_bin_file, uint32_t addr) {
  // GAVIN copies up to 512k flash mem to kernel mem.
  LoadFromBin(kernel_bin_file, addr, system_bus_.get());
}

void System::Initialize() {
  // Copy Bank 18 to Bank 0
  LOG(INFO) << "Copying flash bank 18 to bank 0...";
  for (int i = 0; i < 1 << 16; i++) {
    system_bus_->WriteByte(i, system_bus_->ReadByte(0x180000 + i));
  }

  SDL_Init(SDL_INIT_VIDEO);

  LOG(INFO) << "Starting Vicky...";

  // Fire up Vicky
  system_bus_->vicky()->Start();

  LOG(INFO) << "Starting CPU...";

  // Lower the reset pin.
  cpu_.PowerOn();

  // Fire up the GUI debugger;
  gui_->Start();
}

void System::Sys(uint32_t address) {
  cpu_.cpu_state.ip = address & 0xFFFF;
  cpu_.cpu_state.code_segment_base = address & 0xFF0000;
}

DebugInterface* System::GetDebugInterface() {
  return &debug_;
}

void System::DrawNextLine() {
  system_bus_->vicky()->RenderLine();

  bool frame_end = system_bus_->vicky()->is_vertical_end();
  if (frame_end) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        SetStop();
        gui_->Stop();
        return;
      } else if (event.window.windowID == system_bus_->vicky()->window_id()) {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
          SetStop();
          gui_->Stop();
        }
        system_bus_->keyboard()->ProcessEvent(event);
      }
      system_bus_->vdma()->OnFrameStart();
    }

    current_frame_++;
    system_bus_->int_controller()->RaiseFrameStart();

    if (!FLAGS_turbo) {
      auto sleep_time = next_frame_clock - frame_clock;
      std::this_thread::sleep_for(sleep_time);
    }

    auto now = std::chrono::high_resolution_clock::now();
    if (current_frame_ % 60 == 0) {
      auto profile_now_time = now;
      auto profile_time_past = profile_now_time - profile_previous_time;
      uint64_t profile_cycles_taken =
          cpu_.cpu_state.cycle - profile_last_cycles;

      double time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           profile_time_past)
                           .count();
      double mhz_equiv = profile_cycles_taken / 1000.0 / time_ms;
      profile_info_.fps = 1000.0 / (time_ms / 60.0);
      profile_info_.mhz_equiv = mhz_equiv;
      profile_last_cycles = cpu_.cpu_state.cycle;
      profile_previous_time = profile_now_time;
    }
    frame_clock = now;
    next_frame_clock += kVickyFrameDelayDurationNs;
  }
  ScheduleNextScanline();
}

void System::ScheduleNextScanline() {
  total_scanlines_++;
  events_.ScheduleNoLock(
      (kTargetClockRate * total_scanlines_) / kRasterLinesPerSecond,
      std::bind(&System::DrawNextLine, this));
}

void System::Run() {
  total_scanlines_ = 0;
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

uint16_t System::ReadTwoBytes(uint32_t addr) {
  return cpu_.PeekU16(addr);
}

uint16_t System::ReadByte(uint32_t addr) {
  return system_bus_->ReadByte(addr);
}

void System::StoreByte(uint32_t addr, uint8_t val) {
  system_bus_->WriteByte(addr, val);
}

void System::RaiseIRQ() {
  cpu_.cpu_state.SetInterruptSource(1);
}

void System::ClearIRQ() {
  cpu_.cpu_state.ClearInterruptSource(1);
}

void System::SetStop() {
  cpu_.cpu_state.cycle_stop = 0;
}

Automation* System::automation() {
  return &automation_;
}

Vicky* System::vicky() const {
  return system_bus_->vicky();
}
