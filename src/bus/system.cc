#include "bus/system.h"

#include <gflags/gflags.h>

#include "bus/ch376_sd.h"
#include "bus/int_controller.h"
#include "bus/keyboard.h"
#include "bus/loader.h"
#include "bus/math_copro.h"
#include "bus/ram_device.h"
#include "bus/rtc.h"
#include "bus/vicky.h"

namespace {

constexpr double kVickyTargetFps = 60;
constexpr auto kVickyFrameDelayDuration =
    std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::seconds(1)) /
    kVickyTargetFps;
constexpr int kTargetClockRate = 14000000;
constexpr int kRasterLinesPerSecond = kVickyBitmapHeight * kVickyTargetFps;
constexpr int kCyclesPerScanline = kTargetClockRate / kRasterLinesPerSecond;

DEFINE_bool(turbo, false, "Enable turbo mode; do not throttle to 60fps/14mhz");

} // namespace

class C256SystemBus : public SystemBus {
public:
  C256SystemBus(System *sys) {
    direct_page_ = std::make_unique<RAMDevice>(Address(0x00, 0x0000),
                                               Address(0x00, 0x00ff));
    math_co_ = std::make_unique<MathCoprocessor>();
    int_controller_ = std::make_unique<InterruptController>(sys);
    // TODO: Timers 0x160 - 0x17f
    // TODO: SDMA: 0x180-0x19f
    ram_ = std::make_unique<RAMDevice>(Address(0x00, 0x0200),
                                       Address(0x1f, 0xffff));
    keyboard_ = std::make_unique<Keyboard>(sys, int_controller_.get());
    vicky_ = std::make_unique<Vicky>(sys, int_controller_.get());
    rtc_ = std::make_unique<Rtc>();
    sd_ = std::make_unique<CH376SD>(int_controller_.get(), ".");

    RegisterDevice(direct_page_.get());
    RegisterDevice(math_co_.get());
    RegisterDevice(ram_.get());
    RegisterDevice(int_controller_.get());
    RegisterDevice(vicky_.get());
    RegisterDevice(keyboard_.get());
    RegisterDevice(rtc_.get());
    RegisterDevice(sd_.get());
  }
  virtual ~C256SystemBus() = default;

  InterruptController *int_controller() const { return int_controller_.get(); }
  Vicky *vicky() const { return vicky_.get(); }

private:
  std::unique_ptr<RAMDevice> direct_page_;
  std::unique_ptr<RAMDevice> ram_;
  std::unique_ptr<MathCoprocessor> math_co_;
  std::unique_ptr<InterruptController> int_controller_;
  std::unique_ptr<Vicky> vicky_;
  std::unique_ptr<Keyboard> keyboard_;
  std::unique_ptr<Rtc> rtc_;
  std::unique_ptr<CH376SD> sd_;
};

System::System()
    : system_bus_(std::make_unique<C256SystemBus>(this)),
      native_mode_interrupts_({
          0x0000, // coprocessor enable
          0xfffe, // brk
          0x0000, // abort
          0xfffa, // nmi
          0xfffc, // reset
          0xfffe, // irq
      }),
      emulation_mode_interrupts_({
          0x0000, // coprocessor enable
          0x0000, // unused
          0x0000, // abort
          0xfffa, // NMI
          0xfffc, // reset
          0xfffe, // brkIrq
      }),
      cpu_(*system_bus_, &emulation_mode_interrupts_,
           &native_mode_interrupts_) {

  automation_ =
      std::make_unique<Automation>(&cpu_, this);
}

System::~System() {}

void System::LoadHex(const std::string &kernel_hex_file) {
  // GAVIN copies up to 512k flash mem to kernel mem.
  LoadFromHex(kernel_hex_file, system_bus_.get());
}

void System::LoadBin(const std::string &kernel_bin_file, const Address &addr) {
  // GAVIN copies up to 512k flash mem to kernel mem.
  LoadFromBin(kernel_bin_file, addr, system_bus_.get());
}

void System::Initialize(const std::string &automation_script) {

  // Copy Bank 18 to Bank 0
  LOG(INFO) << "Copying flash bank 18 to bank 0...";
  for (int i = 0; i < 1 << 16; i++) {
    system_bus_->StoreByte(Address(0, i),
                           system_bus_->ReadByte(Address(0x18, i)));
  }

  LOG(INFO) << "Starting Vicky...";

  // Fire up Vicky
  system_bus_->vicky()->Start();

  LOG(INFO) << "Starting CPU...";

  // Lower the reset pin.
  cpu_.SetRESPin(false);

  if (!automation_script.empty()) {
    if (!automation_->LoadScript(automation_script)) {
      LOG(ERROR) << "Could not load automation script: " << automation_script;
    } else {
      automation_->Run();
    }
  }
}

void System::Sys(const Address &address) {
  std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
  Stop();
  cpu_.Jump(address);
  Resume();
}

void System::Run(bool profile, bool automation) {
  uint64_t frames = 0;
  auto profile_previous_time = std::chrono::high_resolution_clock::now();
  uint64_t profile_last_cycles = 0;
  auto frame_clock = std::chrono::high_resolution_clock::now();

  int cycle_jitter_adjust = 0;
  while (is_cpu_executing_) {

    // Execute the number of instruction's we'd expect to get done in a
    // scanline.
    const uint64_t next_cycle_break =
        cpu_.total_cycles_counter() + kCyclesPerScanline;
    while (cpu_.total_cycles_counter() < next_cycle_break - cycle_jitter_adjust) {
      std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
      CHECK(cpu_.ExecuteNextInstruction());
    }
    cycle_jitter_adjust = cpu_.total_cycles_counter() - next_cycle_break;

    // Render a line.
    {
      bool frame_end = false;
      {
        std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
        system_bus_->vicky()->RenderLine();
        frame_end = system_bus_->vicky()->is_vertical_end();
      }
      // If it's frame end...
      if (frame_end) {
        cycle_jitter_adjust = 0;
        // Sleep amount of time until next frame to get 60fps, if not in
        // turbo mode.
        if (!FLAGS_turbo) {
          auto since_last_render_time =
              std::chrono::high_resolution_clock::now() - frame_clock;
          auto sleep_time = kVickyFrameDelayDuration - since_last_render_time;
          std::this_thread::sleep_for(sleep_time);
        }
        frames++;
        {
          std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
          system_bus_->int_controller()->RaiseFrameStart();
        }

        if (profile && frames % 60 == 0) {
          auto profile_now_time = std::chrono::high_resolution_clock::now();
          auto profile_time_past = profile_now_time - profile_previous_time;
          uint64_t profile_cycles_taken =
              cpu_.total_cycles_counter() - profile_last_cycles;

          double time_ms =
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  profile_time_past)
                  .count();
          double mhz_equiv = profile_cycles_taken / 1000.0 / time_ms;
          LOG(INFO) << "average " << 1000.0 / (time_ms / 60.0) << "fps; "
                    << profile_cycles_taken / 60.0 / 480.0
                    << "cycles per line; " << mhz_equiv << "mhz equiv;";
          profile_last_cycles = cpu_.total_cycles_counter();
          profile_previous_time = std::chrono::high_resolution_clock::now();
        }
        frame_clock = std::chrono::high_resolution_clock::now();
      }
    }
  }
}

uint16_t System::ReadTwoBytes(const Address &addr) {
  std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
  return system_bus_->ReadWord(addr);
}

uint16_t System::ReadByte(const Address &addr) {
  std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
  return system_bus_->ReadByte(addr);
}

void System::StoreByte(const Address &addr, uint8_t val) {
  std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
  system_bus_->StoreByte(addr, val);
}

void System::StoreTwoBytes(const Address &addr, uint16_t val) {
  std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
  system_bus_->StoreWord(addr, val);
}

void System::RaiseIRQ() {
  std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
  cpu_.SetIRQPin(true);
}

void System::ClearIRQ() {
  std::lock_guard<std::recursive_mutex> bus_lock(system_bus_mutex_);
  cpu_.SetIRQPin(false);
}

void System::Suspend() { is_cpu_executing_.store(false); }

void System::Start(bool profile, bool automation) {
  is_main_loop_running_.store(true);
  is_cpu_executing_.store(true);
  while (is_main_loop_running_.load()) {
    Run(profile, automation);

    // To avoid CPU hog, sleep after each run, if we're suspended.
    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(100));
  }
}

void System::Resume() { is_cpu_executing_.store(true); }

void System::Stop() {
  Suspend();
  is_main_loop_running_.store(false);
}
