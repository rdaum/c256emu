#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "bus/automation.h"
#include "cpu/65816/cpu_65c816.h"
#include "debug_interface.h"

class GUI;
class C256SystemBus;

struct ProfileInfo {
  double mhz_equiv;
  double fps;
};

// Owns and configures all bus devices and the CPU.
class System {
 public:
  System();
  ~System();

  void LoadHex(const std::string &kernel_hex_file);

  void LoadBin(const std::string &kernel_bin_file, uint32_t addr);

  void Initialize();

  // Launch the loop thread and run the CPU.
  void Start();

  // Stop the loop thread completely.
  void SetStop();

  // Ask the bus to read or write addresses in a thread safe way.
  uint16_t ReadTwoBytes(uint32_t addr);
  uint16_t ReadByte(uint32_t addr);
  void StoreByte(uint32_t addr, uint8_t val);

  // Jump to address.
  void Sys(uint32_t address);

  WDC65C816 *cpu() { return &cpu_; }

  DebugInterface *GetDebugInterface();

  const ProfileInfo &profile_info() const { return profile_info_; }

protected:
  friend class InterruptController;

  void RaiseIRQ();
  void ClearIRQ();

 private:
  void Run();
  void DrawNextLine();
  void ScheduleNextScanline();

  uint32_t current_frame_ = 0;
  uint64_t total_scanlines_ = 0;
  uint64_t profile_last_cycles = 0;

  ProfileInfo profile_info_;

  std::chrono::time_point<std::chrono::high_resolution_clock>
      profile_previous_time;
  std::chrono::time_point<std::chrono::high_resolution_clock> frame_clock;
  std::chrono::time_point<std::chrono::high_resolution_clock> next_frame_clock;

  std::unique_ptr<C256SystemBus> system_bus_;
  std::unique_ptr<GUI> gui_;

  WDC65C816 cpu_;
  EventQueue events_;
  DebugInterface debug_;
};
