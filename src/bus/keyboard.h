#pragma once

#include <SDL2/SDL.h>
#include <glog/logging.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <boost/circular_buffer.hpp>

#include "bus/sdl_to_atset_keymap.h"
#include "cpu/cpu_65816.h"
#include <atomic>

class System;
class InterruptController;

// Emulate an 8042-style keyboard controller. Mostly works.
class Keyboard : public SystemBusDevice {
 public:
  Keyboard(System* system, InterruptController* int_controller);
  ~Keyboard() override = default;

  void PushKey(uint8_t key);

  // SystemBusDevice implementation
  void StoreByte(const Address& addr, uint8_t v, uint8_t** address) override;
  uint8_t ReadByte(const Address& addr, uint8_t** address) override;
  bool DecodeAddress(const Address& from_addr, Address& to_addr) override;

 private:
  void PollKeyboard();
  void PushKey(const Keybinding& key, bool release);

  std::atomic_bool running_;
  std::thread poll_thread_;

  System *sys_;
  InterruptController* int_controller_;

  struct RepeatKeyInfo {
    std::mutex repeat_mutex_;

    Keybinding key = kNoKey;
    uint16_t repeats = 0;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>
        last_key_output_time;
  };
  RepeatKeyInfo repeat_key_;

  std::recursive_mutex keyboard_mutex_;
  boost::circular_buffer<uint8_t>
      input_buffer_;  // written to by the CPU at + 0x0
  boost::circular_buffer<uint8_t>
      output_buffer_;  // written to by the keyboard, read by CPU at  + 0x0
  uint8_t status_register_ = 0;  // read by the CPU at + 0x4

  bool expect_command_byte_ = false;

  uint8_t ccb_ = 0;  // controller command byte;
};
