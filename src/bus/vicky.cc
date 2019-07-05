#include "bus/vicky.h"

#include <chrono>
#include <functional>
#include <thread>

#include "bus/int_controller.h"
#include "bus/vicky_def.h"
#include "system.h"
#include "vicky.h"

#include <GL/glext.h>

namespace {

constexpr uint8_t kColsPerLine = 128;
constexpr uint8_t kTileSize = 16;
constexpr uint8_t kSpriteSize = 32;
constexpr uint16_t kTileSetStride = 256;

bool Set16(uint32_t addr, uint32_t start_addr, uint16_t *dest, uint8_t v) {
  uint16_t o = addr - start_addr;
  if (o > 1)
    return false;
  if (o == 0)
    *dest = (*dest & 0xff00) | v;
  else
    *dest = (*dest & 0x00ff) | (v << 8);
  return true;
}

bool Set24(uint32_t addr, uint32_t start_addr, uint32_t *dest, uint8_t v) {
  uint16_t o = addr - start_addr;
  if (o > 2)
    return false;
  if (o == 0)
    *dest = (*dest & 0x00ffff00) | v;
  else if (o == 1)
    *dest = (*dest & 0x00ff00ff) | (v << 8);
  else if (o == 2)
    *dest = (*dest & 0x0000ffff) | (v << 16);
  return true;
}

void Set32(uint32_t *value, uint8_t bit_number) {
  auto mask = static_cast<uint32_t>(1 << bit_number);
  *value = *value | mask;
}

void SetLower8(uint16_t *destination, uint8_t value) {
  *destination &= 0xFF00;
  *destination |= value;
}

void SetHigher8(uint16_t *destination, uint8_t value) {
  *destination &= 0x00ff;
  *destination |= (value << 8);
}

std::string ScalingQualityStr(Vicky::ScalingQuality scaling_quality) {
  switch (scaling_quality) {
  case Vicky::ScalingQuality::NEAREST:
    return "0";
  case Vicky::ScalingQuality::LINEAR:
    return "1";
  default:
  case Vicky::ScalingQuality::BEST:
    return "2";
  }
}
} // namespace

Vicky::Vicky(System *system, InterruptController *int_controller)
    : sys_(system), int_controller_(int_controller) {
  memset(fg_colour_mem_, 0, sizeof(fg_colour_mem_));
  memset(bg_colour_mem_, 0, sizeof(bg_colour_mem_));
  memset(video_ram_, 0, sizeof(video_ram_));
  memset(tile_sets_, 0, sizeof(tile_sets_));
  memset(sprites_, 0, sizeof(sprites_));
  memset(tile_mem_, 0, sizeof(tile_mem_));
  registers_ = {
      {kBorderColour, &border_colour_.v, 3},
      {kCursorX, &cursor_x_, 2},
      {kCursorY, &cursor_y_, 2},
      {kCursorCtrlReg, &cursor_reg_, 1},
      {kCursorColour, &cursor_colour_, 1},
      {kCursorChar, &cursor_char_, 1},
      {kMousePtrX, &mouse_pos_x_, 2},
      {kMousePtrY, &mouse_pos_y_, 2},
      {kBitmapStartAddress, &bitmap_addr_offset_, 3},
      {kMasterCtrlReg, &mode_, 2},
      {kBackgroundColour, &background_bgr_.v, 3},
  };
}

void Vicky::InitPages(Page *vicky_page_start) {
  auto map = [vicky_page_start](uint32_t addr, uint8_t *ptr, uint32_t size) {
    auto *start = vicky_page_start + (addr >> 12);
    CHECK((size & 0xFFF) == 0);
    for (uint32_t i = 0; i<size>> 12; i++) {
      start[i].ptr = ptr + (i * (1 << 12));
      start[i].io_eq = 1;
      start[i].io_mask = 0;
    }
  };
  map(kTextMemoryBegin, text_mem_, kTextMemorySize);
  map(kTextColorMemoryBegin, text_colour_mem_, kTextColorMemorySize);
  map(kFontBankMemoryBegin, font_bank_, kFontTotalMemorySize);
  map(kGrphLutBegin, (uint8_t *)lut_, kGrphLutTotalSize);
  map(kTileMapsBegin, (uint8_t *)tile_mem_, sizeof(tile_mem_));
}

GLFWwindow *Vicky::Start() {
  window_ =
      glfwCreateWindow(kVickyBitmapWidth * scale_, kVickyBitmapHeight * scale_,
                       "Vicky", nullptr, nullptr);

  CHECK(window_);
  glfwMakeContextCurrent(window_);

  return window_;
}

Vicky::~Vicky() {
  if (window_) {
    glfwDestroyWindow(window_);
  }
}

uint8_t Vicky::ReadByte(uint32_t addr) {
  uint8_t v;
  if (ReadRegister(addr, registers_, &v)) {
    return v;
  }

  if (addr == kMousePtrCtrlReg) {
    if (mouse_cursor_enable_)
      v |= 0x01;
    if (mouse_cursor_select_)
      v |= 0x02;
    return v;
  }
  if (addr == kBorderCtrlReg) {
    if (border_enabled_)
      v |= Border_Ctrl_Enable;
    return v;
  }
  if (addr == kBitmapCtrlReg) {
    if (bitmap_enabled_)
      v |= 0x01;
    v |= (bitmap_lut_ << 4);
    return v;
  }

  if (addr >= kTextFgColourLUT && addr < kTextBgColourLUT) {
    memcpy(&v, (uint8_t *)fg_colour_mem_ + addr - kTextFgColourLUT, 1);
    return v;
  } else if (addr >= kTextBgColourLUT && addr < 0x1fc0) {
    memcpy(&v, (uint8_t *)bg_colour_mem_ + addr - kTextBgColourLUT, 1);
    return v;
  } else if (addr >= GAMMA_B_LUT_PTR && addr < GAMMA_G_LUT_PTR) {
    return gamma_.b[addr & 0xff];
  } else if (addr >= GAMMA_G_LUT_PTR && addr < GAMMA_R_LUT_PTR) {
    return gamma_.g[addr & 0xff];
  } else if (addr >= GAMMA_R_LUT_PTR && addr < 0x4300) {
    return gamma_.r[addr & 0xff];
  } else if (addr >= kMousePtrGrap0Begin && addr <= kMousePtrGrap0End) {
    return mouse_cursor_0_[addr - kMousePtrGrap0Begin];
  } else if (addr >= kMousePtrGrap1Begin && addr <= kMousePtrGrap1End) {
    return mouse_cursor_1_[addr - kMousePtrGrap1Begin];
  }

  if (addr >= kTileRegistersBegin && addr <= kTileRegistersEnd) {
    uint8_t v = 0;
    uint16_t tile_offset = addr - kTileRegistersBegin;
    uint8_t tile_num = tile_offset / kNumTileRegisters;
    uint8_t register_num = tile_offset % kNumTileRegisters;
    TileSet &tile_set = tile_sets_[tile_num];
    if (register_num == 0) {
      if (tile_set.enabled)
        v |= TILE_Enable;
      v |= tile_set.lut << 1;
      if (tile_set.tiled_sheet)
        v |= TILESHEET_256x256_En;
      if (tile_set.scroll_x_enable)
        v |= TILE_Scroll_X_Enable;
      if (tile_set.scroll_y_enable)
        v |= TILE_Scroll_Y_Enable;
    } else if (register_num == 1) {
      v = tile_set.start_addr & 0x000000ff;
    } else if (register_num == 2) {
      v = (tile_set.start_addr & 0x0000ff00) >> 8;
    } else if (register_num == 3) {
      v = (tile_set.start_addr & 0x00ff0000) >> 16;
    } else if (register_num == 4) {
      v = (tile_set.offset_x);
    } else if (register_num == 5) {
      v = (tile_set.offset_y);
    } else {
      LOG(ERROR) << "Unsupported tile reg read: " << std::hex
                 << (int)register_num << " @ " << std::hex << addr;
    }
    return v;
  }

  if (addr >= kSpriteRegistersBegin && addr <= kSpriteRegistersEnd + 8) {
    uint8_t v = 0;
    uint16_t sprite_offset = addr - kSpriteRegistersBegin;
    uint16_t sprite_num = sprite_offset / kNumSpriteRegisters;
    uint16_t register_num = sprite_offset % kNumSpriteRegisters;
    Sprite &sprite = sprites_[sprite_num];
    if (register_num == 0) /* control register */ {
      v |= (sprite.layer << 4);
      if (sprite.enabled)
        v |= SPRITE_Enable;
      v |= (sprite.lut << 1);
      if (sprite.tile_striding)
        v |= SPRITE_Striding;
    } else if (register_num == 1) {
      v = sprite.start_addr & 0x000000ff;
    } else if (register_num == 2) {
      v = sprite.start_addr & 0x0000ff000 >> 8;
    } else if (register_num == 3) {
      v = sprite.start_addr & 0x00ff0000 >> 16;
    } else if (register_num == 4) {
      v = sprite.x & 0x00ff;
    } else if (register_num == 5) {
      v = sprite.x & 0xff00 >> 8;
    } else if (register_num == 6) {
      v = sprite.x & 0x00ff;
    } else if (register_num == 7) {
      v = sprite.y & 0xff00 >> 8;
    } else {
      LOG(ERROR) << "Unsupported sprite reg read: " << register_num;
    }
    return v;
  }

  LOG(INFO) << "Read from unhandled vicky reg: " << std::hex << addr;
  return 0;
}

void Vicky::StoreByte(uint32_t addr, uint8_t v) {
  if (StoreRegister(addr, v, registers_))
    return;

  if (addr >= kTextFgColourLUT && addr < kTextBgColourLUT) {
    memcpy((uint8_t *)fg_colour_mem_ + addr - kTextFgColourLUT, &v, 1);
    return;
  } else if (addr >= kTextBgColourLUT && addr < 0x1fc0) {
    memcpy((uint8_t *)bg_colour_mem_ + addr - kTextBgColourLUT, &v, 1);
    return;
  } else if (addr >= GAMMA_B_LUT_PTR && addr < GAMMA_G_LUT_PTR) {
    gamma_.b[addr & 0xFF] = v;
    return;
  } else if (addr >= GAMMA_G_LUT_PTR && addr < GAMMA_R_LUT_PTR) {
    gamma_.g[addr & 0xFF] = v;
    return;
  } else if (addr >= GAMMA_R_LUT_PTR && addr < 0x4300) {
    gamma_.r[addr & 0xFF] = v;
    return;
  } else if (addr >= kMousePtrGrap0Begin && addr <= kMousePtrGrap0End) {
    mouse_cursor_0_[addr - kMousePtrGrap0Begin] = v;
  } else if (addr >= kMousePtrGrap1Begin && addr <= kMousePtrGrap1End) {
    mouse_cursor_1_[addr - kMousePtrGrap1Begin] = v;
  }

  if (addr == kBitmapCtrlReg) {
    bitmap_enabled_ = v & 0x01;
    bitmap_lut_ = (v & 0b01110000) >> 4;
    return;
  }

  if (addr >= kTileRegistersBegin && addr <= kTileRegistersEnd) {
    uint16_t tile_offset = addr - kTileRegistersBegin;
    uint8_t tile_num = tile_offset / kNumTileRegisters;
    uint8_t register_num = tile_offset % kNumTileRegisters;
    TileSet &tile_set = tile_sets_[tile_num];
    if (register_num == 0) {
      tile_set.enabled = v & TILE_Enable;
      tile_set.lut = (v & 0b00001110) >> 1;
      tile_set.tiled_sheet = (v & TILESHEET_256x256_En);
      tile_set.scroll_x_enable = (v & TILE_Scroll_X_Enable);
      tile_set.scroll_y_enable = (v & TILE_Scroll_Y_Enable);
    } else if (register_num == 1) {
      tile_set.start_addr = (tile_set.start_addr & 0x00ffff00) | v;
    } else if (register_num == 2) {
      tile_set.start_addr = (tile_set.start_addr & 0x00ff00ff) | (v << 8);
    } else if (register_num == 3) {
      tile_set.start_addr = (tile_set.start_addr & 0x0000ffff) | (v << 16);
    } else if (register_num == 4) {
      tile_set.offset_x = v;
    } else if (register_num == 5) {
      tile_set.offset_y = v;
    } else {
      LOG(ERROR) << "Unsupported tile reg: " << std::hex << (int)register_num
                 << " @ " << std::hex << addr;
    }
    return;
  }

  if (addr >= kSpriteRegistersBegin && addr <= kSpriteRegistersEnd + 8) {
    uint16_t sprite_offset = addr - kSpriteRegistersBegin;
    uint16_t sprite_num = sprite_offset / kNumSpriteRegisters;
    uint16_t register_num = sprite_offset % kNumSpriteRegisters;
    Sprite &sprite = sprites_[sprite_num];
    if (register_num == 0) /* control register */ {
      uint8_t layer = (v & 0b01110000) >> 4;
      sprite.layer = layer;
      sprite.enabled = v & SPRITE_Enable;
      sprite.lut = (v & 0b00001110) >> 1;
      sprite.tile_striding = (v & 0x80);
    } else if (register_num == 1) {
      sprite.start_addr = (sprite.start_addr & 0x00ffff00) | v;
    } else if (register_num == 2) {
      sprite.start_addr = (sprite.start_addr & 0x00ff00ff) | (v << 8);
    } else if (register_num == 3) {
      sprite.start_addr = (sprite.start_addr & 0x0000ffff) | (v << 16);
    } else if (register_num == 4) {
      SetLower8(&sprite.x, v);
    } else if (register_num == 5) {
      SetHigher8(&sprite.x, v);
    } else if (register_num == 6) {
      SetLower8(&sprite.y, v);
    } else if (register_num == 7) {
      SetHigher8(&sprite.y, v);
    } else {
      LOG(ERROR) << "Unsupported sprite reg: " << register_num;
    }
    return;
  }

  if (addr == kMousePtrCtrlReg) {
    mouse_cursor_enable_ = v & 0x01;
    mouse_cursor_select_ = v & 0x02;
    return;
  }
  if (addr == kBorderCtrlReg) {
    border_enabled_ = v & Border_Ctrl_Enable;
    return;
  }
  LOG(INFO) << "Unknown Vicky register: " << std::hex << addr;
}

void Vicky::RenderLine() {
  glfwPollEvents();

  if (vblank_cnt_ < kVickyVBlankLines) {
    vblank_cnt_++;
    return;
  }

  //  if (mode_ & Mstr_Ctrl_Disable_Vid)
  //    return;

  bool run_char_gen =
      mode_ & Mstr_Ctrl_Text_Mode_En || mode_ & Mstr_Ctrl_Text_Overlay;

  // Check cursor flash.
  if (run_char_gen && (cursor_reg_ & Vky_Cursor_Enable)) {
    uint16_t flash_interval_ms = 0;
    switch (cursor_reg_ >> 1) {
    case 0b00:
      flash_interval_ms = 1000;
      break;
    case 0b01:
      flash_interval_ms = 500;
      break;
    case 0b10:
      flash_interval_ms = 250;
      break;
    case 0b11:
      flash_interval_ms = 200;
      break;
    }
    auto time_since_flash =
        std::chrono::steady_clock::now() - last_cursor_flash_;
    if (time_since_flash > std::chrono::milliseconds(flash_interval_ms)) {
      cursor_state_ = !cursor_state_;
      last_cursor_flash_ = std::chrono::steady_clock::now();
    }
  }

  uint32_t *row_pixels = &frame_buffer_[kVickyBitmapWidth * raster_y_];

  // Calculate sprites valid for this row before scanning.
  uint32_t sprite_masks[4]{0, 0, 0, 0};
  if (mode_ & Mstr_Ctrl_Sprite_En) {
    for (uint8_t sprite_num = 0; sprite_num < 32; sprite_num++) {
      Sprite &sprite = sprites_[sprite_num];
      if (!sprite.enabled || sprite.y < raster_y_ ||
          sprite.y > raster_y_ + kSpriteSize || sprite.x > 479)
        continue;
      Set32(&sprite_masks[sprite.layer], sprite_num);
    }
  }

  for (uint16_t raster_x = 0; raster_x < kVickyBitmapWidth; raster_x++) {
    CHECK(raster_x < 640);
    uint32_t *row_pixel = &row_pixels[raster_x];

    // Background colour
    row_pixels[raster_x] = ColourCorrect(background_bgr_.v);

    // Bitmap
    if (mode_ & Mstr_Ctrl_Bitmap_En && bitmap_enabled_)
      RenderBitmap(raster_x, row_pixel);

    // Layers back to front, sprites first, tiles next
    for (uint8_t layer = kNumLayers; layer-- > 0;) {
      // Sprites
      if (sprite_masks[layer]) {
        RenderSprites(raster_x, layer, sprite_masks[layer], row_pixel);
      }

      if (mode_ & Mstr_Ctrl_TileMap_En) {
        RenderTileMap(raster_x, layer, row_pixel);
      }
    }

    // Characters
    if (run_char_gen) {
      RenderCharacterGenerator(raster_x, row_pixel);
    }

    // Mouse
    if (mouse_cursor_enable_)
      RenderMouseCursor(raster_x, row_pixel);

    if (border_enabled_) {
      uint32_t border_colour = ColourCorrect(border_colour_.v);
      if (raster_x < kBorderWidth) {
        *row_pixel = border_colour;
        continue;
      }
      if (raster_x > kVickyBitmapWidth - kBorderWidth) {
        *row_pixel = border_colour;

        continue;
      }
      if (raster_y_ < kBorderHeight) {
        *row_pixel = border_colour;

        continue;
      }
      if (raster_y_ > kVickyBitmapHeight - kBorderHeight) {
        *row_pixel = border_colour;

        continue;
      }
    }
  }

  // TODO line interrupt
  raster_y_++;
  if (raster_y_ == kVickyBitmapHeight) {
    int display_w, display_h;
    glfwMakeContextCurrent(window_);
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kVickyBitmapWidth,
                 kVickyBitmapHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE,
                 frame_buffer_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex3f(-1, -1, 0);
    glTexCoord2f(1, 1);
    glVertex3f(1, -1, 0);
    glTexCoord2f(1, 0);
    glVertex3f(1, 1, 0);
    glTexCoord2f(0, 0);
    glVertex3f(-1, 1, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glfwSwapBuffers(window_);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &texture_id);

    vblank_cnt_ = 0;
    raster_y_ = 0;
  }
}

bool Vicky::RenderBitmap(uint16_t raster_x, uint32_t *row_pixel) {
  uint8_t *indexed_row =
      video_ram_ + bitmap_addr_offset_ + (raster_y_ * kVickyBitmapWidth);
  uint8_t colour_index = indexed_row[raster_x];
  if (colour_index == 0)
    return false;
  *row_pixel = ColourCorrect(lut_[bitmap_lut_][colour_index].v);
  return true;
}

bool Vicky::RenderSprites(uint16_t raster_x, uint8_t layer,
                          uint32_t sprite_mask, uint32_t *row_pixel) {
  // TODO this sprite routine can't keep up to 14mhz emulation on my
  // PC if lots of sprites were enabled in many layers.

  // Keep drawing until we run out of sprites in this layer.
  for (int sprite_num = 0; sprite_mask; sprite_num++, sprite_mask >>= 1) {
    Sprite &sprite = sprites_[sprite_num];

    if (!sprite.enabled || raster_x < sprite.x ||
        raster_x > sprite.x + kSpriteSize || raster_y_ < sprite.y ||
        raster_y_ > sprite.y + kSpriteSize)
      continue;

    const uint8_t *sprite_mem = video_ram_ + sprite.start_addr;
    uint8_t colour_index =
        sprite_mem[raster_x - sprite.x + (raster_y_ - sprite.y) * kSpriteSize];
    if (colour_index == 0)
      return false;
    *row_pixel = ColourCorrect(lut_[sprite.lut][colour_index].v);
    return true;
  }
  return false;
}

bool Vicky::RenderTileMap(uint16_t raster_x, uint8_t layer,
                          uint32_t *row_pixel) {
  if (tile_sets_[layer].enabled) {
    // TODO support for linear tile sheets.  this assumes a 256x256 sheet
    // of 16x16 tiles for now.
    const auto &tile_set = tile_sets_[layer];

    uint16_t adjusted_x = raster_x;
    uint16_t adjusted_y = raster_y_;

    // Try to take account of the horizontal and vertical scroll.
    // TODO: this is untested.
    if (tile_set.scroll_x_enable) {
      adjusted_x -= (tile_set.offset_x & 0x0f);
    }
    if (tile_set.scroll_y_enable) {
      adjusted_y -= (tile_set.offset_y & 0x0f);
    }

    if (adjusted_x > kVickyBitmapWidth || adjusted_y > kVickyBitmapHeight)
      return false;

    uint8_t screen_tile_row = adjusted_y / kTileSize;
    uint8_t screen_tile_sub_row = adjusted_y % kTileSize;

    uint8_t screen_tile_col = adjusted_x / kTileSize;
    uint8_t screen_tile_sub_col = adjusted_x % kTileSize;

    TileMem *tile_mem = &tile_mem_[layer];
    uint8_t tile_num = tile_mem->map[screen_tile_row][screen_tile_col];
    uint8_t *tile_sheet_bitmap = &video_ram_[tile_set.start_addr];

    uint8_t tile_sheet_column =
        tile_num % kTileSize; // the column in the tile sheet
    uint8_t tile_sheet_row = tile_num / kTileSize; // the row in the tile sheet

    // the physical memory location of the row in the sheet our tile is
    // in
    uint8_t *tile_bitmap_row =
        &tile_sheet_bitmap[(tile_sheet_row * kTileSize + screen_tile_sub_row) *
                           kTileSetStride];

    // the physical memory location of the column in the sheet our tile
    // is in
    uint8_t *tile_bitmap_column =
        &tile_bitmap_row[tile_sheet_column * kTileSize];

    uint8_t colour_index = tile_bitmap_column[screen_tile_sub_col];
    if (colour_index == 0)
      return false;

    *row_pixel = ColourCorrect(lut_[tile_set.lut][colour_index].v);
    return true;
  }
  return false;
}

bool Vicky::RenderMouseCursor(uint16_t raster_x, uint32_t *row_pixel) {
  // TODO: hook up.
  if ((raster_x >= mouse_pos_x_ && raster_x <= mouse_pos_x_ + 16) &&
      (raster_y_ >= mouse_pos_y_ && raster_y_ <= mouse_pos_y_ + 16)) {
    uint8_t *mouse_mem =
        mouse_cursor_select_ ? mouse_cursor_0_ : mouse_cursor_1_;
    uint8_t mouse_sub_col = raster_x % 16;
    uint8_t mouse_sub_row = raster_y_ % 16;
    uint8_t pixel_val = mouse_mem[mouse_sub_col + (mouse_sub_row * 16)];
    if (pixel_val == 0)
      return false;
    *row_pixel =
        ColourCorrect(pixel_val | (pixel_val << 8) | (pixel_val << 16));
    return true;
  }
  return false;
}

bool Vicky::RenderCharacterGenerator(uint16_t raster_x, uint32_t *row_pixel) {
  int16_t bitmap_x = raster_x;
  int16_t bitmap_y = raster_y_;

  // If the border is enabled, reduce the rendered area accordingly.
  if (border_enabled_) {
    bitmap_x -= kBorderWidth;
    bitmap_y -= kBorderHeight;
    if (bitmap_x < 0 || bitmap_y < 0)
      return false;
  }

  uint16_t row = bitmap_y / 8;
  uint16_t sub_row = bitmap_y % 8;

  uint8_t column = bitmap_x / 8;
  uint8_t sub_column = bitmap_x % 8;

  uint8_t character = text_mem_[column + (row * kColsPerLine)];
  uint8_t colour = text_colour_mem_[column + (row * kColsPerLine)];
  uint8_t fg_colour_num = (uint8_t)((colour & 0xf0) >> 4);
  uint8_t bg_colour_num = (uint8_t)(colour & 0x0f);
  uint8_t *character_font = &font_bank_[character * 8];
  uint8_t *cursor_font = &font_bank_[cursor_char_ * 8];

  uint32_t fg_colour = fg_colour_mem_[fg_colour_num];
  uint32_t bg_colour = bg_colour_mem_[bg_colour_num];

  // TODO: cursor colour?
  bool is_cursor_cell = cursor_state_ && cursor_reg_ & Vky_Cursor_Enable &&
                        (cursor_x_ == column && cursor_y_ == row);

  int pixel_pos = 1 << (7 - sub_column);
  if (is_cursor_cell && cursor_font[sub_row] & pixel_pos) {
    *row_pixel = ColourCorrect(fg_colour);
    return true;
  } else if (character_font[sub_row] & pixel_pos) {
    *row_pixel = ColourCorrect(is_cursor_cell ? bg_colour : fg_colour);
    return true;
  } else if (mode_ & Mstr_Ctrl_Text_Mode_En && !is_cursor_cell) {
    // note no bg color in overlay or when cursor on
    *row_pixel = ColourCorrect(bg_colour);
    return true;
  }
  return false;
}

uint32_t Vicky::ColourCorrect(uint32_t colour_val) {
  // Seems like GAMMA_en is always off? But too dim if we won't use it, so
  // we allow an override for it, and default it to on.
  if (!gamma_override_ && !(mode_ & Mstr_Ctrl_GAMMA_En))
    return colour_val;

  BGRAColour colour;
  colour.v = colour_val;
  BGRAColour corrected;
  corrected.bgra[0] = gamma_.b[colour.bgra[0]];
  corrected.bgra[1] = gamma_.g[colour.bgra[1]];
  corrected.bgra[2] = gamma_.r[colour.bgra[2]];
  corrected.bgra[3] = colour.bgra[3];

  return corrected.v;
}

void Vicky::set_scale(float scale) {
  scale_ = scale;
  glfwSetWindowSize(window_, kVickyBitmapWidth * scale,
                    kVickyBitmapHeight * scale);
}
