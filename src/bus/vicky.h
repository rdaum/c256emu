#pragma once

#include <GLFW/glfw3.h>
#include <glog/logging.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

#include "bus/register_utils.h"
#include "cpu.h"

class System;
class InterruptController;

constexpr uint8_t kBorderWidth = 16;
constexpr uint8_t kBorderHeight = 16;

constexpr uint16_t kVickyBitmapWidth = 640;
constexpr uint16_t kVickyBitmapHeight = 480;
constexpr uint16_t kVickyVBlankLines = 45;

constexpr uint32_t kRasterSize = kVickyBitmapWidth * kVickyBitmapHeight;

constexpr uint8_t kNumLayers = 4;

// Emulate the Vicky VDP.
// Text mode only for now.
class Vicky {
 public:
  Vicky(System* system, InterruptController* int_controller);

  ~Vicky();

  // Initialize.
  GLFWwindow *Start();

  // Render a single scan line and advance to the next.
  void RenderLine();

  void StoreByte(uint32_t addr, uint8_t v);
  uint8_t ReadByte(uint32_t addr);

  void InitPages(Page* vicky_page_start);

  inline bool is_vertical_end() { return raster_y_ == 479; }
  inline int current_scanline() { return raster_y_; }
  inline int max_scanline() { return kVickyBitmapHeight; }

  uint8_t* vram() { return video_ram_; }

  //  unsigned int window_id() const;

  void set_scale(float scale);
  float scale() const { return scale_; }

  enum class ScalingQuality { NEAREST, LINEAR, BEST };

  void set_gamma_override(bool override) { gamma_override_ = override; }
  bool gamma_override() const { return gamma_override_; }

 private:
  bool RenderBitmap(uint16_t raster_x, uint32_t* pixel);
  bool RenderCharacterGenerator(uint16_t raster_x, uint32_t* pixel);
  bool RenderMouseCursor(uint16_t raster_x, uint32_t* pixel);
  bool RenderTileMap(uint16_t raster_x, uint8_t layer, uint32_t* pixel);
  bool RenderSprites(uint16_t raster_x,
                     uint8_t layer,
                     uint32_t sprite_mask,
                     uint32_t* pixel);

  uint32_t ColourCorrect(uint32_t colour_val);

  System* sys_;
  InterruptController* int_controller_;

  float scale_ = 1.0;
  ScalingQuality scaling_quality_ = ScalingQuality::NEAREST;

  // Enable gamma correction even if the video mode doesn't say so.
  bool gamma_override_ = true;

  GLFWwindow *window_;

  union BGRAColour {
    uint32_t v;
    uint8_t bgra[4]{0, 0, 0, 0};
  };

  std::vector<Reg> registers_;

  // All register values and memory blocks.
  BGRAColour lut_[8][256];
  BGRAColour background_bgr_;

  struct {
    uint8_t b[256];
    uint8_t g[256];
    uint8_t r[256];
  } gamma_{};

  uint16_t mode_ = 0;

  uint8_t font_bank_[4096]{};
  uint8_t text_mem_[8192]{};
  uint8_t text_colour_mem_[8192]{};

  uint32_t fg_colour_mem_[16]{};
  uint32_t bg_colour_mem_[16]{};

  uint8_t cursor_colour_ = 0;
  uint8_t cursor_char_ = 0;
  uint8_t cursor_reg_ = 0;
  uint16_t cursor_x_ = 0;
  uint16_t cursor_y_ = 0;

  bool mouse_cursor_enable_ = false;
  bool mouse_cursor_select_ = false;  // false = 0, true = 1
  uint8_t mouse_cursor_0_[256]{};
  uint8_t mouse_cursor_1_[256]{};

  // TODO.
  uint16_t mouse_pos_x_ = 0;
  uint16_t mouse_pos_y_ = 0;

  bool cursor_state_ = false;
  std::chrono::time_point<std::chrono::steady_clock> last_cursor_flash_;

  bool bitmap_enabled_ = false;
  uint8_t bitmap_lut_ = 0;
  uint32_t bitmap_addr_offset_ = 0;

  uint8_t video_ram_[0x400000]{};

  struct TileSet {
    bool enabled = false;
    uint8_t lut = 0;
    bool tiled_sheet = false;  // true if a 256x256 sheet of 16x16 tiles
                               // otherwise a sequential row of 16x16 tiles
    uint32_t start_addr = 0;
    bool scroll_x_enable = false;
    bool scroll_y_enable = false;

    uint8_t offset_x;
    uint8_t offset_y;
  };
  TileSet tile_sets_[kNumLayers];

  union TileMem {
    uint8_t map[32][64];
    uint8_t mem[2048];
  };
  TileMem tile_mem_[4];

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

  bool border_enabled_{};
  BGRAColour border_colour_;

  uint8_t vblank_cnt_ = 0;
  uint16_t raster_y_ = 0;

  // Our physical frame buffer
  uint32_t frame_buffer_[kRasterSize];
  GLuint texture_id_;
};
