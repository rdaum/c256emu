#pragma once

#include <stdint.h>

// Internal VICKY Registers and Internal Memory Locations (LUTs)
constexpr uint32_t MASTER_CTRL_REG_L(0x000);
constexpr uint32_t MASTER_CTRL_REG_H(0x001);

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
constexpr uint32_t BORDER_CTRL_REG(0x0004);
constexpr uint8_t Border_Ctrl_Enable = 0x0;

constexpr uint32_t BORDER_COLOR_B(0x0005);
constexpr uint32_t BORDER_COLOR_G(0x0006);
constexpr uint32_t BORDER_COLOR_R(0x0007);

constexpr uint32_t BACKGROUND_COLOR_B(0x0008);
constexpr uint32_t BACKGROUND_COLOR_G(0x0009);
constexpr uint32_t BACKGROUND_COLOR_R(0x000A);

constexpr uint32_t VKY_TXT_CURSOR_CTRL_REG(0x0010);
constexpr uint8_t Vky_Cursor_Enable = 0x01;
constexpr uint8_t Vky_Cursor_Flash_Rate0 = 0x2;
constexpr uint8_t Vky_Cursor_Flash_Rate1 = 0x4;
constexpr uint8_t Vky_Cursor_FONT_Page0 = 0x08;
constexpr uint8_t Vky_Cursor_FONT_Page1 = 0x10;
constexpr uint32_t VKY_TXT_RESERVED(0x0011);
constexpr uint32_t VKY_TXT_CURSOR_CHAR_REG(0x0012);

constexpr uint32_t VKY_TXT_CURSOR_COLR_REG(0x0013);
constexpr uint32_t VKY_TXT_CURSOR_X_REG_L(0x0014);
constexpr uint32_t VKY_TXT_CURSOR_X_REG_H(0x0015);
constexpr uint32_t VKY_TXT_CURSOR_Y_REG_L(0x0016);
constexpr uint32_t VKY_TXT_CURSOR_Y_REG_H(0x0017);

constexpr uint32_t VKY_INFO_CHIP_NUM_L(0x001C);
constexpr uint32_t VKY_INFO_CHIP_NUM_H(0x001D);
constexpr uint32_t VKY_INFO_CHIP_VER_L(0x001E);
constexpr uint32_t VKY_INFO_CHIP_VER_H(0x001F);

//
// Bit Field Definition for the Control Register
constexpr uint8_t TILE_Enable = 0x01;
constexpr uint8_t TILE_LUT0 = 0x0;
constexpr uint8_t TILE_LUT1 = 0x0;
constexpr uint8_t TILE_LUT2 = 0x0;
constexpr uint8_t TILESHEET_256x256_En = 0x80;
//
// Tile MAP Layer 0 Registers
constexpr uint32_t TL0_CONTROL_REG(
    0x0100);  // Bit[0] - Enable, Bit[3:1] - LUT Select,
constexpr uint32_t TL0_START_ADDY_L(

    0x0101);  // Not USed right now - Starting uint32_t to where is the MAP
constexpr uint32_t TL0_START_ADDY_M(0x0102);
constexpr uint32_t TL0_START_ADDY_H(0x0103);
constexpr uint32_t TL0_MAP_X_STRIDE_L(0x0104);
constexpr uint32_t TL0_MAP_X_STRIDE_H(0x0105);
constexpr uint32_t TL0_MAP_Y_STRIDE_L(0x0106);
constexpr uint32_t TL0_MAP_Y_STRIDE_H(0x0107);
// TL0_RESERVED_0          = 0xAF0108
// TL0_RESERVED_1          = 0xAF0109
// TL0_RESERVED_2          = 0xAF010A
// TL0_RESERVED_3          = 0xAF010B
// TL0_RESERVED_4          = 0xAF010C
// TL0_RESERVED_5          = 0xAF010D
// TL0_RESERVED_6          = 0xAF010E
// TL0_RESERVED_7          = 0xAF010F
// Tile MAP Layer 1 Registers
constexpr uint32_t TL1_CONTROL_REG(
    0x0108);  // Bit[0] - Enable, Bit[3:1] - LUT Select,
constexpr uint32_t TL1_START_ADDY_L(

    0x0109);  // Not USed right now - Starting uint32_t to where is the MAP
constexpr uint32_t TL1_START_ADDY_M(0x010A);
constexpr uint32_t TL1_START_ADDY_H(0x010B);
constexpr uint32_t TL1_MAP_X_STRIDE_L(0x010C);
constexpr uint32_t TL1_MAP_X_STRIDE_H(0x010D);
constexpr uint32_t TL1_MAP_Y_STRIDE_L(0x010E);
constexpr uint32_t TL1_MAP_Y_STRIDE_H(0x010F);
// TL1_RESERVED_0          = 0xAF0118
// TL1_RESERVED_1          = 0xAF0119
// TL1_RESERVED_2          = 0xAF011A
// TL1_RESERVED_3          = 0xAF011B
// TL1_RESERVED_4          = 0xAF011C
// TL1_RESERVED_5          = 0xAF011D
// TL1_RESERVED_6          = 0xAF011E
// TL1_RESERVED_7          = 0xAF011F
// Tile MAP Layer 2 Registers
constexpr uint32_t TL2_CONTROL_REG(
    0x0110);  // Bit[0] - Enable, Bit[3:1] - LUT Select,
constexpr uint32_t TL2_START_ADDY_L(

    0x0111);  // Not USed right now - Starting uint32_t to where is the MAP
constexpr uint32_t TL2_START_ADDY_M(0x0112);
constexpr uint32_t TL2_START_ADDY_H(0x0113);
constexpr uint32_t TL2_MAP_X_STRIDE_L(0x0114);
constexpr uint32_t TL2_MAP_X_STRIDE_H(0x0115);
constexpr uint32_t TL2_MAP_Y_STRIDE_L(0x0116);
constexpr uint32_t TL2_MAP_Y_STRIDE_H(0x0117);
// TL2_RESERVED_0          = 0xAF0128
// TL2_RESERVED_1          = 0xAF0129
// TL2_RESERVED_2          = 0xAF012A
// TL2_RESERVED_3          = 0xAF012B
// TL2_RESERVED_4          = 0xAF012C
// TL2_RESERVED_5          = 0xAF012D
// TL2_RESERVED_6          = 0xAF012E
// TL2_RESERVED_7          = 0xAF012F
// Tile MAP Layer 3 Registers
constexpr uint32_t TL3_CONTROL_REG(
    0x0118);  // Bit[0] - Enable, Bit[3:1] - LUT Select,
constexpr uint32_t TL3_START_ADDY_L(

    0x0119);  // Not USed right now - Starting uint32_t to where is the MAP
constexpr uint32_t TL3_START_ADDY_M(0x011A);
constexpr uint32_t TL3_START_ADDY_H(0x011B);
constexpr uint32_t TL3_MAP_X_STRIDE_L(0x011C);
constexpr uint32_t TL3_MAP_X_STRIDE_H(0x011D);
constexpr uint32_t TL3_MAP_Y_STRIDE_L(0x011E);
constexpr uint32_t TL3_MAP_Y_STRIDE_H(0x011F);
// TL3_RESERVED_0          = 0xAF0138
// TL3_RESERVED_1          = 0xAF0139
// TL3_RESERVED_2          = 0xAF013A
// TL3_RESERVED_3          = 0xAF013B
// TL3_RESERVED_4          = 0xAF013C
// TL3_RESERVED_5          = 0xAF013D
// TL3_RESERVED_6          = 0xAF013E
// TL3_RESERVED_7          = 0xAF013F
// Bitmap Registers
constexpr uint32_t BM_CONTROL_REG(0x0140);
constexpr uint32_t BM_START_ADDY_L(0x0141);
constexpr uint32_t BM_START_ADDY_M(0x0142);
constexpr uint32_t BM_START_ADDY_H(0x0143);
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
constexpr uint8_t SPRITE_DEPTH1 = 0x2;
constexpr uint8_t SPRITE_DEPTH2 = 0x4;

constexpr uint32_t SP00_CONTROL_REG(0x0200);
constexpr uint32_t SP01_CONTROL_REG(0x0208);
constexpr uint32_t SP02_CONTROL_REG(0x0210);
constexpr uint32_t SP03_CONTROL_REG(0x0218);
constexpr uint32_t SP04_CONTROL_REG(0x0220);
constexpr uint32_t SP05_CONTROL_REG(0x0228);
constexpr uint32_t SP06_CONTROL_REG(0x0230);
constexpr uint32_t SP07_CONTROL_REG(0x0238);
constexpr uint32_t SP08_CONTROL_REG(0x0240);
constexpr uint32_t SP09_CONTROL_REG(0x0248);
constexpr uint32_t SP10_CONTROL_REG(0x0250);
constexpr uint32_t SP11_CONTROL_REG(0x0258);
constexpr uint32_t SP12_CONTROL_REG(0x0260);
constexpr uint32_t SP13_CONTROL_REG(0x0268);
constexpr uint32_t SP14_CONTROL_REG(0x0270);
constexpr uint32_t SP15_CONTROL_REG(0x0278);
constexpr uint32_t SP16_CONTROL_REG(0x0280);
constexpr uint32_t SP17_CONTROL_REG(0x0288);
// etc. until
constexpr uint32_t SP31_CONTROL_REG(0x02F8);

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

constexpr uint32_t MOUSE_PTR_GRAP0_START(
    0x0500);  // 16 x 16 = 256 Pixels (Grey Scale) 0 =
              // Transparent, 1 = Black , 255 = White
constexpr uint32_t MOUSE_PTR_GRAP0_END(0x05FF);  //  Pointer 0
constexpr uint32_t MOUSE_PTR_GRAP1_START(0x0600);
constexpr uint32_t MOUSE_PTR_GRAP1_END(0x06FF);  //  Pointer 1
constexpr uint32_t MOUSE_PTR_CTRL_REG_L(
    0x0700);  //  Bit[0] Enable, Bit[1] = 0  (0 = Pointer0, 1 = Pointer1)
constexpr uint32_t MOUSE_PTR_CTRL_REG_H(0x0701);
constexpr uint32_t MOUSE_PTR_X_POS_L(
    0x0702);  //  X Position (0 - 639) (Can only read
              //  now) Writing will have no effect
constexpr uint32_t MOUSE_PTR_X_POS_H(0x0703);
constexpr uint32_t MOUSE_PTR_Y_POS_L(
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

constexpr uint32_t FG_CHAR_LUT_PTR(0x1F40);
constexpr uint32_t BG_CHAR_LUT_PTR(0x1F80);

constexpr uint32_t GRPH_LUT0_PTR(0x2000);
constexpr uint32_t GRPH_LUT1_PTR(0x2400);
constexpr uint32_t GRPH_LUT2_PTR(0x2800);
constexpr uint32_t GRPH_LUT3_PTR(0x2C00);
constexpr uint32_t GRPH_LUT4_PTR(0x3000);
constexpr uint32_t GRPH_LUT5_PTR(0x3400);
constexpr uint32_t GRPH_LUT6_PTR(0x3800);
constexpr uint32_t GRPH_LUT7_PTR(0x3C00);

constexpr uint32_t GAMMA_B_LUT_PTR(0x4000);
constexpr uint32_t GAMMA_G_LUT_PTR(0x4100);
constexpr uint32_t GAMMA_R_LUT_PTR(0x4200);

constexpr uint32_t TILE_MAP0(0x5000);
constexpr uint32_t TILE_MAP1(0x5800);  // 0xAF5800 - 0xAF5FFF
constexpr uint32_t TILE_MAP2(0x6000);  // 0xAF6000 - 0xAF67FF
constexpr uint32_t TILE_MAP3(0x6800);  // 0xAF6800 - 0xAF6FFF

constexpr uint32_t FONT_MEMORY_BANK0(0x8000);  // 0xAF8000 - 0xAF87FF
constexpr uint32_t FONT_MEMORY_BANK1(0x8800);  // 0xAF8800 - 0xAF8FFF
constexpr uint32_t CS_TEXT_MEM_PTR(0xA000);
constexpr uint32_t CS_COLOR_MEM_PTR(0xC000);
constexpr uint32_t CS_COLOUR_MEM_END(0xDFFF);

constexpr uint32_t BTX_START(0xE000);
constexpr uint32_t BTX_END(0xFFFF);
