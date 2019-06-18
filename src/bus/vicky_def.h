#pragma once

#include <stdint.h>

constexpr uint32_t kMasterCtrlReg(0x000);

// Control Bits Fields
constexpr uint8_t Mstr_Ctrl_Text_Mode_En = 0x01;  // Enable the Text Mode
constexpr uint8_t Mstr_Ctrl_Text_Overlay = 0x02;
constexpr uint8_t Mstr_Ctrl_Graph_Mode_En = 0x04;
constexpr uint8_t Mstr_Ctrl_Bitmap_En = 0x08;
constexpr uint8_t Mstr_Ctrl_TileMap_En = 0x10;
constexpr uint8_t Mstr_Ctrl_Sprite_En = 0x20;
constexpr uint8_t Mstr_Ctrl_GAMMA_En = 0x40;
constexpr uint8_t Mstr_Ctrl_Disable_Vid = 0x80;

// Reserved - TBD
constexpr uint32_t VKY_RESERVED_00(0x0002);
constexpr uint32_t VKY_RESERVED_01(0x0003);

constexpr uint32_t kBorderCtrlReg(0x0004);
constexpr uint8_t Border_Ctrl_Enable = 0x0;
constexpr uint32_t kBorderColour(0x0005);


constexpr uint32_t kBackgroundColour(0x0008);

constexpr uint32_t kCursorCtrlReg(0x0010);
constexpr uint8_t Vky_Cursor_Enable = 0x01;
constexpr uint8_t Vky_Cursor_FONT_Page0 = 0x08;
constexpr uint8_t Vky_Cursor_FONT_Page1 = 0x10;
constexpr uint32_t VKY_TXT_RESERVED(0x0011);

constexpr uint32_t kCursorChar(0x0012);
constexpr uint32_t kCursorColour(0x0013);
constexpr uint32_t kCursorX(0x0014);
constexpr uint32_t kCursorY(0x0016);

constexpr uint32_t VKY_INFO_CHIP_NUM_L(0x001C);
constexpr uint32_t VKY_INFO_CHIP_NUM_H(0x001D);
constexpr uint32_t VKY_INFO_CHIP_VER_L(0x001E);
constexpr uint32_t VKY_INFO_CHIP_VER_H(0x001F);

//
// Bit Field Definition for the Control Register
constexpr uint8_t TILE_Enable = 0x01;
constexpr uint8_t TILE_LUT0 = 0x02;
constexpr uint8_t TILE_LUT1 = 0x04;
constexpr uint8_t TILE_LUT2 = 0x08;
constexpr uint8_t TILE_Scroll_X_Enable = 0x10;
constexpr uint8_t TILE_Scroll_Y_Enable = 0x20;
constexpr uint8_t TILESHEET_256x256_En = 0x80; // 0 -> Sequential, 1-> 256x256 Tile Sheet Striding
//
constexpr uint32_t kTileRegistersBegin(0x0100);
constexpr uint32_t kTileRegistersEnd(0x011F);
constexpr uint8_t kNumTileRegisters(0x08);

// Bitmap Registers
constexpr uint32_t kBitmapCtrlReg(0x0140);
constexpr uint32_t kBitmapStartAddress(0x0141);

constexpr uint32_t BM_X_SIZE_L(0x0144);
constexpr uint32_t BM_X_SIZE_H(0x0145);
constexpr uint32_t BM_Y_SIZE_L(0x0146);
constexpr uint32_t BM_Y_SIZE_H(0x0147);
constexpr uint32_t BM_RESERVED_0(0x0148);
constexpr uint32_t BM_RESERVED_1(0x0149);
constexpr uint32_t BM_RESERVED_2(0x014A);
constexpr uint32_t BM_RESERVED_3(0x014B);
constexpr uint32_t BM_RESERVED_4(0x014C);
constexpr uint32_t BM_RESERVED_5(0x014D);
constexpr uint32_t BM_RESERVED_6(0x014E);
constexpr uint32_t BM_RESERVED_7(0x014F);

// Sprite Registers
// Bit Field Definition for the Control Register
constexpr uint8_t SPRITE_Enable = 0x01;
constexpr uint8_t SPRITE_LUT0 = 0x02;
constexpr uint8_t SPRITE_LUT1 = 0x04;
constexpr uint8_t SPRITE_LUT2 = 0x08;
constexpr uint8_t SPRITE_DEPTH0 = 0x10;
constexpr uint8_t SPRITE_DEPTH1 = 0x20;
constexpr uint8_t SPRITE_DEPTH2 = 0x40;

constexpr uint32_t kSpriteRegistersBegin(0x0200);
constexpr uint32_t kSpriteRegistersEnd(0x02F8);
constexpr uint8_t kNumSpriteRegisters(0x08);

// DMA Controller 0xAF0400 - 0xAF04FF
constexpr uint32_t VDMA_CONTROL_REG(0x0400);
constexpr uint32_t VDMA_COUNT_REG_L(0x0401);
constexpr uint32_t VDMA_COUNT_REG_M(0x0402);
constexpr uint32_t VDMA_COUNT_REG_H(0x0403);
constexpr uint32_t VDMA_DATA_2_WRITE_L(0x0404);
constexpr uint32_t VDMA_DATA_2_WRITE_H(0x0405);
constexpr uint32_t VDMA_STRIDE_L(0x0406);
constexpr uint32_t VDMA_STRIDE_H(0x0407);
constexpr uint32_t VDMA_SRC_ADDY_L(0x0408);
constexpr uint32_t VDMA_SRC_ADDY_M(0x0409);
constexpr uint32_t VDMA_SRC_ADDY_H(0x040A);
constexpr uint32_t VDMA_RESERVED_0(0x040B);
constexpr uint32_t VDMA_DST_ADDY_L(0x040C);
constexpr uint32_t VDMA_DST_ADDY_M(0x040D);
constexpr uint32_t VDMA_DST_ADDY_H(0x040E);
constexpr uint32_t VDMA_RESERVED_1(0x040F);

constexpr uint32_t kMousePtrGrap0Begin(
    0x0500);  // 16 x 16 = 256 Pixels (Grey Scale) 0 =
              // Transparent, 1 = Black , 255 = White
constexpr uint32_t kMousePtrGrap0End(0x05FF);  //  Pointer 0

constexpr uint32_t kMousePtrGrap1Begin(0x0600);
constexpr uint32_t kMousePtrGrap1End(0x06FF);  //  Pointer 1

constexpr uint32_t kMousePtrCtrlReg(
    0x0700);  //  Bit[0] Enable, Bit[1] = 0  (0 = Pointer0, 1 = Pointer1)

constexpr uint32_t kMousePtrX(
    0x0702);  //  X Position (0 - 639) (Can only read
              //  now) Writing will have no effect
constexpr uint32_t kMousePtrY(
    0x0704);  //  Y Position (0 - 479) (Can only read
              //  now) Writing will have no effect

constexpr uint32_t MOUSE_PTR_Y_POS_H(0x0705);
constexpr uint32_t MOUSE_PTR_BYTE0(
    0x0706);  //  Byte 0 of Mouse Packet (you must write 3 Bytes)
constexpr uint32_t MOUSE_PTR_BYTE1(
    0x0707);  //  Byte 1 of Mouse Packet (if you don't, then )
constexpr uint32_t MOUSE_PTR_BYTE2(
    0x0708);  //  Byte 2 of Mouse Packet (state Machine
              //  will be jammed in 1 state)

constexpr uint32_t kTextFgColourLUT(0x1F40);
constexpr uint32_t kTextBgColourLUT(0x1F80);

constexpr uint32_t kGrphLutBegin(0x2000);     // start of all 8 LUTs
constexpr uint32_t kGrphLutSize(0x0400);      // the size of each LUT
constexpr uint32_t kGrphLutTotalSize(0x4000); // the total size of all 8 LUTs

constexpr uint32_t GAMMA_B_LUT_PTR(0x4000);
constexpr uint32_t GAMMA_G_LUT_PTR(0x4100);
constexpr uint32_t GAMMA_R_LUT_PTR(0x4200);

constexpr uint32_t kTileMapsBegin(0x5000);
constexpr uint32_t kTileMapsEnd(0x6ffff);
constexpr uint32_t kTileMapSize(0x800);

constexpr uint32_t kFontBankMemoryBegin(0x8000);   // start of font banks
constexpr uint32_t kFontBankMemorySize(0x800);     // size of each bank
constexpr uint32_t kFontTotalMemorySize(0x1000);   // size of both combined

constexpr uint32_t kTextMemoryBegin(0xA000);
constexpr uint32_t kTextMemorySize(0x2000);

constexpr uint32_t kTextColorMemoryBegin(0xc000);
constexpr uint32_t kTextColorMemorySize(0x2000);

constexpr uint32_t BTX_START(0xE000);
constexpr uint32_t BTX_END(0xFFFF);
