#pragma once

#include <SDL2/SDL.h>
#include <glog/logging.h>

#include <atomic>
#include <mutex>
#include <thread>
#include "bus/sdl_to_atset_keymap.h"
#include <atomic>
#include <circular_buffer.hpp>

class System;
class InterruptController;

// Emulate an 8042-style keyboard controller. Mostly works.
class Keyboard {
 public:
  Keyboard(System* system, InterruptController* int_controller);
  ~Keyboard() = default;

  void PushKey(uint8_t key);

  // SystemBusDevice implementation
  void StoreByte(uint32_t addr, uint8_t v);
  uint8_t ReadByte(uint32_t addr);

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
  jm::circular_buffer<uint8_t, 64>
      input_buffer_;  // written to by the CPU at + 0x0
  jm::circular_buffer<uint8_t, 64>
      output_buffer_;  // written to by the keyboard, read by CPU at  + 0x0
  uint8_t status_register_ = 0;  // read by the CPU at + 0x4

  bool expect_command_byte_ = false;

  uint8_t ccb_ = 0;  // controller command byte;
};
