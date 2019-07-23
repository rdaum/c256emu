#include "system.h"

#include <gflags/gflags.h>

#include "bus/c256_system_bus.h"
#include "bus/i8042_kbd_mouse.h"
#include "bus/int_controller.h"
#include "bus/loader.h"
#include "bus/ps2_kbdmouse.h"
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
constexpr int kRasterLinesPerSecond =
    (kVickyBitmapHeight + kVickyVBlankLines) * kVickyTargetFps;

DEFINE_bool(gui, true, "Enable the GUI debugger / profiler");
DEFINE_double(clock_rate, 14.318, "Target clock rate in Mhz");

void key_cb_func(GLFWwindow *window, int key, int scancode, int action,
                 int mods) {
  System *system = (System *)glfwGetWindowUserPointer(window);
  system->KeyEvent(key, scancode, action, mods);

}

void mouse_move_cb_func(GLFWwindow *window, double xpos, double ypos) {
  System *system = (System *)glfwGetWindowUserPointer(window);
  system->MouseMoveEvent(xpos, ypos);
}

void mouse_scroll_cb_func(GLFWwindow *window, double xoffset, double yoffset) {
  System *system = (System *)glfwGetWindowUserPointer(window);
  system->MouseScrollEvent(xoffset, yoffset);
}

void mouse_button_cb_func(GLFWwindow *window, int button, int action,
                          int mods) {
  System *system = (System *)glfwGetWindowUserPointer(window);
  system->MouseButtonEvent(button, action, mods);
}

void window_close_callback(GLFWwindow *window) {
  System *system = (System *)glfwGetWindowUserPointer(window);
  system->SetStop();
}

} // namespace

System::System()
    : system_bus_(std::make_unique<C256SystemBus>(this)),
      loader_(system_bus_.get()),
      gui_(FLAGS_gui ? std::make_unique<GUI>(this) : nullptr),
      cpu_(system_bus_.get()), debug_(&cpu_, &events_, system_bus_.get(), true),
      automation_(&cpu_, this, &debug_) {

}

System::~System() = default;

void System::Initialize() {
  glfwInit();
  LOG(INFO) << "Starting Vicky...";

  // Fire up Vicky
  GLFWwindow *window = system_bus_->vicky()->Start();

  cpu_.tracing.addrs.resize(16);

  BootCPU();

  if (FLAGS_gui) {
    // Fire up the GUI debugger;
    int x, y;
    glfwGetWindowPos(window, &x, &y);
    gui_->Start(x, y);

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, key_cb_func);
    glfwSetCursorPosCallback(window, mouse_move_cb_func);
    glfwSetScrollCallback(window, mouse_scroll_cb_func);
    glfwSetMouseButtonCallback(window, mouse_button_cb_func);
    glfwSetWindowCloseCallback(window, window_close_callback);
  }
}

void System::BootCPU(bool hard_boot) {
  // Copy Flash bank 18 to Bank 0
  LOG(INFO) << "Copying flash bank 18 to bank 0...";
  for (int i = 0; i < 1 << 16; i++) {
    system_bus_->WriteByte(i, system_bus_->ReadByte(0x180000 + i));
  }

  LOG(INFO) << "PowerOn CPU...";
  // Lower the reset pin.

  if (hard_boot) {
    cpu_.PowerOn();
  } else
    cpu_.Reset();
}

void System::Sys(uint32_t address) {
  // Emulate a JML. Stack won't be in a good state.
  cpu_.cpu_state.code_segment_base = address & 0xFF0000;
  cpu_.cpu_state.ip = address & 0xFFFF;
}

DebugInterface *System::GetDebugInterface() { return &debug_; }

void System::DrawNextLine() {
  system_bus_->vicky()->RenderLine();

  bool frame_end = system_bus_->vicky()->is_vertical_end();
  if (frame_end) {
    current_frame_++;
    system_bus_->int_controller()->SetFrameStart(true);
    system_bus_->vdma()->OnFrameStart();

    if (live_watches_) {
      PerformWatches();
    }

    auto sleep_time = next_frame_clock - frame_clock;
    std::this_thread::sleep_for(sleep_time);

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
  } else {
    system_bus_->int_controller()->SetFrameStart(false);
  }
  ScheduleNextScanline();
}

void System::ScheduleNextScanline() {
  total_scanlines_++;
  events_.ScheduleNoLock((FLAGS_clock_rate * 1000000 * total_scanlines_) /
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

void System::PerformWatches() {
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
    watched_sp_ = sp;
    for (uint8_t i = 0; i < 0xff; i++) {
      stack_watch_.push_back(ReadByte(watched_sp_ - i));
    }
    peek_rtsl_ = ReadByte(sp + 1);
    peek_rtsl_ |= ReadByte(sp + 2) << 8;
    peek_rtsl_ |= ReadByte(sp + 3) << 16;
  }
  if (direct_page_watch_enabled_) {
    direct_page_watch_.clear();
    uint16_t dp = cpu_.cpu_state.regs.d.u16;
    for (uint8_t i = 0; i < 0xff; i++) {
      direct_page_watch_.push_back(ReadByte(dp + i));
    }
  }
}

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

uint16_t System::watched_sp() const { return watched_sp_; }

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

uint32_t System::peek_rtsl() const { return peek_rtsl_; }

std::vector<uint8_t> System::direct_page_watch() {
  std::unique_lock<std::mutex> l(memory_watch_mutex_);
  return direct_page_watch_;
}

void System::KeyEvent(int key, int scancode, int action, int mods) {
  system_bus()->keyboard()->kbd()->ps2_keyboard_event(key, scancode, action,
                                                      mods);
}


void System::MouseMoveEvent(double xpos, double ypos) {
  system_bus()->keyboard()->mouse()->ps2_mouse_move(xpos, ypos);
}

void System::MouseScrollEvent(double xoffset, double yoffset) {
  system_bus()->keyboard()->mouse()->ps2_mouse_scroll(xoffset, yoffset);
}

void System::MouseButtonEvent(int button, int action, int mods) {
  system_bus()->keyboard()->mouse()->ps2_mouse_button(button, action, mods);
}