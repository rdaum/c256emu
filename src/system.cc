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
constexpr int kRasterLinesPerSecond =
    (kVickyBitmapHeight + kVickyVBlankLines) * kVickyTargetFps;

DEFINE_bool(turbo, false, "Enable turbo mode; do not throttle to 60fps/14mhz");

} // namespace

System::System()
    : turbo_(FLAGS_turbo), system_bus_(std::make_unique<C256SystemBus>(this)),
      loader_(system_bus_.get()), gui_(std::make_unique<GUI>(this)),
      cpu_(system_bus_.get()), debug_(&cpu_, &events_, system_bus_.get(), true),
      automation_(&cpu_, this, &debug_) {}

System::~System() = default;

void System::Initialize() {
  // Copy Bank 18 to Bank 0
  LOG(INFO) << "Copying flash bank 18 to bank 0...";
  for (int i = 0; i < 1 << 16; i++) {
    system_bus_->WriteByte(i, system_bus_->ReadByte(0x180000 + i));
  }

  SDL_Init(SDL_INIT_VIDEO);

  LOG(INFO) << "Starting Vicky...";

  // Fire up Vicky
  auto window_geometry = system_bus_->vicky()->Start();

  LOG(INFO) << "Starting CPU...";

  // Lower the reset pin.
  cpu_.PowerOn();

  // Fire up the GUI debugger;
  gui_->Start(window_geometry.x + window_geometry.w, window_geometry.y);
}

void System::Sys(uint32_t address) {
  cpu_.cpu_state.ip = address & 0xFFFF;
  cpu_.cpu_state.code_segment_base = address & 0xFF0000;
}

DebugInterface *System::GetDebugInterface() { return &debug_; }

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
    }

    current_frame_++;
    system_bus_->int_controller()->RaiseFrameStart();
    system_bus_->vdma()->OnFrameStart();

    if (!turbo_) {
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
  std::unique_lock<std::mutex> l(memory_watch_mutex_);

  for (auto &mw : memory_watches_) {
    mw.last_results.clear();
    for (uint32_t addr = mw.start_addr; addr < mw.start_addr + mw.num_bytes;
         addr++) {
      mw.last_results.push_back(ReadByte(addr));
    }
  }
  if (stack_watch_enabled_) {
    stack_watch_.clear();
    uint16_t sp = cpu_.cpu_state.regs.sp.u16;
    for (uint8_t i = 0; i < 0xff; i++) {
      stack_watch_.push_back(ReadByte(sp - i));
    }
  }
  if (direct_page_watch_enabled_) {
    direct_page_watch_.clear();
    uint16_t dp = cpu_.cpu_state.regs.d.u16;
    for (uint8_t i = 0; i < 0xff; i++) {
      direct_page_watch_.push_back(ReadByte(dp + i));
    }
  }
  ScheduleNextScanline();
}

void System::ScheduleNextScanline() {
  total_scanlines_++;
  events_.ScheduleNoLock((kTargetClockRate * total_scanlines_) /
                             kRasterLinesPerSecond,
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

uint16_t System::ReadTwoBytes(uint32_t addr) { return cpu_.PeekU16(addr); }

uint16_t System::ReadByte(uint32_t addr) { return system_bus_->ReadByte(addr); }

void System::StoreByte(uint32_t addr, uint8_t val) {
  system_bus_->WriteByte(addr, val);
}

void System::RaiseIRQ() { cpu_.cpu_state.SetInterruptSource(1); }

void System::ClearIRQ() { cpu_.cpu_state.ClearInterruptSource(1); }

void System::SetStop() { cpu_.cpu_state.cycle_stop = 0; }

Automation *System::automation() { return &automation_; }

Vicky *System::vicky() const { return system_bus_->vicky(); }

void System::AddMemoryWatch(uint32_t start_addr, size_t num_bytes) {
  std::unique_lock<std::mutex> l(memory_watch_mutex_);

  memory_watches_.push_back(MemoryWatch{start_addr, num_bytes, {}});
}

void System::DelMemoryWatch(uint32_t start_addr) {
  std::unique_lock<std::mutex> l(memory_watch_mutex_);

  auto it = std::find_if(memory_watches_.begin(), memory_watches_.end(),
                         [start_addr](const MemoryWatch &m) {
                           return m.start_addr == start_addr;
                         });
  if (it != memory_watches_.end()) {
    memory_watches_.erase(it);
  }
}

std::vector<System::MemoryWatch> System::memory_watches() {
  std::unique_lock<std::mutex> l(memory_watch_mutex_);

  return memory_watches_;
}

void System::set_stack_watch_enabled(bool enable) {
  stack_watch_enabled_ = enable;
}

bool System::stack_watch_enabled() const { return stack_watch_enabled_; }

std::vector<uint8_t> System::stack_watch() {
  std::unique_lock<std::mutex> l(memory_watch_mutex_);
  return stack_watch_;
}

void System::set_direct_page_watch_enabled(bool enable) {
  direct_page_watch_enabled_ = enable;
}

bool System::direct_page_watch_enabled() const {
  return direct_page_watch_enabled_;
}

std::vector<uint8_t> System::direct_page_watch() {
  std::unique_lock<std::mutex> l(memory_watch_mutex_);
  return direct_page_watch_;
}
