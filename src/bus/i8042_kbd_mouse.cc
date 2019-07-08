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
#include "bus/i8042_kbd_mouse.h"

#include <glog/logging.h>

#include "bus/int_controller.h"
#include "bus/ps2_kbdmouse.h"
#include "cpu.h"

/*	Keyboard Controller Commands */
#define KBD_CCMD_READ_MODE 0x20     /* Read mode bits */
#define KBD_CCMD_WRITE_MODE 0x60    /* Write mode bits */
#define KBD_CCMD_GET_VERSION 0xA1   /* Get controller version */
#define KBD_CCMD_MOUSE_DISABLE 0xA7 /* Disable mouse interface */
#define KBD_CCMD_MOUSE_ENABLE 0xA8  /* Enable mouse interface */
#define KBD_CCMD_TEST_MOUSE 0xA9    /* Mouse interface test */
#define KBD_CCMD_SELF_TEST 0xAA     /* Controller self test */
#define KBD_CCMD_KBD_TEST 0xAB      /* Keyboard interface test */
#define KBD_CCMD_KBD_DISABLE 0xAD   /* Keyboard interface disable */
#define KBD_CCMD_KBD_ENABLE 0xAE    /* Keyboard interface enable */
#define KBD_CCMD_READ_INPORT 0xC0   /* read input port */
#define KBD_CCMD_READ_OUTPORT 0xD0  /* read output port */
#define KBD_CCMD_WRITE_OUTPORT 0xD1 /* write output port */
#define KBD_CCMD_WRITE_OBUF 0xD2
#define KBD_CCMD_WRITE_AUX_OBUF                                                \
  0xD3                            /* Write to output buffer as if              \
                                     initiated by the auxiliary device */
#define KBD_CCMD_WRITE_MOUSE 0xD4 /* Write the following byte to the mouse */
#define KBD_CCMD_DISABLE_A20 0xDD /* HP vectra only ? */
#define KBD_CCMD_ENABLE_A20 0xDF  /* HP vectra only ? */
#define KBD_CCMD_PULSE_BITS_3_0                                                \
  0xF0 /* Pulse bits 3-0 of the output port P2.                                \
        */
#define KBD_CCMD_RESET                                                         \
  0xFE                      /* Pulse bit 0 of the output port P2 = CPU reset.  \
                             */
#define KBD_CCMD_NO_OP 0xFF /* Pulse no bits of the output port P2. */

/* Keyboard Commands */
#define KBD_CMD_SET_LEDS 0xED /* Set keyboard leds */
#define KBD_CMD_ECHO 0xEE
#define KBD_CMD_GET_ID 0xF2        /* get keyboard ID */
#define KBD_CMD_SET_RATE 0xF3      /* Set typematic rate */
#define KBD_CMD_ENABLE 0xF4        /* Enable scanning */
#define KBD_CMD_RESET_DISABLE 0xF5 /* reset and disable scanning */
#define KBD_CMD_RESET_ENABLE 0xF6  /* reset and enable scanning */
#define KBD_CMD_RESET 0xFF         /* Reset */

/* Keyboard Replies */
#define KBD_REPLY_POR 0xAA    /* Power on reset */
#define KBD_REPLY_ACK 0xFA    /* Command ACK */
#define KBD_REPLY_RESEND 0xFE /* Command NACK, send the cmd again */

/* Status Register Bits */
#define KBD_STAT_OBF 0x01       /* Keyboard output buffer full */
#define KBD_STAT_IBF 0x02       /* Keyboard input buffer full */
#define KBD_STAT_SELFTEST 0x04  /* Self test successful */
#define KBD_STAT_CMD 0x08       /* Last write was a command write (0=data) */
#define KBD_STAT_UNLOCKED 0x10  /* Zero if keyboard locked */
#define KBD_STAT_MOUSE_OBF 0x20 /* Mouse output buffer full */
#define KBD_STAT_GTO 0x40       /* General receive/xmit timeout */
#define KBD_STAT_PERR 0x80      /* Parity error */

/* Controller Mode Register Bits */
#define KBD_MODE_KBD_INT 0x01   /* Keyboard data generate IRQ1 */
#define KBD_MODE_MOUSE_INT 0x02 /* Mouse data generate IRQ12 */
#define KBD_MODE_SYS 0x04       /* The system flag (?) */
#define KBD_MODE_NO_KEYLOCK                                                    \
  0x08 /* The keylock doesn't affect the keyboard if set */
#define KBD_MODE_DISABLE_KBD 0x10   /* Disable keyboard interface */
#define KBD_MODE_DISABLE_MOUSE 0x20 /* Disable mouse interface */
#define KBD_MODE_KCC 0x40           /* Scan code conversion to PC format */
#define KBD_MODE_RFU 0x80

/* Output Port Bits */
#define KBD_OUT_RESET 0x01     /* 1=normal mode, 0=reset */
#define KBD_OUT_A20 0x02       /* x86 only */
#define KBD_OUT_OBF 0x10       /* Keyboard output buffer full */
#define KBD_OUT_MOUSE_OBF 0x20 /* Mouse output buffer full */

/* OSes typically write 0xdd/0xdf to turn the A20 line off and on.
 * We make the default value of the outport include these four bits,
 * so that the subsection is rarely necessary.
 */
#define KBD_OUT_ONES 0xcc

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

#define KBD_PENDING_KBD 1
#define KBD_PENDING_AUX 2

I8042::I8042(InterruptController *irq_controller)
    : irq_controller_(irq_controller),
      kbd_(std::make_unique<PS2KbdState>(
          [this](int l) { this->kbd_update_kbd_irq(l); })),
      mouse_(std::make_unique<PS2MouseState>(
          [this](int l) { this->kbd_update_aux_irq(l); })),
      mask_(0x04) {
  kbd_reset();
}

I8042::~I8042() {}

/* update irq and KBD_STAT_[MOUSE_]OBF */
/* XXX: not generating the irqs if KBD_MODE_DISABLE_KBD is set may be
   incorrect, but it avoids having to simulate exact delays */
void I8042::kbd_update_irq() {
  int irq_kbd_level, irq_mouse_level;

  irq_kbd_level = 0;
  irq_mouse_level = 0;
  status_ &= ~(KBD_STAT_OBF | KBD_STAT_MOUSE_OBF);
  outport_ &= ~(KBD_OUT_OBF | KBD_OUT_MOUSE_OBF);
  if (pending_) {
    status_ |= KBD_STAT_OBF;
    outport_ |= KBD_OUT_OBF;
    /* kbd data takes priority over aux data.  */
    if (pending_ == KBD_PENDING_AUX) {
      status_ |= KBD_STAT_MOUSE_OBF;
      outport_ |= KBD_OUT_MOUSE_OBF;
      if (mode_ & KBD_MODE_MOUSE_INT)
        irq_mouse_level = 1;
    } else {
      // TODO: not sure why int mode keeps getting turned off.
      //      if ((mode_ & KBD_MODE_KBD_INT) && !(mode_ & KBD_MODE_DISABLE_KBD))
      irq_kbd_level = 1;
    }
  }
  irq_controller_->SetKeyboard(irq_kbd_level);
  irq_controller_->SetMouse(irq_mouse_level);
}

void I8042::kbd_update_kbd_irq(int level) {
  if (level)
    pending_ |= KBD_PENDING_KBD;
  else
    pending_ &= ~KBD_PENDING_KBD;
  kbd_update_irq();
}

void I8042::kbd_update_aux_irq(int level) {
  if (level)
    pending_ |= KBD_PENDING_AUX;
  else
    pending_ &= ~KBD_PENDING_AUX;
  kbd_update_irq();
}

uint8_t I8042::kbd_read_status(cpuaddr_t addr) { return status_; }

void I8042::kbd_queue(int b, int aux) {
  if (aux)
    mouse_->ps2_queue(b);
  else
    kbd_->ps2_queue(b);
}

void I8042::outport_write(uint32_t val) { outport_ = val; }

void I8042::kbd_write_command(cpuaddr_t addr, uint64_t val) {
  /* Bits 3-0 of the output port P2 of the keyboard controller may be pulsed
   * low for approximately 6 micro seconds. Bits 3-0 of the KBD_CCMD_PULSE
   * command specify the output port bits to be pulsed.
   * 0: Bit should be pulsed. 1: Bit should not be modified.
   * The only useful version of this command is pulsing bit 0,
   * which does a CPU reset.
   */
  if ((val & KBD_CCMD_PULSE_BITS_3_0) == KBD_CCMD_PULSE_BITS_3_0) {
    if (!(val & 1))
      val = KBD_CCMD_RESET;
    else
      val = KBD_CCMD_NO_OP;
  }

  switch (val) {
  case KBD_CCMD_READ_MODE:
    kbd_queue(mode_, 0);
    break;
  case KBD_CCMD_WRITE_MODE:
  case KBD_CCMD_WRITE_OBUF:
  case KBD_CCMD_WRITE_AUX_OBUF:
  case KBD_CCMD_WRITE_MOUSE:
  case KBD_CCMD_WRITE_OUTPORT:
    write_cmd_ = val;
    break;
  case KBD_CCMD_MOUSE_DISABLE:
    mode_ |= KBD_MODE_DISABLE_MOUSE;
    break;
  case KBD_CCMD_MOUSE_ENABLE:
    mode_ &= ~KBD_MODE_DISABLE_MOUSE;
    break;
  case KBD_CCMD_TEST_MOUSE:
    kbd_queue(0x00, 0);
    break;
  case KBD_CCMD_SELF_TEST:
    status_ |= KBD_STAT_SELFTEST;
    kbd_queue(0x55, 0);
    break;
  case KBD_CCMD_KBD_TEST:
    kbd_queue(0x00, 0);
    break;
  case KBD_CCMD_KBD_DISABLE:
    mode_ |= KBD_MODE_DISABLE_KBD;
    kbd_update_irq();
    break;
  case KBD_CCMD_KBD_ENABLE:
    mode_ &= ~KBD_MODE_DISABLE_KBD;
    kbd_update_irq();
    break;
  case KBD_CCMD_READ_INPORT:
    kbd_queue(0x80, 0);
    break;
  case KBD_CCMD_READ_OUTPORT:
    kbd_queue(outport_, 0);
    break;
  case KBD_CCMD_RESET:
    LOG(ERROR) << "Reset key hit"; // TODO: reset?
    break;
  case KBD_CCMD_NO_OP:
    /* ignore that */
    break;
  default:
    LOG(ERROR) << "unsupported keyboard cmd=" << std::hex << (int)val;
    break;
  }
}

void I8042::kbd_write_data(cpuaddr_t addr, uint8_t val) {
  switch (write_cmd_) {
  case 0:
    kbd_->ps2_write_keyboard(val);
    break;
  case KBD_CCMD_WRITE_MODE:
    mode_ = val;
    kbd_->ps2_keyboard_set_translation((mode_ & KBD_MODE_KCC) != 0);
    /* ??? */
    kbd_update_irq();
    break;
  case KBD_CCMD_WRITE_OBUF:
    kbd_queue(val, 0);
    break;
  case KBD_CCMD_WRITE_AUX_OBUF:
    kbd_queue(val, 1);
    break;
  case KBD_CCMD_WRITE_OUTPORT:
    outport_write(val);
    break;
  case KBD_CCMD_WRITE_MOUSE:
    mouse_->ps2_write_mouse(val);
    break;
  default:
    break;
  }
  write_cmd_ = 0;
}

uint8_t I8042::kbd_read_data(cpuaddr_t addr) {
  uint32_t val;

  if (pending_ == KBD_PENDING_AUX)
    val = mouse_->ps2_read_data();
  else
    val = kbd_->ps2_read_data();

  return val;
}

void I8042::kbd_reset() {
  mode_ = KBD_MODE_KBD_INT | KBD_MODE_MOUSE_INT;
  status_ = KBD_STAT_CMD | KBD_STAT_UNLOCKED;
  outport_ = KBD_OUT_RESET | KBD_OUT_A20 | KBD_OUT_ONES;
}

uint8_t I8042::kbd_outport_default() {
  return KBD_OUT_RESET | KBD_OUT_A20 | KBD_OUT_ONES |
         (status_ & KBD_STAT_OBF ? KBD_OUT_OBF : 0) |
         (status_ & KBD_STAT_MOUSE_OBF ? KBD_OUT_MOUSE_OBF : 0);
}

bool I8042::kbd_outport_needed(void *opaque) {
  return outport_ != kbd_outport_default();
}

/* Memory mapped interface */
uint8_t I8042::ReadByte(cpuaddr_t addr) {

  if (addr & mask_)
    return kbd_read_status(addr) & 0xff;
  else
    return kbd_read_data(addr) & 0xff;
}

void I8042::StoreByte(cpuaddr_t addr, uint8_t value) {

  if (addr & mask_)
    kbd_write_command(0, value & 0xff);
  else
    kbd_write_data(0, value & 0xff);
}
