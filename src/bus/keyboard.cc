#include "bus/keyboard.h"

#include <chrono>
#include <thread>

#include "bus/int_controller.h"
#include "bus/sdl_to_atset_keymap.h"
#include "bus/system.h"

namespace {
// status register bits
constexpr uint8_t kOutputBufferStatus = 0; // clear if empty, 1 if readable
constexpr uint8_t kInputBufferStatus =
    1; // clear if empty and writable, 1 if full, don't write
constexpr uint8_t kSystemFlag = 2;    // 0 on reset, 1 after self test
constexpr uint8_t kCommandData = 3;   // 0 if last was data, 1 if was command
constexpr uint8_t kKeyboardLock = 4;  // 0 locked, 1 unlocked
constexpr uint8_t kAuxBufferFull = 5; // what's in output? 0 keyboard, 1 mouse
constexpr uint8_t kTimeout = 6;       // timeout on transmission
constexpr uint8_t kParity = 7;        // parity error

// controller command bits (write to 0x0)
constexpr uint8_t kInterruptEnable = 0; // send interrupt if buffer is full
constexpr uint8_t kMouseInterruptEnable =
    1; // send interrupt when mouse output buffer is full
// constexpr uint8_t kSystemFlag = 2;         // see above
constexpr uint8_t kIgnoreKeyboardLock = 3; // unused always 0 on ps2
constexpr uint8_t kKeyboardEnable = 4;     // 0 enable, 1 disable
constexpr uint8_t kMouseEnable = 5;        // 0 enable, 1 disable
constexpr uint8_t kTranslate =
    6; // 0 no translate, 1 translate keyboard scancodes

// CPU -> controller commands (write to 0x64)
constexpr uint8_t kReadCommandByte = 0x20;
constexpr uint8_t kWriteCommandByte = 0x60;
constexpr uint8_t kSelfTest = 0xaa;
constexpr uint8_t kInterfaceTest = 0xab;
constexpr uint8_t kDisableKeyboard = 0xad;
constexpr uint8_t kEnableKeyboard = 0xae;
constexpr uint8_t kReadInputPort = 0xc0;
constexpr uint8_t kReadOutputPort = 0xd0;
constexpr uint8_t kWriteOtputPort = 0xd1;
constexpr uint8_t kReadTestInputs = 0xe0;
constexpr uint8_t kSystemReset = 0xfe;
constexpr uint8_t kDisableMousePort = 0xa7;
constexpr uint8_t kEnableMousePort = 0xa8;
constexpr uint8_t kTestMousePort = 0xa9;
constexpr uint8_t kWriteMousePort = 0xd4;

constexpr uint8_t kMaxInputBufferSize = 64;

constexpr uint32_t kKeyboardStatusRegister = 0xaf1064;
constexpr uint32_t kKeyboardBufferRegister = 0xaf1060;

} // namespace

Keyboard::Keyboard(System *sys, InterruptController *int_controller)
    : sys_(sys), int_controller_(int_controller) {}

void Keyboard::StoreByte(uint32_t addr, uint8_t v) {
  if (addr == 0x1060) {
    if (expect_command_byte_) {
      ccb_ = v;
      expect_command_byte_ = false;
      return;
    }

    if (v == 0xf4) {
      output_buffer_.push_back(0xfa);
    } else {
      output_buffer_.push_back(0);
    }
    return;
  }
  if (addr == 0x1064) {
    if (v == kSelfTest) {
      output_buffer_.push_back(0x55); // test passed
      status_register_ = 0;
      status_register_ |= (1 << kCommandData);
      status_register_ |= (1 << kSystemFlag);

      return;
    }
    if (v == kInterfaceTest) {
      output_buffer_.push_back(0x00); // test passed
      status_register_ = 0;
      status_register_ |= (1 << kCommandData);
      status_register_ |= (1 << kSystemFlag);
      return;
    }
    if (v == kEnableMousePort) {
      status_register_ = 0;
      status_register_ |= (1 << kCommandData);
      status_register_ |= (1 << kSystemFlag);
      return;
    }
    if (v == kTestMousePort) {
      output_buffer_.push_back(0x00); // test passed
      status_register_ = 0;
      status_register_ |= (1 << kCommandData);
      status_register_ |= (1 << kSystemFlag);
      return;
    }
    if (v == kReadCommandByte) {
      output_buffer_.push_back(ccb_);
      status_register_ = 0;
      status_register_ |= (1 << kCommandData);
      status_register_ |= (1 << kSystemFlag);
      return;
    }
    if (v == kWriteCommandByte) {
      // Command will go on 0x1060, next.
      status_register_ = 0;
      status_register_ |= (1 << kCommandData);
      status_register_ |= (1 << kSystemFlag);
      expect_command_byte_ = true;
      return;
    }
  }
}

uint8_t Keyboard::ReadByte(const uint32_t addr) {
  if (addr == 0x1064) {
    uint8_t status_register = status_register_;
    status_register |= (1 << kKeyboardLock);
    if (!output_buffer_.empty())
      status_register |= (1 << kOutputBufferStatus);
    if (!input_buffer_.empty())
      status_register |= (1 << kInputBufferStatus);

    return status_register;
  }
  if (addr == 0x1060) {
    uint8_t return_byte = 0;
    bool is_empty = true;
    {
      if (output_buffer_.empty()) {
        return_byte = 0;
        LOG(WARNING) << "attempt to read empty keyboard output buffer";
      } else {
        return_byte = output_buffer_.front();
        output_buffer_.pop_front();
      }
      is_empty = output_buffer_.empty();
    }
    if (is_empty) {
      int_controller_->LowerKeyboard();
    }
    return return_byte;
  }
  return 0;
}

Keybinding FindKey(SDL_Scancode scan_code) {
  for (auto keymap_key : keymap) {
    if (keymap_key.scancode == scan_code) {
      return keymap_key;
    }
  }
  return kNoKey;
}

void Keyboard::PollKeyboard() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    HandleSDLEvent(event);
  }
}

void Keyboard::HandleSDLEvent(const SDL_Event &event) {
  switch (event.type) {
  case SDL_KEYDOWN: {
    auto key = FindKey(event.key.keysym.scancode);
    if (key.at_set1_code == kNoKey.at_set1_code) {
      LOG(ERROR) << "Unhandled key: " << event.key.keysym.scancode;
    } else {
      repeat_key_.key = key;
      repeat_key_.last_key_output_time =
          std::chrono::time_point_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now());
      PushKey(key, false);
    }
  } break;
  case SDL_KEYUP: {
    auto key = FindKey(event.key.keysym.scancode);
    if (key.at_set1_code == kNoKey.at_set1_code) {
      LOG(ERROR) << "Unhandled key: " << event.key.keysym.scancode;
    } else {
      PushKey(key, true);
      if (key.at_set1_code == repeat_key_.key.at_set1_code) {
        repeat_key_.key = kNoKey;
        repeat_key_.repeats = 0;
      }
    }
  } break;
  }
}

void Keyboard::PushKey(const Keybinding &key, bool release) {
  {
    if (key.extended) {
      output_buffer_.push_back(0xe0);
    }
    output_buffer_.push_back(release ? key.at_set1_code | 0x80
                                     : key.at_set1_code);
  }
  int_controller_->RaiseKeyboard();
}
