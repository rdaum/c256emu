#pragma once

#include <SDL2/SDL.h>
#include <glog/logging.h>

#include "cpu/cpu_65816.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

class System;
class InterruptController;

using UniqueTexturePtr =
    std::unique_ptr<SDL_Texture, std::function<void(SDL_Texture *)>>;

using UniquePixelFormatPtr =
    std::unique_ptr<SDL_PixelFormat, std::function<void(SDL_PixelFormat *)>>;

constexpr uint8_t kBorderWidth = 16;
constexpr uint8_t kBorderHeight = 16;

constexpr uint16_t kVickyBitmapWidth = 640;
constexpr uint16_t kVickyBitmapHeight = 480;

constexpr uint32_t kRasterSize = kVickyBitmapWidth * kVickyBitmapHeight;

constexpr uint8_t kNumLayers = 4;

// Emulate the Vicky VDP.
// Text mode only for now.
class Vicky : public SystemBusDevice {
public:
  Vicky(System *system, InterruptController *int_controller);

  ~Vicky() override;

  // Initialize.
  void Start();

  // Render a single scan line and advance to the next.
  void RenderLine();

  // SystemBusDevice implementation
  std::vector<MemoryRegion> GetMemoryRegions() override;
  void StoreByte(const Address &addr, uint8_t v, uint8_t **address) override;
  uint8_t ReadByte(const Address &addr, uint8_t **address) override;
  bool DecodeAddress(const Address &from_addr, Address &to_addr) override;

  inline bool is_vertical_end() { return raster_y_ == 479; }

private:
  bool RenderBitmap(uint16_t raster_x, uint32_t *pixel);
  bool RenderCharacterGenerator(uint16_t raster_x, uint32_t *pixel);
  bool RenderMouseCursor(uint16_t raster_x, uint32_t *pixel);
  bool RenderTileMap(uint16_t raster_x, uint8_t layer, uint32_t *pixel);
  bool RenderSprites(uint16_t raster_x, uint8_t layer, uint32_t sprite_mask,
                     uint32_t *pixel);

  uint32_t ApplyGamma(uint32_t colour_val);

  System *sys_;
  InterruptController *int_controller_;

  SDL_Window *window_;
  SDL_Renderer *renderer_;

  union BGRAColour {
    uint32_t v;
    uint8_t bgra[4] { 0, 0 ,0 , 0 };
  };
  BGRAColour lut_[8][256];
  BGRAColour background_bgr_;

  struct {
    uint8_t b[256];
    uint8_t g[256];
    uint8_t r[256];
  } gamma_;

  uint16_t mode_ = 0;

  uint8_t font_bank_0_[2048];
  uint8_t font_bank_1_[2048];
  uint8_t text_mem_[8192];
  uint8_t text_colour_mem_[8192];

  uint32_t fg_colour_mem_[16];
  uint32_t bg_colour_mem_[16];

  uint8_t cursor_colour_;
  uint8_t cursor_char_;
  uint8_t cursor_reg_;
  uint16_t cursor_x_ = 0;
  uint16_t cursor_y_ = 0;

  bool mouse_cursor_enable_ = false;
  bool mouse_cursor_select_ = false; // false = 0, true = 1
  uint8_t mouse_cursor_0_[256];
  uint8_t mouse_cursor_1_[256];

  // Set by SDL
  int mouse_pos_x_;
  int mouse_pos_y_;

  bool cursor_state_ = false;
  std::chrono::time_point<std::chrono::steady_clock> last_cursor_flash_;

  bool bitmap_enabled_ = false;
  uint8_t bitmap_lut_ = 0;
  uint32_t bitmap_addr_offset_ = 0;
  uint8_t video_ram_[0x400000];

  struct TileSet {
    bool enabled = false;
    uint8_t lut = 0;
    bool tiled_sheet = false; // true if a 256x256 sheet of 16x16 tiles
                              // otherwise a sequential row of 16x16 tiles
    uint32_t start_addr = 0;
    uint16_t stride_x;
    uint16_t stride_y;
    union {
      uint8_t map[32][64];
      uint8_t mem[2048];
    } tile_map;
  };
  TileSet tile_sets_[kNumLayers];

  struct Sprite {
    bool tile_striding = false;
    bool enabled = false;
    uint8_t layer = 0;
    uint32_t start_addr;
    uint8_t lut = 0;
    uint16_t x;
    uint16_t y;
  };
  Sprite sprites_[32];

  bool border_enabled_;
  BGRAColour border_colour_;

  uint16_t raster_y_ = 0;

  // Our physical frame buffer
  uint32_t frame_buffer_[kRasterSize];

  // Which is uploaded to this texture each frame.
  UniqueTexturePtr vicky_texture_;

  UniquePixelFormatPtr pixel_format_;
};
