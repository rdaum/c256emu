#pragma once

#include "cpu/cpu_65816.h"

// Internal VICKY Registers and Internal Memory Locations (LUTs)
constexpr Address MASTER_CTRL_REG_L(0xaf, 0x000);
constexpr Address MASTER_CTRL_REG_H(0xaf, 0x002);

// Control Bits Fields
constexpr uint8_t Mstr_Ctrl_Text_Mode_En = 0x01; // Enable the Text Mode
constexpr uint8_t Mstr_Ctrl_Text_Overlay = 0x02;
constexpr uint8_t Mstr_Ctrl_Graph_Mode_En = 0x04;
constexpr uint8_t Mstr_Ctrl_Bitmap_En = 0x08;
constexpr uint8_t Mstr_Ctrl_TileMap_En = 0x10;
constexpr uint8_t Mstr_Ctrl_Sprite_En = 0x20;
constexpr uint8_t Mstr_Ctrl_GAMMA_En = 0x40;
constexpr uint8_t Mstr_Ctrl_Disable_Vid = 0x80;

// Reserved - TBD
constexpr Address VKY_RESERVED_00(0xaf, 0x0002);
constexpr Address VKY_RESERVED_01(0xaf, 0x0003);
constexpr Address BORDER_CTRL_REG(0xaf, 0x0004);
constexpr uint8_t Border_Ctrl_Enable = 0x0;

constexpr Address BORDER_COLOR_B(0xaf, 0x0005);
constexpr Address BORDER_COLOR_G(0xaf, 0x0006);
constexpr Address BORDER_COLOR_R(0xaf, 0x0007);

constexpr Address BACKGROUND_COLOR_B(0xaf, 0x0008);
constexpr Address BACKGROUND_COLOR_G(0xaf, 0x0009);
constexpr Address BACKGROUND_COLOR_R(0xaf, 0x000A);

constexpr Address VKY_TXT_CURSOR_CTRL_REG(0xaf, 0x0010);
constexpr uint8_t Vky_Cursor_Enable = 0x01;
constexpr uint8_t Vky_Cursor_Flash_Rate0 = 0x2;
constexpr uint8_t Vky_Cursor_Flash_Rate1 = 0x4;
constexpr uint8_t Vky_Cursor_FONT_Page0 = 0x08;
constexpr uint8_t Vky_Cursor_FONT_Page1 = 0x10;
constexpr Address VKY_TXT_RESERVED(0xaf, 0x0011);
constexpr Address VKY_TXT_CURSOR_CHAR_REG(0xaf, 0x0012);

constexpr Address VKY_TXT_CURSOR_COLR_REG(0xaf, 0x0013);
constexpr Address VKY_TXT_CURSOR_X_REG_L(0xaf, 0x0014);
constexpr Address VKY_TXT_CURSOR_X_REG_H(0xaf, 0x0015);
constexpr Address VKY_TXT_CURSOR_Y_REG_L(0xaf, 0x0016);
constexpr Address VKY_TXT_CURSOR_Y_REG_H(0xaf, 0x0017);

constexpr Address VKY_INFO_CHIP_NUM_L(0xaf, 0x001C);
constexpr Address VKY_INFO_CHIP_NUM_H(0xaf, 0x001D);
constexpr Address VKY_INFO_CHIP_VER_L(0xaf, 0x001E);
constexpr Address VKY_INFO_CHIP_VER_H(0xaf, 0x001F);

//
// Bit Field Definition for the Control Register
constexpr uint8_t TILE_Enable = 0x01;
constexpr uint8_t TILE_LUT0 = 0x0;
constexpr uint8_t TILE_LUT1 = 0x0;
constexpr uint8_t TILE_LUT2 = 0x0;
constexpr uint8_t TILESHEET_256x256_En = 0x80;
//
// Tile MAP Layer 0 Registers
constexpr Address
    TL0_CONTROL_REG(0xaf,
                    0x0100); // Bit[0] - Enable, Bit[3:1] - LUT Select,
constexpr Address TL0_START_ADDY_L(
    0xaf,
    0x0101); // Not USed right now - Starting Address to where is the MAP
constexpr Address TL0_START_ADDY_M(0xaf, 0x0102);
constexpr Address TL0_START_ADDY_H(0xaf, 0x0103);
constexpr Address TL0_MAP_X_STRIDE_L(0xaf, 0x0104);
constexpr Address TL0_MAP_X_STRIDE_H(0xaf, 0x0105);
constexpr Address TL0_MAP_Y_STRIDE_L(0xaf, 0x0106);
constexpr Address TL0_MAP_Y_STRIDE_H(0xaf, 0x0107);
// TL0_RESERVED_0          = 0xAF0108
// TL0_RESERVED_1          = 0xAF0109
// TL0_RESERVED_2          = 0xAF010A
// TL0_RESERVED_3          = 0xAF010B
// TL0_RESERVED_4          = 0xAF010C
// TL0_RESERVED_5          = 0xAF010D
// TL0_RESERVED_6          = 0xAF010E
// TL0_RESERVED_7          = 0xAF010F
// Tile MAP Layer 1 Registers
constexpr Address
    TL1_CONTROL_REG(0xaf,
                    0x0108); // Bit[0] - Enable, Bit[3:1] - LUT Select,
constexpr Address TL1_START_ADDY_L(
    0xaf,
    0x0109); // Not USed right now - Starting Address to where is the MAP
constexpr Address TL1_START_ADDY_M(0xaf, 0x010A);
constexpr Address TL1_START_ADDY_H(0xaf, 0x010B);
constexpr Address TL1_MAP_X_STRIDE_L(0xaf, 0x010C);
constexpr Address TL1_MAP_X_STRIDE_H(0xaf, 0x010D);
constexpr Address TL1_MAP_Y_STRIDE_L(0xaf, 0x010E);
constexpr Address TL1_MAP_Y_STRIDE_H(0xaf, 0x010F);
// TL1_RESERVED_0          = 0xAF0118
// TL1_RESERVED_1          = 0xAF0119
// TL1_RESERVED_2          = 0xAF011A
// TL1_RESERVED_3          = 0xAF011B
// TL1_RESERVED_4          = 0xAF011C
// TL1_RESERVED_5          = 0xAF011D
// TL1_RESERVED_6          = 0xAF011E
// TL1_RESERVED_7          = 0xAF011F
// Tile MAP Layer 2 Registers
constexpr Address
    TL2_CONTROL_REG(0xaf,
                    0x0110); // Bit[0] - Enable, Bit[3:1] - LUT Select,
constexpr Address TL2_START_ADDY_L(
    0xaf,
    0x0111); // Not USed right now - Starting Address to where is the MAP
constexpr Address TL2_START_ADDY_M(0xaf, 0x0112);
constexpr Address TL2_START_ADDY_H(0xaf, 0x0113);
constexpr Address TL2_MAP_X_STRIDE_L(0xaf, 0x0114);
constexpr Address TL2_MAP_X_STRIDE_H(0xaf, 0x0115);
constexpr Address TL2_MAP_Y_STRIDE_L(0xaf, 0x0116);
constexpr Address TL2_MAP_Y_STRIDE_H(0xaf, 0x0117);
// TL2_RESERVED_0          = 0xAF0128
// TL2_RESERVED_1          = 0xAF0129
// TL2_RESERVED_2          = 0xAF012A
// TL2_RESERVED_3          = 0xAF012B
// TL2_RESERVED_4          = 0xAF012C
// TL2_RESERVED_5          = 0xAF012D
// TL2_RESERVED_6          = 0xAF012E
// TL2_RESERVED_7          = 0xAF012F
// Tile MAP Layer 3 Registers
constexpr Address
    TL3_CONTROL_REG(0xAF,
                    0x0118); // Bit[0] - Enable, Bit[3:1] - LUT Select,
constexpr Address TL3_START_ADDY_L(
    0xaf,
    0x0119); // Not USed right now - Starting Address to where is the MAP
constexpr Address TL3_START_ADDY_M(0xaf, 0x011A);
constexpr Address TL3_START_ADDY_H(0xaf, 0x011B);
constexpr Address TL3_MAP_X_STRIDE_L(0xaf, 0x011C);
constexpr Address TL3_MAP_X_STRIDE_H(0xaf, 0x011D);
constexpr Address TL3_MAP_Y_STRIDE_L(0xaf, 0x011E);
constexpr Address TL3_MAP_Y_STRIDE_H(0xaf, 0x011F);
// TL3_RESERVED_0          = 0xAF0138
// TL3_RESERVED_1          = 0xAF0139
// TL3_RESERVED_2          = 0xAF013A
// TL3_RESERVED_3          = 0xAF013B
// TL3_RESERVED_4          = 0xAF013C
// TL3_RESERVED_5          = 0xAF013D
// TL3_RESERVED_6          = 0xAF013E
// TL3_RESERVED_7          = 0xAF013F
// Bitmap Registers
constexpr Address BM_CONTROL_REG(0xaf, 0x0140);
constexpr Address BM_START_ADDY_L(0xaf, 0x0141);
constexpr Address BM_START_ADDY_M(0xaf, 0x0142);
constexpr Address BM_START_ADDY_H(0xaf, 0x0143);
constexpr Address BM_X_SIZE_L(0xaf, 0x0144);
constexpr Address BM_X_SIZE_H(0xaf, 0x0145);
constexpr Address BM_Y_SIZE_L(0xaf, 0x0146);
constexpr Address BM_Y_SIZE_H(0xaf, 0x0147);
constexpr Address BM_RESERVED_0(0xaf, 0x0148);
constexpr Address BM_RESERVED_1(0xaf, 0x0149);
constexpr Address BM_RESERVED_2(0xaf, 0x014A);
constexpr Address BM_RESERVED_3(0xaf, 0x014B);
constexpr Address BM_RESERVED_4(0xaf, 0x014C);
constexpr Address BM_RESERVED_5(0xaf, 0x014D);
constexpr Address BM_RESERVED_6(0xaf, 0x014E);
constexpr Address BM_RESERVED_7(0xaf, 0x014F);
// Sprite Registers
// Bit Field Definition for the Control Register
constexpr uint8_t SPRITE_Enable = 0x01;
constexpr uint8_t SPRITE_LUT0 = 0x02;
constexpr uint8_t SPRITE_LUT1 = 0x04;
constexpr uint8_t SPRITE_LUT2 = 0x08;
constexpr uint8_t SPRITE_DEPTH0 = 0x10;
constexpr uint8_t SPRITE_DEPTH1 = 0x2;
constexpr uint8_t SPRITE_DEPTH2 = 0x4;

constexpr Address SP00_CONTROL_REG(0xaf, 0x0200);
constexpr Address SP01_CONTROL_REG(0xaf, 0x0208);
constexpr Address SP02_CONTROL_REG(0xaf, 0x0210);
constexpr Address SP03_CONTROL_REG(0xaf, 0x0218);
constexpr Address SP04_CONTROL_REG(0xaf, 0x0220);
constexpr Address SP05_CONTROL_REG(0xaf, 0x0228);
constexpr Address SP06_CONTROL_REG(0xaf, 0x0230);
constexpr Address SP07_CONTROL_REG(0xaf, 0x0238);
constexpr Address SP08_CONTROL_REG(0xaf, 0x0240);
constexpr Address SP09_CONTROL_REG(0xaf, 0x0248);
constexpr Address SP10_CONTROL_REG(0xaf, 0x0250);
constexpr Address SP11_CONTROL_REG(0xaf, 0x0258);
constexpr Address SP12_CONTROL_REG(0xaf, 0x0260);
constexpr Address SP13_CONTROL_REG(0xaf, 0x0268);
constexpr Address SP14_CONTROL_REG(0xaf, 0x0270);
constexpr Address SP15_CONTROL_REG(0xaf, 0x0278);
constexpr Address SP16_CONTROL_REG(0xaf, 0x0280);
constexpr Address SP17_CONTROL_REG(0xaf, 0x0288);
// etc. until
constexpr Address SP31_CONTROL_REG(0xaf, 0x02F8);

// DMA Controller 0xAF0400 - 0xAF04FF
constexpr Address VDMA_CONTROL_REG(0xaf, 0x0400);
constexpr Address VDMA_COUNT_REG_L(0xaf, 0x0401);
constexpr Address VDMA_COUNT_REG_M(0xaf, 0x0402);
constexpr Address VDMA_COUNT_REG_H(0xaf, 0x0403);
constexpr Address VDMA_DATA_2_WRITE_L(0xaf, 0x0404);
constexpr Address VDMA_DATA_2_WRITE_H(0xaf, 0x0405);
constexpr Address VDMA_STRIDE_L(0xaf, 0x0406);
constexpr Address VDMA_STRIDE_H(0xaf, 0x0407);
constexpr Address VDMA_SRC_ADDY_L(0xaf, 0x0408);
constexpr Address VDMA_SRC_ADDY_M(0xaf, 0x0409);
constexpr Address VDMA_SRC_ADDY_H(0xaf, 0x040A);
constexpr Address VDMA_RESERVED_0(0xaf, 0x040B);
constexpr Address VDMA_DST_ADDY_L(0xaf, 0x040C);
constexpr Address VDMA_DST_ADDY_M(0xaf, 0x040D);
constexpr Address VDMA_DST_ADDY_H(0xaf, 0x040E);
constexpr Address VDMA_RESERVED_1(0xaf, 0x040F);

constexpr Address
    MOUSE_PTR_GRAP0_START(0xAF,
                          0x0500); // 16 x 16 = 256 Pixels (Grey Scale) 0 =
                                   // Transparent, 1 = Black , 255 = White
constexpr Address MOUSE_PTR_GRAP0_END(0xaf, 0x05FF); //  Pointer 0
constexpr Address MOUSE_PTR_GRAP1_START(0xAF, 0x0600);
constexpr Address MOUSE_PTR_GRAP1_END(0xaf, 0x06FF); //  Pointer 1
constexpr Address MOUSE_PTR_CTRL_REG_L(
    0xaf, 0x0700); //  Bit[0] Enable, Bit[1] = 0  ( 0 = Pointer0, 1 = Pointer1)
constexpr Address MOUSE_PTR_CTRL_REG_H(0xaf, 0x0701);
constexpr Address
    MOUSE_PTR_X_POS_L(0xaf, 0x0702); //  X Position (0 - 639) (Can only read
                                     //  now) Writing will have no effect
constexpr Address MOUSE_PTR_X_POS_H(0xaf, 0x0703);
constexpr Address
    MOUSE_PTR_Y_POS_L(0xaf, 0x0704); //  Y Position (0 - 479) (Can only read
                                     //  now) Writing will have no effect
constexpr Address MOUSE_PTR_Y_POS_H(0xaf, 0x0705);
constexpr Address
    MOUSE_PTR_BYTE0(0xaf,
                    0x0706); //  Byte 0 of Mouse Packet (you must write 3 Bytes)
constexpr Address
    MOUSE_PTR_BYTE1(0xaf,
                    0x0707); //  Byte 1 of Mouse Packet (if you don't, then )
constexpr Address
    MOUSE_PTR_BYTE2(0xaf, 0x0708); //  Byte 2 of Mouse Packet (state Machine
                                   //  will be jammed in 1 state)

constexpr Address FG_CHAR_LUT_PTR(0xaf, 0x1F40);
constexpr Address BG_CHAR_LUT_PTR(0xaf, 0x1F80);

constexpr Address GRPH_LUT0_PTR(0xaf, 0x2000);
constexpr Address GRPH_LUT1_PTR(0xaf, 0x2400);
constexpr Address GRPH_LUT2_PTR(0xaf, 0x2800);
constexpr Address GRPH_LUT3_PTR(0xaf, 0x2C00);
constexpr Address GRPH_LUT4_PTR(0xaf, 0x3000);
constexpr Address GRPH_LUT5_PTR(0xaf, 0x3400);
constexpr Address GRPH_LUT6_PTR(0xaf, 0x3800);
constexpr Address GRPH_LUT7_PTR(0xaf, 0x3C00);

constexpr Address GAMMA_B_LUT_PTR(0xaf, 0x4000);
constexpr Address GAMMA_G_LUT_PTR(0xaf, 0x4100);
constexpr Address GAMMA_R_LUT_PTR(0xaf, 0x4200);

constexpr Address TILE_MAP0(0xaf, 0x5000);
constexpr Address TILE_MAP1(0xaf, 0x5800); // 0xAF5800 - 0xAF5FFF
constexpr Address TILE_MAP2(0xaf, 0x6000); // 0xAF6000 - 0xAF67FF
constexpr Address TILE_MAP3(0xaf, 0x6800); // 0xAF6800 - 0xAF6FFF

constexpr Address FONT_MEMORY_BANK0(0xaf, 0x8000); // 0xAF8000 - 0xAF87FF
constexpr Address FONT_MEMORY_BANK1(0xaf, 0x8800); // 0xAF8800 - 0xAF8FFF
constexpr Address CS_TEXT_MEM_PTR(0xaf, 0xA000);
constexpr Address CS_COLOR_MEM_PTR(0xaf, 0xC000);
constexpr Address CS_COLOUR_MEM_END(0xaf, 0xDFFF);

constexpr Address BTX_START(0xaf, 0xE000);
constexpr Address BTX_END(0xaf, 0xFFFF);
