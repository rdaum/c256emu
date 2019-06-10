#pragma once

#include <stdint.h>

class System;

// TODO: polarity/edge/mask
class InterruptController {
public:
  explicit InterruptController(System *sys);

  // Raise various specific interrupts.
  void RaiseFrameStart();
  void RaiseKeyboard();
  void LowerKeyboard();
  void RaiseCH376();
  void LowerCH376();

  // SystemBusDevice implementation.
  void StoreByte(uint32_t addr, uint8_t v);
  uint8_t ReadByte(uint32_t addr);

private:
  union InterruptSet1 {
    struct {
      bool UNUSED : 1; // Always 1
      bool vicky0 : 1; // Start of frame
      bool vicky1 : 1; // Line Interrupt
      bool timer_0 : 1;
      bool timer_1 : 1;
      bool timer_2 : 1;
      bool rtc : 1;
      bool lpc : 1;

      bool Pending() const {
        return vicky0 || vicky1 || timer_0 || timer_1 || timer_2 || rtc || lpc;
      }
    } ints;
    uint8_t val;
  };

  union InterruptSet2 {
    struct {
      ;
      bool kbd : 1;
      bool vicky2 : 1; // Sprite collision
      bool vicky3 : 1; // tile collision
      bool lpc_com2 : 1;
      bool lpc_com1 : 1;
      bool lpc_midi : 1; // mpu-401
      bool lpc_lpt : 1;
      bool ch376 : 1; // sd card

      bool Pending() const {
        return kbd || vicky2 || vicky3 || lpc_com2 || lpc_com1 || lpc_midi ||
               lpc_lpt || ch376;
      }
    } ints;
    uint8_t val;
  };
  union InterruptSet3 {
    struct {
      bool opl2_left_channel : 1;
      bool opl2_right_channel : 1;
      bool beatrix : 1;
      bool gavin_dma : 1;
      bool UNUSED0 : 1; // Always 1
      bool dac_hot_plug : 1;
      bool expansion : 1;
      bool UNUSED1 : 1; // Always 1

      bool Pending() const {
        return opl2_left_channel || opl2_right_channel || beatrix ||
               gavin_dma || dac_hot_plug || expansion;
      }
    } ints;
    uint8_t val;
  };

  System *sys_;

  InterruptSet1 pending_reg0_;
  InterruptSet2 pending_reg1_;
  InterruptSet3 pending_reg2_;

  InterruptSet1 polarity_reg0_;
  InterruptSet2 polarity_reg1_;
  InterruptSet3 polarity_reg2_;

  InterruptSet1 edge_reg0_;
  InterruptSet2 edge_reg1_;
  InterruptSet3 edge_reg2_;

  InterruptSet1 mask_reg0_;
  InterruptSet2 mask_reg1_;
  InterruptSet3 mask_reg2_;
};