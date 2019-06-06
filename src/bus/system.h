#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "bus/automation.h"

#include "cpu/cpu_65816.h"

class C256SystemBus;

// Owns and configures all bus devices and the CPU.
class System {
public:
  System();
  ~System();

  void LoadHex(const std::string &kernel_hex_file);

  void LoadBin(const std::string &kernel_bin_file, const Address &addr);

  void Initialize(const std::string &automation_script = "");

  // Launch the loop thread and run the CPU.
  void Start(bool profile, bool automation);

  // Stop the loop thread completely.
  void Stop();

  // Resume or suspend CPU execution.
  void Resume();
  void Suspend();

  // Ask the bus to read or write addresses in a thread safe way.
  uint16_t ReadTwoBytes(const Address &addr);
  uint16_t ReadByte(const Address &addr);
  void StoreByte(const Address &addr, uint8_t val);
  void StoreTwoBytes(const Address &addr, uint16_t val);

  // Jump to address.
  void Sys(const Address &address);

  Automation *automation() const { return automation_.get(); }
  Cpu65816 *cpu() { return &cpu_; }

protected:
  friend class InterruptController;

  void RaiseIRQ();
  void ClearIRQ();

private:
  void Run(bool profile, bool automation);

  std::atomic_bool is_main_loop_running_; // is the main loop thread running?
  std::atomic_bool is_cpu_executing_;     // is the cpu currently executing?

  std::recursive_mutex system_bus_mutex_;
  std::unique_ptr<C256SystemBus> system_bus_;

  NativeModeInterrupts native_mode_interrupts_;
  EmulationModeInterrupts emulation_mode_interrupts_;

  std::unique_ptr<Automation> automation_;

  Cpu65816 cpu_;
};
