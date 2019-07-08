#pragma once

#include "cpu.h"

#include <cstdint>

class InterruptController;
class PS2KbdState;
class PS2MouseState;

class I8042 {
public:
  explicit I8042(InterruptController *irq_controller);
  ~I8042();

  uint8_t ReadByte(cpuaddr_t addr);
  void StoreByte(cpuaddr_t addr, uint8_t value);

  PS2MouseState* mouse() const { return mouse_.get(); }
  PS2KbdState *kbd() const { return kbd_.get(); }

private:
  void kbd_update_irq();
  void kbd_update_kbd_irq(int level);
  void kbd_update_aux_irq(int level);
  void kbd_queue(int b, int aux);
  void outport_write(unsigned int val);
  void kbd_write_command(unsigned int addr, uint64_t val);
  uint8_t kbd_read_status(cpuaddr_t addr);
  void kbd_write_data(unsigned int addr, uint8_t val);
  void kbd_reset();
  uint8_t kbd_outport_default();
  bool kbd_outport_needed(void * opaque);
  uint8_t kbd_read_data(cpuaddr_t addr);

  InterruptController *irq_controller_;

  std::unique_ptr<PS2KbdState> kbd_;
  std::unique_ptr<PS2MouseState> mouse_;
  cpuaddr_t mask_;

  uint8_t write_cmd_ = 0; /* if non zero, write data to port 60 is expected */
  uint8_t status_ = 0;
  uint8_t mode_ = 0;
  uint8_t outport_ = 0;
  /* Bitmask of devices with data available.  */
  uint8_t pending_ = 0;

};