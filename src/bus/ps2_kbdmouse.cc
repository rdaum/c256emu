/*
 * Copyright (c) 2003-2004 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "bus/ps2_kbdmouse.h"

#include <cstring>

#include <GLFW/glfw3.h>
#include <glog/logging.h>

/* Keyboard Commands */
#define KBD_CMD_SET_LEDS 0xED /* Set keyboard leds */
#define KBD_CMD_ECHO 0xEE
#define KBD_CMD_SCANCODE 0xF0      /* Get/set scancode set */
#define KBD_CMD_GET_ID 0xF2        /* get keyboard ID */
#define KBD_CMD_SET_RATE 0xF3      /* Set typematic rate */
#define KBD_CMD_ENABLE 0xF4        /* Enable scanning */
#define KBD_CMD_RESET_DISABLE 0xF5 /* reset and disable scanning */
#define KBD_CMD_RESET_ENABLE 0xF6  /* reset and enable scanning */
#define KBD_CMD_RESET 0xFF         /* Reset */

/* Keyboard Replies */
#define KBD_REPLY_POR 0xAA    /* Power on reset */
#define KBD_REPLY_ID 0xAB     /* Keyboard ID */
#define KBD_REPLY_ACK 0xFA    /* Command ACK */
#define KBD_REPLY_RESEND 0xFE /* Command NACK, send the cmd again */

/* Mouse Commands */
#define AUX_SET_SCALE11 0xE6 /* Set 1:1 scaling */
#define AUX_SET_SCALE21 0xE7 /* Set 2:1 scaling */
#define AUX_SET_RES 0xE8     /* Set resolution */
#define AUX_GET_SCALE 0xE9   /* Get scaling factor */
#define AUX_SET_STREAM 0xEA  /* Set stream mode */
#define AUX_POLL 0xEB        /* Poll */
#define AUX_RESET_WRAP 0xEC  /* Reset wrap mode */
#define AUX_SET_WRAP 0xEE    /* Set wrap mode */
#define AUX_SET_REMOTE 0xF0  /* Set remote mode */
#define AUX_GET_TYPE 0xF2    /* Get type */
#define AUX_SET_SAMPLE 0xF3  /* Set sample rate */
#define AUX_ENABLE_DEV 0xF4  /* Enable aux device */
#define AUX_DISABLE_DEV 0xF5 /* Disable aux device */
#define AUX_SET_DEFAULT 0xF6
#define AUX_RESET 0xFF /* Reset aux device */
#define AUX_ACK 0xFA   /* Command byte ACK. */

#define MOUSE_STATUS_REMOTE 0x40
#define MOUSE_STATUS_ENABLED 0x20
#define MOUSE_STATUS_SCALE21 0x10

#define PS2_QUEUE_SIZE 16 /* Buffer size required by PS/2 protocol */

/* Bits for 'modifiers' field in PS2KbdState */
#define MOD_CTRL_L (1 << 0)
#define MOD_SHIFT_L (1 << 1)
#define MOD_ALT_L (1 << 2)
#define MOD_CTRL_R (1 << 3)
#define MOD_SHIFT_R (1 << 4)
#define MOD_ALT_R (1 << 5)

#define PS2_MOUSE_BUTTON_LEFT 0x01
#define PS2_MOUSE_BUTTON_RIGHT 0x02
#define PS2_MOUSE_BUTTON_MIDDLE 0x04
#define PS2_MOUSE_BUTTON_SIDE 0x08
#define PS2_MOUSE_BUTTON_EXTRA 0x10

static uint8_t translate_table[256] = {
    0xff, 0x43, 0x41, 0x3f, 0x3d, 0x3b, 0x3c, 0x58, 0x64, 0x44, 0x42, 0x40,
    0x3e, 0x0f, 0x29, 0x59, 0x65, 0x38, 0x2a, 0x70, 0x1d, 0x10, 0x02, 0x5a,
    0x66, 0x71, 0x2c, 0x1f, 0x1e, 0x11, 0x03, 0x5b, 0x67, 0x2e, 0x2d, 0x20,
    0x12, 0x05, 0x04, 0x5c, 0x68, 0x39, 0x2f, 0x21, 0x14, 0x13, 0x06, 0x5d,
    0x69, 0x31, 0x30, 0x23, 0x22, 0x15, 0x07, 0x5e, 0x6a, 0x72, 0x32, 0x24,
    0x16, 0x08, 0x09, 0x5f, 0x6b, 0x33, 0x25, 0x17, 0x18, 0x0b, 0x0a, 0x60,
    0x6c, 0x34, 0x35, 0x26, 0x27, 0x19, 0x0c, 0x61, 0x6d, 0x73, 0x28, 0x74,
    0x1a, 0x0d, 0x62, 0x6e, 0x3a, 0x36, 0x1c, 0x1b, 0x75, 0x2b, 0x63, 0x76,
    0x55, 0x56, 0x77, 0x78, 0x79, 0x7a, 0x0e, 0x7b, 0x7c, 0x4f, 0x7d, 0x4b,
    0x47, 0x7e, 0x7f, 0x6f, 0x52, 0x53, 0x50, 0x4c, 0x4d, 0x48, 0x01, 0x45,
    0x57, 0x4e, 0x51, 0x4a, 0x37, 0x49, 0x46, 0x54, 0x80, 0x81, 0x82, 0x41,
    0x54, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
    0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
    0xfc, 0xfd, 0xfe, 0xff,
};

static unsigned int ps2_modifier_bit(int key) {
  switch (key) {
  case GLFW_KEY_LEFT_CONTROL:
    return MOD_CTRL_L;
  case GLFW_KEY_RIGHT_CONTROL:
    return MOD_CTRL_R;
  case GLFW_KEY_LEFT_SHIFT:
    return MOD_SHIFT_L;
  case GLFW_KEY_RIGHT_SHIFT:
    return MOD_SHIFT_R;
  case GLFW_KEY_LEFT_ALT:
    return MOD_ALT_L;
  case GLFW_KEY_RIGHT_ALT:
    return MOD_ALT_R;
  default:
    return 0;
  }
}

struct Keybinding {
  int glfw_key;
  uint8_t at_set1_code;
  uint8_t at_set2_code;
  uint8_t at_set3_code;
  bool meta;
};

constexpr Keybinding keymap[] = {
    {GLFW_KEY_ESCAPE, 0x01, 0x76, 0x08, false},
    {GLFW_KEY_1, 0x02, 0x16, 0x16, false},
    {GLFW_KEY_2, 0x03, 0x1e, 0x1e, false},
    {GLFW_KEY_3, 0x04, 0x26, 0x26, false},
    {GLFW_KEY_4, 0x05, 0x25, 0x25, false},
    {GLFW_KEY_5, 0x06, 0x2e, 0x2e, false},
    {GLFW_KEY_6, 0x07, 0x36, 0x36, false},
    {GLFW_KEY_7, 0x08, 0x3d, 0x3d, false},
    {GLFW_KEY_8, 0x09, 0x3e, 0x3e, false},
    {GLFW_KEY_9, 0x0a, 0x46, 0x46, false},
    {GLFW_KEY_0, 0x0b, 0x45, 0x45, false},
    {GLFW_KEY_MINUS, 0x0c, 0x4e, 0x4e, false},
    {GLFW_KEY_EQUAL, 0x0d, 0x55, 0x55, false},
    {GLFW_KEY_BACKSPACE, 0x0e, 0x66, 0x66, false},
    {GLFW_KEY_TAB, 0x0f, 0x0d, 0x0d, false},
    {GLFW_KEY_Q, 0x10, 0x15, 0x15, false},
    {GLFW_KEY_W, 0x11, 0x1d, 0x1d, false},
    {GLFW_KEY_E, 0x12, 0x24, 0x24, false},
    {GLFW_KEY_R, 0x13, 0x2d, 0x2d, false},
    {GLFW_KEY_T, 0x14, 0x2c, 0x2c, false},
    {GLFW_KEY_Y, 0x15, 0x35, 0x35, false},
    {GLFW_KEY_U, 0x16, 0x3c, 0x3c, false},
    {GLFW_KEY_I, 0x17, 0x43, 0x43, false},
    {GLFW_KEY_O, 0x18, 0x44, 0x44, false},
    {GLFW_KEY_P, 0x19, 0x4d, 0x4d, false},
    {GLFW_KEY_LEFT_BRACKET, 0x1a, 0x54, 0x54, false},
    {GLFW_KEY_RIGHT_BRACKET, 0x1b, 0x5b, 0x5b, false},
    {GLFW_KEY_ENTER, 0x1c, 0x5a, 0x5a, false},
    {GLFW_KEY_LEFT_CONTROL, 0x1d, 0x14, 0x11, true},
    {GLFW_KEY_RIGHT_CONTROL, 0x1d, 0x14, 0x11, true},
    {GLFW_KEY_A, 0x1e, 0x1c, 0x1c, false},
    {GLFW_KEY_S, 0x1f, 0x1b, 0x1b, false},
    {GLFW_KEY_D, 0x20, 0x23, 0x23, false},
    {GLFW_KEY_F, 0x21, 0x2b, 0x2b, false},
    {GLFW_KEY_G, 0x22, 0x34, 0x34, false},
    {GLFW_KEY_H, 0x23, 0x33, 0x33, false},
    {GLFW_KEY_J, 0x24, 0x3b, 0x3b, false},
    {GLFW_KEY_K, 0x25, 0x42, 0x42, false},
    {GLFW_KEY_L, 0x26, 0x4b, 0x4b, false},
    {GLFW_KEY_SEMICOLON, 0x27, 0x4c, 0x4c, false},
    {GLFW_KEY_APOSTROPHE, 0x28, 0x52, 0x52, false},
    {GLFW_KEY_GRAVE_ACCENT, 0x29, 0x0e, 0x0e, false},
    {GLFW_KEY_LEFT_SHIFT, 0x2a, 0x12, 0x12, false},
    {GLFW_KEY_RIGHT_SHIFT, 0x36, 0x59, 0x59, false},
    {GLFW_KEY_BACKSLASH, 0x2b, 0x5d, 0x5c, false},
    {GLFW_KEY_Z, 0x2c, 0x1a, 0x1a, false},
    {GLFW_KEY_X, 0x2d, 0x22, 0x22, false},
    {GLFW_KEY_C, 0x2e, 0x21, 0x21, false},
    {GLFW_KEY_V, 0x2f, 0x2a, 0x2a, false},
    {GLFW_KEY_B, 0x30, 0x32, 0x32, false},
    {GLFW_KEY_N, 0x31, 0x31, 0x31, false},
    {GLFW_KEY_M, 0x32, 0x3a, 0x3a, false},
    {GLFW_KEY_COMMA, 0x33, 0x41, 0x41, false},
    {GLFW_KEY_PERIOD, 0x34, 0x49, 0x49, false},
    {GLFW_KEY_SLASH, 0x35, 0x4a, 0x4a, false},
    {GLFW_KEY_KP_MULTIPLY, 0x37, 0x7c, 0x7e, false},
    {GLFW_KEY_LEFT_ALT, 0x38, 0x11, 0x19, false},
    {GLFW_KEY_RIGHT_ALT, 0x38, 0x11, 0x19, true},
    {GLFW_KEY_SPACE, 0x39, 0x29, 0x29, false},
    {GLFW_KEY_CAPS_LOCK, 0x3a, 0x58, 0x14, false},
    {GLFW_KEY_F1, 0x3b, 0x05, 0x07, false},
    {GLFW_KEY_F2, 0x3c, 0x06, 0x0f, false},
    {GLFW_KEY_F3, 0x3d, 0x04, 0x17, false},
    {GLFW_KEY_F4, 0x3e, 0x0c, 0x1f, false},
    {GLFW_KEY_F5, 0x3f, 0x03, 0x27, false},
    {GLFW_KEY_F6, 0x40, 0x0b, 0x2f, false},
    {GLFW_KEY_F7, 0x41, 0x83, 0x37, false},
    {GLFW_KEY_F8, 0x42, 0x0a, 0x3f, false},
    {GLFW_KEY_F9, 0x43, 0x01, 0x47, false},
    {GLFW_KEY_F10, 0x44, 0x09, 0x4f, false},
    {GLFW_KEY_NUM_LOCK, 0x45, 0x77, 0x76, false},
    {GLFW_KEY_SCROLL_LOCK, 0x46, 0x7e, 0x5f, false},
    {GLFW_KEY_KP_7, 0x47, 0x6c, 0x6c, false},
    {GLFW_KEY_KP_8, 0x48, 0x75, 0x75, false},
    {GLFW_KEY_KP_9, 0x49, 0x7d, 0x7d, false},
    {GLFW_KEY_KP_SUBTRACT, 0x4a, 0x7b, 0x4e, false},
    {GLFW_KEY_KP_4, 0x4b, 0x6b, 0x6b, false},
    {GLFW_KEY_KP_5, 0x4c, 0x73, 0x73, false},
    {GLFW_KEY_KP_6, 0x4d, 0x74, 0x74, false},
    {GLFW_KEY_KP_ADD, 0x4e, 0x79, 0x7c, false},
    {GLFW_KEY_KP_1, 0x4f, 0x69, 0x69, false},
    {GLFW_KEY_KP_2, 0x50, 0x72, 0x72, false},
    {GLFW_KEY_KP_3, 0x51, 0x7a, 0x7a, false},
    {GLFW_KEY_KP_0, 0x52, 0x70, 0x70, false},
    {GLFW_KEY_KP_DECIMAL, 0x53, 0x71, 0x71, false},
    {GLFW_KEY_F11, 0x57, 0x78, 0x56, false},
    {GLFW_KEY_F12, 0x58, 0x07, 0x5e, false},
    {GLFW_KEY_F13, 0x5d, 0x2f, 0x7f, false},
    {GLFW_KEY_F14, 0x5e, 0x37, 0x80, false},
    {GLFW_KEY_F15, 0x5f, 0x3f, 0x81, false},
    {GLFW_KEY_INSERT, 0x52, 0x70, 0x67, true},
    {GLFW_KEY_UP, 0x48, 0x75, 0x63, true},
    {GLFW_KEY_DOWN, 0x50, 0x72, 0x60, true},
    {GLFW_KEY_LEFT, 0x4b, 0x6b, 0x61, true},
    {GLFW_KEY_RIGHT, 0x4d, 0x74, 0x6a, true},
};

const Keybinding *translate_key(int key) {
  for (const auto &keybinding : keymap) {
    if (keybinding.glfw_key == key) {
      return &keybinding;
    }
  }
  return nullptr;
}

void PS2State::ps2_reset_queue() {
  queue.rptr = 0;
  queue.wptr = 0;
  queue.count = 0;
}

void PS2State::ps2_queue_noirq(int b) {
  if (queue.count == PS2_QUEUE_SIZE) {
    return;
  }

  queue.data[queue.wptr] = b;
  if (++queue.wptr == PS2_QUEUE_SIZE)
    queue.wptr = 0;
  queue.count++;
}

void PS2State::ps2_raise_irq() { update_irq_(1); }

void PS2State::ps2_queue(int b) {
  ps2_queue_noirq(b);
  update_irq_(1);
}

void PS2State::ps2_queue_2(int b1, int b2) {
  if (PS2_QUEUE_SIZE - queue.count < 2) {
    return;
  }

  ps2_queue_noirq(b1);
  ps2_queue_noirq(b2);
  update_irq_(1);
}

void PS2State::ps2_queue_3(int b1, int b2, int b3) {
  if (PS2_QUEUE_SIZE - queue.count < 3) {
    return;
  }

  ps2_queue_noirq(b1);
  ps2_queue_noirq(b2);
  ps2_queue_noirq(b3);
  update_irq_(1);
}

void PS2State::ps2_queue_4(int b1, int b2, int b3, int b4) {
  if (PS2_QUEUE_SIZE - queue.count < 4) {
    return;
  }

  ps2_queue_noirq(b1);
  ps2_queue_noirq(b2);
  ps2_queue_noirq(b3);
  ps2_queue_noirq(b4);
  update_irq_(1);
}

PS2KbdState::PS2KbdState(std::function<void(int)> update_irq)
    : PS2State(update_irq) {
  ps2_kbd_reset();
}

PS2KbdState::~PS2KbdState() {}

/* keycode is the untranslated scancode in the current scancode set. */
void PS2KbdState::ps2_put_keycode(int keycode) {
  if (translate) {
    if (keycode == 0xf0) {
      need_high_bit = true;
    } else if (need_high_bit) {
      ps2_queue(translate_table[keycode] | 0x80);
      need_high_bit = false;
    } else {
      ps2_queue(translate_table[keycode]);
    }
  } else {
    ps2_queue(keycode);
  }
}

void PS2KbdState::ps2_keyboard_event(int key, int scancode, int action,
                                     int mods) {
  int mod;

  /* do not process events while disabled to prevent stream corruption */
  if (!scan_enabled) {
    return;
  }

  mod = ps2_modifier_bit(key);
  if (action == GLFW_PRESS) {
    modifiers |= mod;
  } else {
    modifiers &= ~mod;
  }

  if (scancode_set == 1) {
    if (key == GLFW_KEY_PAUSE) {
      if (modifiers & (MOD_CTRL_L | MOD_CTRL_R)) {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x46);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xc6);
        }
      } else {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe1);
          ps2_put_keycode(0x1d);
          ps2_put_keycode(0x45);
          ps2_put_keycode(0xe1);
          ps2_put_keycode(0x9d);
          ps2_put_keycode(0xc5);
        }
      }
    } else if (key == GLFW_KEY_PRINT_SCREEN) {
      if (modifiers & MOD_ALT_L) {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xb8);
          ps2_put_keycode(0x38);
          ps2_put_keycode(0x54);
        } else {
          ps2_put_keycode(0xd4);
          ps2_put_keycode(0xb8);
          ps2_put_keycode(0x38);
        }
      } else if (modifiers & MOD_ALT_R) {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xb8);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x38);
          ps2_put_keycode(0x54);
        } else {
          ps2_put_keycode(0xd4);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xb8);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x38);
        }
      } else if (modifiers &
                 (MOD_SHIFT_L | MOD_CTRL_L | MOD_SHIFT_R | MOD_CTRL_R)) {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x37);
        } else {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xb7);
        }
      } else {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x2a);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x37);
        } else {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xb7);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xaa);
        }
      }
    } else {
      auto binding = translate_key(key);
      if (binding) {
        uint16_t keycode = binding->at_set1_code;
        if (binding->meta) {
          ps2_put_keycode(keycode >> 8);
        }
        if (action != GLFW_PRESS) {
          keycode |= 0x80;
        }
        ps2_put_keycode(keycode & 0xff);
      } else {
        LOG(ERROR) << "ignoring input with glfw key: "
                   << glfwGetKeyName(key, scancode);
      }
    }
  } else if (scancode_set == 2) {
    if (key == GLFW_KEY_PAUSE) {
      if (modifiers & (MOD_CTRL_L | MOD_CTRL_R)) {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x7e);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x7e);
        }
      } else {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe1);
          ps2_put_keycode(0x14);
          ps2_put_keycode(0x77);
          ps2_put_keycode(0xe1);
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x14);
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x77);
        }
      }
    } else if (key == GLFW_KEY_PRINT_SCREEN) {
      if (modifiers & MOD_ALT_L) {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x11);
          ps2_put_keycode(0x11);
          ps2_put_keycode(0x84);
        } else {
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x84);
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x11);
          ps2_put_keycode(0x11);
        }
      } else if (modifiers & MOD_ALT_R) {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x11);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x11);
          ps2_put_keycode(0x84);
        } else {
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x84);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x11);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x11);
        }
      } else if (modifiers &
                 (MOD_SHIFT_L | MOD_CTRL_L | MOD_SHIFT_R | MOD_CTRL_R)) {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x7c);
        } else {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x7c);
        }
      } else {
        if (action == GLFW_PRESS) {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x12);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0x7c);
        } else {
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x7c);
          ps2_put_keycode(0xe0);
          ps2_put_keycode(0xf0);
          ps2_put_keycode(0x12);
        }
      }
    } else {
      auto binding = translate_key(key);
      if (binding) {
        uint16_t keycode = binding->at_set2_code;
        if (binding->meta) {
          ps2_put_keycode(keycode >> 8);
        }
        if (action != GLFW_PRESS) {
          keycode |= 0x80;
        }
        ps2_put_keycode(keycode & 0xff);
      } else {
        LOG(ERROR) << "ignoring input with glfw key: "
                   << glfwGetKeyName(key, scancode);
      }
    }
  } else if (scancode_set == 3) {
    auto binding = translate_key(key);
    if (binding) {
      uint16_t keycode = binding->at_set3_code;
      if (binding->meta) {
        ps2_put_keycode(keycode >> 8);
      }
      if (action != GLFW_PRESS) {
        keycode |= 0x80;
      }
      ps2_put_keycode(keycode & 0xff);
    } else {
      LOG(ERROR) << "ignoring input with glfw key: "
                 << glfwGetKeyName(key, scancode);
    }
  }
}

uint32_t PS2State::ps2_read_data() {
  PS2Queue *q;
  int val, index;

  q = &queue;
  if (q->count == 0) {
    /* NOTE: if no data left, we return the last keyboard one
       (needed for EMM386) */
    /* XXX: need a timer to do things correctly */
    index = q->rptr - 1;
    if (index < 0)
      index = PS2_QUEUE_SIZE - 1;
    val = q->data[index];
  } else {
    val = q->data[q->rptr];
    if (++q->rptr == PS2_QUEUE_SIZE)
      q->rptr = 0;
    q->count--;
    /* reading deasserts IRQ */
    update_irq_(0);
    /* reassert IRQs if data left */
    update_irq_(q->count != 0);
  }
  return val;
}

void PS2KbdState::ps2_set_ledstate(int ledstate) { this->ledstate = ledstate; }

void PS2KbdState::ps2_reset_keyboard() {
  scan_enabled = 1;
  scancode_set = 1;
  ps2_reset_queue();
  ps2_set_ledstate(0);
}

void PS2KbdState::ps2_write_keyboard(int val) {
  switch (write_cmd) {
  default:
  case -1:
    switch (val) {
    case 0x00:
      ps2_queue(KBD_REPLY_ACK);
      break;
    case 0x05:
      ps2_queue(KBD_REPLY_RESEND);
      break;
    case KBD_CMD_GET_ID:
      /* We emulate a MF2 AT keyboard here */
      if (translate)
        ps2_queue_3(KBD_REPLY_ACK, KBD_REPLY_ID, 0x41);
      else
        ps2_queue_3(KBD_REPLY_ACK, KBD_REPLY_ID, 0x83);
      break;
    case KBD_CMD_ECHO:
      ps2_queue(KBD_CMD_ECHO);
      break;
    case KBD_CMD_ENABLE:
      scan_enabled = 1;
      ps2_queue(KBD_REPLY_ACK);
      break;
    case KBD_CMD_SCANCODE:
    case KBD_CMD_SET_LEDS:
    case KBD_CMD_SET_RATE:
      write_cmd = val;
      ps2_queue(KBD_REPLY_ACK);
      break;
    case KBD_CMD_RESET_DISABLE:
      ps2_reset_keyboard();
      scan_enabled = 0;
      ps2_queue(KBD_REPLY_ACK);
      break;
    case KBD_CMD_RESET_ENABLE:
      ps2_reset_keyboard();
      scan_enabled = 1;
      ps2_queue(KBD_REPLY_ACK);
      break;
    case KBD_CMD_RESET:
      ps2_reset_keyboard();
      ps2_queue_2(KBD_REPLY_ACK, KBD_REPLY_POR);
      break;
    default:
      ps2_queue(KBD_REPLY_RESEND);
      break;
    }
    break;
  case KBD_CMD_SCANCODE:
    if (val == 0) {
      if (queue.count <= PS2_QUEUE_SIZE - 2) {
        ps2_queue(KBD_REPLY_ACK);
        ps2_put_keycode(scancode_set);
      }
    } else if (val >= 1 && val <= 3) {
      scancode_set = val;
      ps2_queue(KBD_REPLY_ACK);
    } else {
      ps2_queue(KBD_REPLY_RESEND);
    }
    write_cmd = -1;
    break;
  case KBD_CMD_SET_LEDS:
    ps2_set_ledstate(val);
    ps2_queue(KBD_REPLY_ACK);
    write_cmd = -1;
    break;
  case KBD_CMD_SET_RATE:
    ps2_queue(KBD_REPLY_ACK);
    write_cmd = -1;
    break;
  }
}

/* Set the scancode translation mode.
   0 = raw scancodes.
   1 = translated scancodes (used by qemu internally).  */

void PS2KbdState::ps2_keyboard_set_translation(int mode) { translate = mode; }

int PS2MouseState::ps2_mouse_send_packet() {
  const int needed = 3 + (mouse_type - 2);
  unsigned int b;
  int dx1, dy1, dz1;

  if (PS2_QUEUE_SIZE - queue.count < needed) {
    return 0;
  }

  dx1 = mouse_dx;
  dy1 = mouse_dy;
  dz1 = mouse_dz;
  /* XXX: increase range to 8 bits ? */
  if (dx1 > 127)
    dx1 = 127;
  else if (dx1 < -127)
    dx1 = -127;
  if (dy1 > 127)
    dy1 = 127;
  else if (dy1 < -127)
    dy1 = -127;
  b = 0x08 | ((dx1 < 0) << 4) | ((dy1 < 0) << 5) | (mouse_buttons & 0x07);
  ps2_queue_noirq(b);
  ps2_queue_noirq(dx1 & 0xff);
  ps2_queue_noirq(dy1 & 0xff);
  /* extra byte for IMPS/2 or IMEX */
  switch (mouse_type) {
  default:
    break;
  case 3:
    if (dz1 > 127)
      dz1 = 127;
    else if (dz1 < -127)
      dz1 = -127;
    ps2_queue_noirq(dz1 & 0xff);
    break;
  case 4:
    if (dz1 > 7)
      dz1 = 7;
    else if (dz1 < -7)
      dz1 = -7;
    b = (dz1 & 0x0f) | ((mouse_buttons & 0x18) << 1);
    ps2_queue_noirq(b);
    break;
  }

  ps2_raise_irq();

  /* update deltas */
  mouse_dx -= dx1;
  mouse_dy -= dy1;
  mouse_dz -= dz1;

  return 1;
}

void PS2MouseState::ps2_mouse_move(double x, double y) {
  /* check if deltas are recorded when disabled */
  if (!(mouse_status & MOUSE_STATUS_ENABLED))
    return;

  mouse_dx = x;
  mouse_dy = y;
}

void PS2MouseState::ps2_mouse_button(int button, int action, int mods) {
  /* check if deltas are recorded when disabled */
  if (!(mouse_status & MOUSE_STATUS_ENABLED))
    return;

  uint8_t translated_buttons = 0;
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    translated_buttons |= PS2_MOUSE_BUTTON_LEFT;
  } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
    translated_buttons |= PS2_MOUSE_BUTTON_MIDDLE;
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    translated_buttons |= PS2_MOUSE_BUTTON_RIGHT;
  }
  if (action == GLFW_PRESS) {
    mouse_buttons |= translated_buttons;
  } else {
    mouse_buttons &= ~translated_buttons;
  }
}

void PS2MouseState::ps2_mouse_scroll(double xoffset, double yoffset) {
  mouse_dz = yoffset;
}

void PS2MouseState::ps2_mouse_sync() {
  /* do not sync while disabled to prevent stream corruption */
  if (!(mouse_status & MOUSE_STATUS_ENABLED)) {
    return;
  }

  if (!(mouse_status & MOUSE_STATUS_REMOTE)) {
    /* if not remote, send event. Multiple events are sent if
       too big deltas */
    while (ps2_mouse_send_packet()) {
      if (mouse_dx == 0 && mouse_dy == 0 && mouse_dz == 0)
        break;
    }
  }
}

void PS2MouseState::ps2_mouse_fake_event() {
  mouse_dx++;
  ps2_mouse_sync();
}

void PS2MouseState::ps2_write_mouse(int val) {

#ifdef DEBUG_MOUSE
  printf("kbd: write mouse 0x%02x\n", val);
#endif
  switch (write_cmd) {
  default:
  case -1:
    /* mouse command */
    if (mouse_wrap) {
      if (val == AUX_RESET_WRAP) {
        mouse_wrap = 0;
        ps2_queue(AUX_ACK);
        return;
      } else if (val != AUX_RESET) {
        ps2_queue(val);
        return;
      }
    }
    switch (val) {
    case AUX_SET_SCALE11:
      mouse_status &= ~MOUSE_STATUS_SCALE21;
      ps2_queue(AUX_ACK);
      break;
    case AUX_SET_SCALE21:
      mouse_status |= MOUSE_STATUS_SCALE21;
      ps2_queue(AUX_ACK);
      break;
    case AUX_SET_STREAM:
      mouse_status &= ~MOUSE_STATUS_REMOTE;
      ps2_queue(AUX_ACK);
      break;
    case AUX_SET_WRAP:
      mouse_wrap = 1;
      ps2_queue(AUX_ACK);
      break;
    case AUX_SET_REMOTE:
      mouse_status |= MOUSE_STATUS_REMOTE;
      ps2_queue(AUX_ACK);
      break;
    case AUX_GET_TYPE:
      ps2_queue_2(AUX_ACK, mouse_type);
      break;
    case AUX_SET_RES:
    case AUX_SET_SAMPLE:
      write_cmd = val;
      ps2_queue(AUX_ACK);
      break;
    case AUX_GET_SCALE:
      ps2_queue_4(AUX_ACK, mouse_status, mouse_resolution, mouse_sample_rate);
      break;
    case AUX_POLL:
      ps2_queue(AUX_ACK);
      ps2_mouse_send_packet();
      break;
    case AUX_ENABLE_DEV:
      mouse_status |= MOUSE_STATUS_ENABLED;
      ps2_queue(AUX_ACK);
      break;
    case AUX_DISABLE_DEV:
      mouse_status &= ~MOUSE_STATUS_ENABLED;
      ps2_queue(AUX_ACK);
      break;
    case AUX_SET_DEFAULT:
      mouse_sample_rate = 100;
      mouse_resolution = 2;
      mouse_status = 0;
      ps2_queue(AUX_ACK);
      break;
    case AUX_RESET:
      mouse_sample_rate = 100;
      mouse_resolution = 2;
      mouse_status = 0;
      mouse_type = 0;
      ps2_reset_queue();
      ps2_queue_3(AUX_ACK, 0xaa, mouse_type);
      break;
    default:
      break;
    }
    break;
  case AUX_SET_SAMPLE:
    mouse_sample_rate = val;
    /* detect IMPS/2 or IMEX */
    switch (mouse_detect_state) {
    default:
    case 0:
      if (val == 200)
        mouse_detect_state = 1;
      break;
    case 1:
      if (val == 100)
        mouse_detect_state = 2;
      else if (val == 200)
        mouse_detect_state = 3;
      else
        mouse_detect_state = 0;
      break;
    case 2:
      if (val == 80)
        mouse_type = 3; /* IMPS/2 */
      mouse_detect_state = 0;
      break;
    case 3:
      if (val == 80)
        mouse_type = 4; /* IMEX */
      mouse_detect_state = 0;
      break;
    }
    ps2_queue(AUX_ACK);
    write_cmd = -1;
    break;
  case AUX_SET_RES:
    mouse_resolution = val;
    ps2_queue(AUX_ACK);
    write_cmd = -1;
    break;
  }
}

void PS2State::ps2_common_reset() {
  write_cmd = -1;
  ps2_reset_queue();
  update_irq_(0);
}

void PS2State::ps2_common_post_load() {
  PS2Queue *q = &queue;
  uint8_t i, size;
  uint8_t tmp_data[PS2_QUEUE_SIZE];

  /* set the useful data buffer queue size, < PS2_QUEUE_SIZE */
  size = q->count;
  if (q->count < 0) {
    size = 0;
  } else if (q->count > PS2_QUEUE_SIZE) {
    size = PS2_QUEUE_SIZE;
  }

  /* move the queue elements to the start of data array */
  for (i = 0; i < size; i++) {
    if (q->rptr < 0 || q->rptr >= sizeof(q->data)) {
      q->rptr = 0;
    }
    tmp_data[i] = q->data[q->rptr++];
  }
  memcpy(q->data, tmp_data, size);

  /* reset rptr/wptr/count */
  q->rptr = 0;
  q->wptr = (size == PS2_QUEUE_SIZE) ? 0 : size;
  q->count = size;
}

PS2State::~PS2State() {}

PS2State::PS2State(std::function<void(int)> update_irq)
    : update_irq_(update_irq) {
  ps2_reset_queue();
}

void PS2KbdState::ps2_kbd_reset() {
  ps2_common_reset();
  scan_enabled = 1;
  translate = 0;
  scancode_set = 1;
  modifiers = 0;
}

void PS2MouseState::ps2_mouse_reset() {
  ps2_common_reset();
  mouse_status = 0;
  mouse_resolution = 0;
  mouse_sample_rate = 0;
  mouse_wrap = 0;
  mouse_type = 0;
  mouse_detect_state = 0;
  mouse_dx = 0;
  mouse_dy = 0;
  mouse_dz = 0;
  mouse_buttons = 0;
}

PS2MouseState::~PS2MouseState() {}

bool PS2KbdState::ps2_keyboard_ledstate_needed(void *opaque) {
  return ledstate != 0; /* 0 is default state */
}

int PS2KbdState::ps2_kbd_ledstate_post_load(int version_id) {
  //  kbd_put_ledstate(ledstate);
  return 0;
}

bool PS2KbdState::ps2_keyboard_need_high_bit_needed() {
  return need_high_bit != 0; /* 0 is the usual state */
}

int PS2KbdState::ps2_kbd_post_load(int version_id) {
  if (version_id == 2)
    scancode_set = 2;

  ps2_common_post_load();

  return 0;
}
