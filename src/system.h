#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "automation/automation.h"
#include "bus/loader.h"
#include "cpu/65816/cpu_65c816.h"
#include "debug_interface.h"

class GUI;
class C256SystemBus;
class Vicky;

struct ProfileInfo {
  double mhz_equiv;
  double fps;
};

// Owns and configures all bus devices and the CPU.
class System {
 public:
  System();
  ~System();

  void Initialize();

  // Launch the CPU scheduler.
  void Run();

  // Stop the CPU scheduler completely.
  void SetStop();

  // [Re]Boot the CPU; usually called by Initialize
  void BootCPU(bool hard_boot = true);

  // Ask the bus to read or write addresses in a thread safe way.
  uint16_t ReadTwoBytes(uint32_t addr);
  uint16_t ReadByte(uint32_t addr);
  void StoreByte(uint32_t addr, uint8_t val);

  // Jump to address.
  void Sys(uint32_t address);

  WDC65C816* cpu() { return &cpu_; }
  DebugInterface* GetDebugInterface();
  ProfileInfo profile_info() const { return profile_info_; }
  Automation* automation();
  Vicky* vicky() const;
  C256SystemBus *system_bus() const { return system_bus_.get(); }

  Loader* loader() { return &loader_; }

  void set_live_watches(bool live_watch) { live_watches_ = true; }
  bool live_watches() const { return live_watches_; }

  void PerformWatches();

  struct MemoryWatch {
    cpuaddr_t start_addr;
    size_t num_bytes;
    std::vector<uint8_t> last_results;
  };
  void AddMemoryWatch(cpuaddr_t start_addr, size_t num_bytes);
  void DelMemoryWatch(cpuaddr_t start_addr);
  std::vector<MemoryWatch> memory_watches();

  void set_stack_watch_enabled(bool enable);
  bool stack_watch_enabled() const;
  uint16_t watched_sp() const;
  uint32_t peek_rtsl() const;
  std::vector<uint8_t> stack_watch();

  void set_direct_page_watch_enabled(bool enable);
  bool direct_page_watch_enabled() const;
  std::vector<uint8_t> direct_page_watch();

  void DrawNextLine();
  void ScheduleNextScanline();
  void KeyEvent(int key, int scancode, int action,
                int mods);
  void MouseMoveEvent(double xpos, double ypos);
  void MouseScrollEvent(double xoffset, double yoffset);
  void MouseButtonEvent(int button, int action,
                        int mods);
protected:
  friend class InterruptController;

  void RaiseIRQ();
  void ClearIRQ();

 private:


  uint32_t current_frame_ = 0;
  uint64_t total_scanlines_ = 0;
  uint64_t profile_last_cycles = 0;

  ProfileInfo profile_info_;

  std::chrono::time_point<std::chrono::high_resolution_clock>
      profile_previous_time;
  std::chrono::time_point<std::chrono::high_resolution_clock> frame_clock;
  std::chrono::time_point<std::chrono::high_resolution_clock> next_frame_clock;

  std::unique_ptr<C256SystemBus> system_bus_;
  Loader loader_;

  std::unique_ptr<GUI> gui_;

  WDC65C816 cpu_;
  EventQueue events_;
  DebugInterface debug_;
  Automation automation_;

  std::atomic_bool live_watches_ = true;

  std::mutex memory_watch_mutex_;
  std::vector<MemoryWatch> memory_watches_;
  bool stack_watch_enabled_ = false;
  uint16_t watched_sp_; // the sp at the time the stack watch was captured.
  uint32_t peek_rtsl_;  // 3 bytes behind the sp, to peek at potential RTL/RTS
  std::vector<uint8_t> stack_watch_;
  bool direct_page_watch_enabled_ = false;
  std::vector<uint8_t> direct_page_watch_;
};
