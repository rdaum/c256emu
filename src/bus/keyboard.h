#pragma once

#include <SDL2/SDL.h>
#include <glog/logging.h>

#include <atomic>
#include <chrono>
#include <circular_buffer.hpp>

#include "bus/sdl_to_atset_keymap.h"

class System;
class InterruptController;

// Emulate an 8042-style keyboard controller. Mostly works.
class Keyboard {
 public:
  Keyboard(System* system, InterruptController* int_controller);
  ~Keyboard() = default;

  void HandleSDLEvent(const SDL_Event &event);

  void StoreByte(uint32_t addr, uint8_t v);
  uint8_t ReadByte(uint32_t addr);

 private:
  void PollKeyboard();
  void PushKey(const Keybinding& key, bool release);

  System *sys_;
  InterruptController* int_controller_;

  struct RepeatKeyInfo {
    Keybinding key = kNoKey;
    uint16_t repeats = 0;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>
        last_key_output_time;
  };
  RepeatKeyInfo repeat_key_;

  jm::circular_buffer<uint8_t, 64>
      input_buffer_;  // written to by the CPU at + 0x0
  jm::circular_buffer<uint8_t, 64>
      output_buffer_;  // written to by the keyboard, read by CPU at  + 0x0
  uint8_t status_register_ = 0;  // read by the CPU at + 0x4

  bool expect_command_byte_ = false;

  uint8_t ccb_ = 0;  // controller command byte;
};
