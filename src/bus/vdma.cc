#include "bus/vdma.h"

#include <glog/logging.h>

#include "bus/int_controller.h"

namespace {

constexpr uint32_t kVdmaControlReg = 0xaf0400;

constexpr uint32_t kVdmaByte2Write =
    0xaf0401;  // same address as below; one for read, one for write
constexpr uint32_t kVmdaStatusReg = 0xaf0401;
constexpr uint32_t kVdmaSrcAddy = 0xaf0402;
constexpr uint32_t kVdmaDstAddr = 0xaf0405;
constexpr uint32_t kVdmaTransferSize = 0xaf0408;
constexpr uint32_t kVdmaSrcStride = 0xaf040c;
constexpr uint32_t kVdmaDstStride = 0xaf040e;

void BitBlt(uint8_t* src,
            uint8_t* dst,
            uint32_t src_stride,
            uint32_t dst_stride,
            uint16_t x_size,
            uint16_t y_size) {
  // TODO error handling
  for (uint16_t y = 0; y < y_size; y++) {
    uint8_t* src_row = &src[(y * src_stride)];
    uint8_t* dst_row = &dst[(y * dst_stride)];
    for (uint16_t x = 0; x < x_size; x++) {
      dst_row[x] = src_row[x];
    }
  }
}

void FillBlt(uint8_t* dst,
             uint32_t dst_stride,
             uint16_t x_size,
             uint16_t y_size,
             uint8_t fill_v) {
  // TODO error handling
  for (uint16_t y = 0; y < y_size; y++) {
    uint8_t* row = &dst[(y * dst_stride)];
    for (uint16_t x = 0; x < x_size; x++) {
      dst[(y * dst_stride) + x] = fill_v;
    }
  }
}
}  // namespace

VDMA::VDMA(uint8_t* vram, InterruptController *int_controller)
    : registers_({
          {kVdmaControlReg, &ctrl_reg_.v, 1},
          {kVdmaSrcAddy, &src_addr_, 3},
          {kVdmaDstAddr, &dst_addr_, 3},
          {kVdmaTransferSize, &size_, 4},
          {kVdmaSrcStride, &src_stride_, 2},
          {kVdmaDstStride, &dst_stride_, 2},
      }),
      vram_(vram), int_controller_(int_controller) {}

void VDMA::OnFrameStart() {
  if (!ctrl_reg_.reg.enable)
    return;
  if (!ctrl_reg_.reg.start_trf)
    return;
  if (!ctrl_reg_.reg.linear_block) {
    status_reg_.reg.vmda_ips = true;
    if (ctrl_reg_.reg.trf_fill) {
      FillBlt(&vram_[dst_addr_], dst_stride_, size_.block.x_size,
              size_.block.y_size, write_byte_);
    } else {
      BitBlt(&vram_[src_addr_], &vram_[dst_addr_], src_stride_, dst_stride_,
             size_.block.x_size, size_.block.y_size);
    }
    status_reg_.reg.vmda_ips = false;
  } else {
    // TODO linear ("1d") transfer
  }
  if (ctrl_reg_.reg.int_enable) {
    int_controller_->RaiseVDMATransferComplete();
  }

}

void VDMA::StoreByte(uint32_t addr, uint8_t v) {
  if (StoreRegister(addr, v, registers_)) {
    return;
  }

  // Write-only register shares same 'address' as the read-only status reg.
  if (addr == kVdmaByte2Write) {
    write_byte_ = v;
    return;
  }
  LOG(ERROR) << "Unknown VDMA register write: " << std::hex << addr
             << " := " << std::hex << (int)v;
}

uint8_t VDMA::ReadByte(uint32_t addr) {
  uint8_t v;
  if (ReadRegister(addr, registers_, &v)) {
    return v;
  }
  if (addr == kVmdaStatusReg) {
    return status_reg_.v;
  }
  LOG(ERROR) << "Unknown VDMA register read: " << std::hex << addr;
  return 0;
}
