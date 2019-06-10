#include "bus/vicky.h"

#include <chrono>
#include <functional>
#include <thread>

#include "bus/int_controller.h"
#include "bus/keyboard.h"
#include "bus/sdl_to_atset_keymap.h"
#include "bus/system.h"
#include "bus/vicky_def.h"
#include "vicky.h"

namespace {

constexpr uint8_t kColsPerLine = 128;
constexpr uint8_t kTileSize = 16;
constexpr uint8_t kSpriteSize = 32;

using UniqueTexturePtr =
    std::unique_ptr<SDL_Texture, std::function<void(SDL_Texture *)>>;

bool Set16(uint32_t addr, uint32_t start_addr, uint16_t *dest, uint8_t v) {
  uint16_t o = addr - start_addr;
  if (o > 1) return false;
  if (o == 0)
    *dest = (*dest & 0xff00) | v;
  else
    *dest = (*dest & 0x00ff) | (v << 8);
  return true;
}

bool Set24(uint32_t addr, uint32_t start_addr, uint32_t *dest, uint8_t v) {
  uint16_t o = addr - start_addr;
  if (o > 2) return false;
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
}  // namespace

Vicky::Vicky(System *system, InterruptController *int_controller)
    : sys_(system), int_controller_(int_controller) {
  memset(fg_colour_mem_, 0, sizeof(fg_colour_mem_));
  memset(bg_colour_mem_, 0, sizeof(bg_colour_mem_));
  memset(video_ram_, 0, sizeof(video_ram_));
  memset(tile_sets_, 0, sizeof(tile_sets_));
  memset(sprites_, 0, sizeof(sprites_));
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
  map(CS_TEXT_MEM_PTR, text_mem_, CS_COLOR_MEM_PTR - CS_TEXT_MEM_PTR);
  map(CS_COLOR_MEM_PTR, text_colour_mem_, BTX_START - CS_COLOR_MEM_PTR);
  map(FONT_MEMORY_BANK0, font_bank_, 0x1000);
  map(GRPH_LUT0_PTR, (uint8_t *)lut_, GAMMA_B_LUT_PTR - GRPH_LUT0_PTR);
}

void Vicky::Start() {
  SDL_Init(SDL_INIT_VIDEO);
  window_ = SDL_CreateWindow("Vicky", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, kVickyBitmapWidth,
                             kVickyBitmapHeight, SDL_WINDOW_OPENGL);

  CHECK(window_);
  renderer_ = SDL_CreateRenderer(window_, -1, 0);
  vicky_texture_ = SDL_CreateTexture(
      renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
      kVickyBitmapWidth, kVickyBitmapHeight);

  CHECK(renderer_);
}

Vicky::~Vicky() {
  if (window_) {
    SDL_DestroyTexture(vicky_texture_);
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
  }
}

uint8_t Vicky::ReadByte(uint32_t addr) { return 0; }

void Vicky::StoreByte(uint32_t addr, uint8_t v) {
  uint16_t offset = addr;

  switch (offset) {
    case VKY_TXT_CURSOR_X_REG_L:
      cursor_x_ = (cursor_x_ & 0xFF00) | v;
      return;
    case VKY_TXT_CURSOR_X_REG_H:
      cursor_x_ = (cursor_x_ & 0xFF) | ((uint16_t)v << 8);
      return;
    case VKY_TXT_CURSOR_Y_REG_L:
      cursor_y_ = (cursor_y_ & 0xFF00) | v;
      return;
    case VKY_TXT_CURSOR_Y_REG_H:
      cursor_y_ = (cursor_y_ & 0xFF) | ((uint16_t)v << 8);
      return;
    case BORDER_COLOR_B:
      border_colour_.bgra[0] = v;
      return;
    case BORDER_COLOR_G:
      border_colour_.bgra[1] = v;
      return;
    case BORDER_COLOR_R:
      border_colour_.bgra[2] = v;
      return;
    case MOUSE_PTR_X_POS_L:
      mouse_pos_x_ = (mouse_pos_x_ & 0xFF00) | v;
      return;
    case MOUSE_PTR_X_POS_H:
      mouse_pos_x_ = (mouse_pos_x_ & 0xFF) | ((uint16_t)v << 8);
      return;
    case MOUSE_PTR_Y_POS_L:
      mouse_pos_y_ = (mouse_pos_y_ & 0xFF00) | v;
      return;
    case MOUSE_PTR_Y_POS_H:
      mouse_pos_y_ = (mouse_pos_y_ & 0xFF) | ((uint16_t)v << 8);
      return;
  }

  if (addr >= FG_CHAR_LUT_PTR && addr < BG_CHAR_LUT_PTR) {
    memcpy((uint8_t *)fg_colour_mem_ + addr - FG_CHAR_LUT_PTR, &v, 1);
    return;
  } else if (addr >= BG_CHAR_LUT_PTR && addr < 0x1fc0) {
    memcpy((uint8_t *)bg_colour_mem_ + addr - BG_CHAR_LUT_PTR, &v, 1);
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
  } else if (addr >= MOUSE_PTR_GRAP0_START && addr <= MOUSE_PTR_GRAP0_END) {
    mouse_cursor_0_[addr - MOUSE_PTR_GRAP0_START] = v;
  } else if (addr >= MOUSE_PTR_GRAP1_START && addr <= MOUSE_PTR_GRAP1_END) {
    mouse_cursor_1_[addr - MOUSE_PTR_GRAP1_START] = v;
  }

  if (addr == BM_CONTROL_REG) {
    bitmap_enabled_ = v & 0x01;
    bitmap_lut_ = (v & 0b01110000) >> 4;
    return;
  }

  if (Set16(addr, MASTER_CTRL_REG_L, &mode_, v)) {
    LOG(INFO) << "Set mode: " << mode_ << " Address: " << addr;
    return;
  }

  if (addr >= TL0_CONTROL_REG && addr <= TL3_MAP_Y_STRIDE_H) {
    uint16_t sprite_offset = offset - TL0_CONTROL_REG;
    uint8_t tile_num = sprite_offset / 8;
    uint8_t register_num = sprite_offset % 8;
    if (register_num == 0) {
      tile_sets_[tile_num].enabled = v & 0x01;
      tile_sets_[tile_num].lut = (v & 0b00001110) >> 1;
      tile_sets_[tile_num].tiled_sheet = (v & 0x80);
    } else if (register_num == 1) {
      tile_sets_[tile_num].start_addr =
          (tile_sets_[tile_num].start_addr & 0x00ffff00) | v;
    } else if (register_num == 2) {
      tile_sets_[tile_num].start_addr =
          (tile_sets_[tile_num].start_addr & 0x00ff00ff) | (v << 8);
    } else if (register_num == 3) {
      tile_sets_[tile_num].start_addr =
          (tile_sets_[tile_num].start_addr & 0x0000ffff) | (v << 16);
    } else if (register_num == 4) {
      SetLower8(&tile_sets_[tile_num].stride_x, v);
    } else if (register_num == 5) {
      SetHigher8(&tile_sets_[tile_num].stride_x, v);
    } else if (register_num == 6) {
      SetLower8(&tile_sets_[tile_num].stride_y, v);
    } else if (register_num == 7) {
      SetHigher8(&tile_sets_[tile_num].stride_y, v);
    } else {
      LOG(ERROR) << "Unsupported tile reg: " << register_num;
    }
    return;
  }

  if (addr >= TILE_MAP0 && addr <= TILE_MAP3 + 2048) {
    uint16_t tile_offset = offset - TILE_MAP0;
    uint8_t tile_num = tile_offset / 0x800;
    uint8_t map_offset = tile_offset % 0x800;
    tile_sets_[tile_num].tile_map.mem[map_offset] = v;
    return;
  }

  if (Set24(addr, BACKGROUND_COLOR_B, &background_bgr_.v, v)) {
    return;
  }

  if (addr >= SP00_CONTROL_REG && addr <= SP31_CONTROL_REG + 8) {
    uint16_t sprite_offset = offset - SP00_CONTROL_REG;
    uint16_t sprite_num = sprite_offset / 0x08;
    uint16_t register_num = sprite_offset % 0x08;
    Sprite &sprite = sprites_[sprite_num];
    if (register_num == 0) /* control register */ {
      uint8_t layer = (v & 0b01110000) >> 4;
      sprite.layer = layer;
      sprite.enabled = v & 0x01;
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

  if (addr == VKY_TXT_CURSOR_CTRL_REG) {
    cursor_reg_ = v;
    return;
  }
  if (addr == VKY_TXT_CURSOR_CHAR_REG) {
    cursor_char_ = v;
    return;
  }
  if (addr == VKY_TXT_CURSOR_COLR_REG) {
    cursor_colour_ = v;
    return;
  }

  if (addr == MOUSE_PTR_CTRL_REG_L) {
    mouse_cursor_enable_ = v & 0x01;
    mouse_cursor_select_ = v & 0x02;
    return;
  }
  if (addr == BORDER_CTRL_REG) {
    border_enabled_ = v & (1 << Border_Ctrl_Enable);
    return;
  }
  LOG(INFO) << "Unknown Vicky register: " << std::hex << addr;
}

void Vicky::RenderLine() {
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
    uint32_t *row_pixel = &row_pixels[raster_x];

    // Background colour
    row_pixels[raster_x] = ApplyGamma(background_bgr_.v);

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
    if (run_char_gen) RenderCharacterGenerator(raster_x, row_pixel);

    // Mouse
    if (mouse_cursor_enable_) RenderMouseCursor(raster_x, row_pixel);

    if (border_enabled_) {
      uint32_t border_colour = ApplyGamma(border_colour_.v);
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
    SDL_UpdateTexture(vicky_texture_, nullptr, frame_buffer_,
                      kVickyBitmapWidth * sizeof(uint32_t));
    SDL_RenderCopy(renderer_, vicky_texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
    raster_y_ = 0;
  }
}

bool Vicky::RenderBitmap(uint16_t raster_x, uint32_t *row_pixel) {
  uint8_t *indexed_row =
      video_ram_ + bitmap_addr_offset_ + (raster_y_ * kVickyBitmapWidth);
  uint8_t colour_index = indexed_row[raster_x];
  if (colour_index == 0) return false;
  *row_pixel = ApplyGamma(lut_[bitmap_lut_][colour_index].v);
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
    if (colour_index == 0) return false;
    *row_pixel = ApplyGamma(lut_[sprite.lut][colour_index].v);
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
    uint8_t screen_tile_row = raster_y_ / kTileSize;
    uint8_t screen_tile_sub_row = raster_y_ % kTileSize;

    uint8_t screen_tile_col = raster_x / kTileSize;
    uint8_t screen_tile_sub_col = raster_x % kTileSize;

    uint8_t tile_num = tile_set.tile_map.map[screen_tile_row][screen_tile_col];
    uint8_t *tile_sheet_bitmap = &video_ram_[tile_set.start_addr];

    uint8_t tile_sheet_column =
        tile_num % kTileSize;  // the column in the tile sheet
    uint8_t tile_sheet_row = tile_num / kTileSize;  // the row in the tile sheet

    // the physical memory location of the row in the sheet our tile is
    // in
    uint8_t *tile_bitmap_row =
        &tile_sheet_bitmap[(tile_sheet_row * kTileSize + screen_tile_sub_row) *
                           tile_set.stride_x];

    // the physical memory location of the column in the sheet our tile
    // is in
    uint8_t *tile_bitmap_column =
        &tile_bitmap_row[tile_sheet_column * kTileSize];

    uint8_t colour_index = tile_bitmap_column[screen_tile_sub_col];
    if (colour_index == 0) return false;

    *row_pixel = ApplyGamma(lut_[tile_set.lut][colour_index].v);
    return true;
  }
  return false;
}

bool Vicky::RenderMouseCursor(uint16_t raster_x, uint32_t *row_pixel) {
  SDL_GetMouseState(&mouse_pos_x_, &mouse_pos_y_);
  if ((raster_x >= mouse_pos_x_ && raster_x <= mouse_pos_x_ + 16) &&
      (raster_y_ >= mouse_pos_y_ && raster_y_ <= mouse_pos_y_ + 16)) {
    uint8_t *mouse_mem =
        mouse_cursor_select_ ? mouse_cursor_0_ : mouse_cursor_1_;
    uint8_t mouse_sub_col = raster_x % 16;
    uint8_t mouse_sub_row = raster_y_ % 16;
    uint8_t pixel_val = mouse_mem[mouse_sub_col + (mouse_sub_row * 16)];
    if (pixel_val == 0) return false;
    *row_pixel = ApplyGamma(pixel_val | (pixel_val << 8) | (pixel_val << 16));
    return true;
  }
  return false;
}

bool Vicky::RenderCharacterGenerator(uint16_t raster_x, uint32_t *row_pixel) {
  uint16_t bitmap_x = raster_x;
  uint16_t bitmap_y = raster_y_;

  // If the border is enabled, reduce the rendered area accordingly.
  if (border_enabled_) {
    bitmap_x -= kBorderWidth;
    bitmap_y -= kBorderHeight;
  }
  const uint16_t cursor_x = cursor_x_;
  const uint16_t cursor_y = cursor_y_;
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
                        (cursor_x == column && cursor_y == row);

  int pixel_pos = 1 << (7 - sub_column);
  if (is_cursor_cell && cursor_font[sub_row] & pixel_pos) {
    *row_pixel = ApplyGamma(fg_colour);
    return true;
  } else if (character_font[sub_row] & pixel_pos) {
    *row_pixel = ApplyGamma(is_cursor_cell ? bg_colour : fg_colour);
    return true;
  } else if (mode_ & Mstr_Ctrl_Text_Mode_En && !is_cursor_cell) {
    // note no bg color in overlay or when cursor on
    *row_pixel = ApplyGamma(bg_colour);
    return true;
  }
  return false;
}

uint32_t Vicky::ApplyGamma(uint32_t colour_val) {
  // Seems like GAMMA_en is always off? But too dim if we won't use it.

  //  if (!(mode_ & Mstr_Ctrl_GAMMA_En)) return colour_val;
  BGRAColour colour{.v = colour_val};
  BGRAColour corrected{.bgra = {
                           gamma_.b[colour.bgra[0]],
                           gamma_.g[colour.bgra[1]],
                           gamma_.r[colour.bgra[2]],
                           colour.bgra[0],
                       }};

  return corrected.v;
}
