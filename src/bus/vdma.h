#pragma once

#include <cstdint>

#include "bus/register_utils.h"

class InterruptController;

class VDMA {
 public:
  VDMA(uint8_t* vram, InterruptController *int_controller);

  void OnFrameStart();
  void StoreByte(uint32_t addr, uint8_t v);
  uint8_t ReadByte(uint32_t addr);

 private:
  std::vector<Reg> registers_;
  std::vector<Reg> read_only_registers_;

  uint8_t* vram_;
  InterruptController *int_controller_;
  
  union {
    uint8_t v;
    struct {
      bool enable : 1;
      bool linear_block : 1;  // linear or block
      bool trf_fill : 1;      // transfer or fill
      bool int_enable : 1;    // generate interrupt on completion
      bool start_trf : 1;     // begin?  clear when ready to start again
      uint8_t unused : 4;
    } reg;
  } ctrl_reg_;

  uint8_t write_byte_;  // write only

  union {
    uint8_t v;
    struct {
      bool size_err;     // if 1 size is invalid
      bool dst_add_err;  // if 1, dest addr invalid
      bool src_add_err;  // if 1 src addr invalid
      bool vmda_ips;     // 1 if in progress (no cpu access to mem)
    } reg;
  } status_reg_;

  uint32_t src_addr_;
  uint32_t dst_addr_;

  union {
    struct {
      uint32_t size : 24;
      uint8_t ignored : 8;
    } linear;
    struct {
      uint16_t x_size;
      uint16_t y_size;
    } block;
  } size_;

  uint16_t src_stride_;
  uint16_t dst_stride_;
};
