
#include <cstdint>
#include <functional>

struct PS2Queue {
  /* Keep the data array 256 bytes long, which compatibility
   with older qemu versions. */
  uint8_t data[256];
  int rptr, wptr, count;
};

class PS2State {
public:
  explicit PS2State(std::function<void(int)> update_irq);

  virtual ~PS2State();

  virtual void ps2_raise_irq();
  virtual void ps2_queue(int b);
  virtual void ps2_queue_noirq(int b);
  virtual void ps2_reset_queue();
  virtual void ps2_queue_2(int b1, int b2);
  virtual void ps2_queue_3(int b1, int b2, int b3);
  virtual void ps2_queue_4(int b1, int b2, int b3, int b4);
  virtual uint32_t ps2_read_data();
  virtual void ps2_common_reset();
  virtual void ps2_common_post_load();

  int32_t write_cmd;

private:
  std::function<void(int)> update_irq_;

protected:
  PS2Queue queue;
};

class PS2KbdState : public PS2State {
public:
  explicit PS2KbdState(std::function<void(int)> update_irq);

  virtual ~PS2KbdState();
  void ps2_keyboard_event(int key, int scancode, int action, int mods);

  bool ps2_keyboard_need_high_bit_needed();
  int ps2_kbd_post_load(int version_id);
  int ps2_kbd_ledstate_post_load(int version_id);
  bool ps2_keyboard_ledstate_needed(void *opaque);
  void ps2_kbd_reset();
  void ps2_put_keycode(int keycode);
  void ps2_set_ledstate(int ledstate);
  void ps2_reset_keyboard();

  void ps2_write_keyboard(int val);
  void ps2_keyboard_set_translation(int mode);

private:
  int scan_enabled;
  int translate;
  int scancode_set; /* 1=XT, 2=AT, 3=PS/2 */
  int ledstate;
  bool need_high_bit;
  unsigned int modifiers; /* bitmask of MOD_* constants above */
};

struct PS2MouseState : public PS2State {
public:
  explicit PS2MouseState(std::function<void(int)> update_irq)
      : PS2State(update_irq) {}
  virtual ~PS2MouseState();

  void ps2_mouse_move(double x, double y);
  void ps2_mouse_button(int button, int action, int mods);
  void ps2_mouse_scroll(double xoffset, double yoffset);

  void ps2_mouse_reset();
  void ps2_write_mouse(int val);
  int ps2_mouse_send_packet();
  void ps2_mouse_fake_event();

private:
  void ps2_mouse_sync();

  uint8_t mouse_status;
  uint8_t mouse_resolution;
  uint8_t mouse_sample_rate;
  uint8_t mouse_wrap;
  uint8_t mouse_type; /* 0 = PS2, 3 = IMPS/2, 4 = IMEX */
  uint8_t mouse_detect_state;
  int mouse_dx; /* current values, needed for 'poll' mode */
  int mouse_dy;
  int mouse_dz;
  uint8_t mouse_buttons;
};
