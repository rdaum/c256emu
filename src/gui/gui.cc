#include "gui/gui.h"

#include <imgui.h>

#include <SDL2/SDL_opengl.h>
#include <circular_buffer.hpp>
#include <glog/logging.h>

#include "gui/imgui_impl_opengl2.h"
#include "gui/imgui_impl_sdl.h"
#include "system.h"

namespace {

std::string Addr(uint32_t address) {
  std::stringstream stream;
  uint8_t bank = address >> 16;
  uint32_t addr = address & 0x0000ffff;

  stream << std::setfill('0') << std::setw(2) << std::hex << (int)bank << ":"
         << std::setw(4) << std::hex << addr;
  return stream.str();
}
} // namespace

GUI::~GUI() { Close(); }

void GUI::Start() {
  gui_thread_ = std::thread([this] {
    {
      std::lock_guard<std::mutex> lock(gui_mutex_);

      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
      SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
      SDL_WindowFlags window_flags =
          (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
      window_ =
          SDL_CreateWindow("c256emu", SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED, 640, 480, window_flags);
      CHECK(window_);
      gl_context_ = SDL_GL_CreateContext(window_);
      SDL_GL_MakeCurrent(window_, gl_context_);
      SDL_GL_SetSwapInterval(1); // Enable vsync

      // Setup Dear ImGui context
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      io_ = &ImGui::GetIO();
      io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

      // Setup Dear ImGui style
      ImGui::StyleColorsLight();

      ImGui_ImplSDL2_InitForOpenGL(window_, gl_context_);

      ImGui_ImplOpenGL2_Init();
    }
    running_ = true;
    while (running_) {
      auto refresh_interval =
          std::chrono::steady_clock::now() + std::chrono::milliseconds(16);
      Render();
      std::this_thread::sleep_until(refresh_interval);
    }
  });
}

void GUI::Close() {
  running_ = false;
  if (window_) {
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context_);
    SDL_DestroyWindow(window_);
  }
}

void GUI::Stop() {
  running_ = false;
  gui_thread_.join();
  Close();
}

void GUI::ProcessEvent(const SDL_Event &event) {
  std::lock_guard<std::mutex> lock(gui_mutex_);
  if (event.window.windowID == SDL_GetWindowID(window_)) {
    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
      Stop();
      return;
    }
    ImGui_ImplSDL2_ProcessEvent(&event);
  }
}

void GUI::Render() {
  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplSDL2_NewFrame(window_);
  ImGui::NewFrame();

  ImGui::SetNextWindowPos({0, 0});
  ImGui::SetNextWindowSize(io_->DisplaySize);
  if (ImGui::Begin("Debugger", nullptr,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoCollapse)) {

    // TODO: almost none of this is thread safe with the CPU

    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Appearing);
    static jm::circular_buffer<float, 128> mhz_buffer;
    static jm::circular_buffer<float, 128> fps_buffer;
    if (ImGui::CollapsingHeader("Profile")) {
      auto profile_info = system_->profile_info();
      mhz_buffer.push_back(profile_info.mhz_equiv);
      fps_buffer.push_back(profile_info.fps);
      ImGui::BeginGroup();
      ImGui::LabelText("FPS", "%f", profile_info.fps);
      ImGui::PlotLines("", mhz_buffer.data(), fps_buffer.size());
      ImGui::LabelText("Mhz", "%f", profile_info.mhz_equiv);
      ImGui::PlotLines("", mhz_buffer.data(), mhz_buffer.size());
      ImGui::EndGroup();
    }

    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Appearing);
    if (ImGui::CollapsingHeader("CPU")) {
      ImGui::BeginGroup();
      WDC65C816 *cpu = system_->cpu();
      uint32_t program_address = cpu->program_address();
      ImGui::LabelText("PC", "%s (%d)", Addr(program_address).c_str(),
                       program_address);
      ImGui::LabelText("Stack", "%s (%d)",
                       Addr(cpu->cpu_state.regs.sp.u16).c_str(),
                       cpu->cpu_state.regs.sp.u16);
      ImGui::LabelText("Native/Emulation", "%s",
                       cpu->mode_emulation ? "Emulation" : "Native");
      ImGui::LabelText("Accumulator width", "%s",
                       cpu->mode_long_a ? "16" : "8");
      ImGui::LabelText("Index width", "%s", cpu->mode_long_xy ? "16" : "8");
      ImGui::EndGroup();
    }
    if (ImGui::CollapsingHeader("Breakpoints")) {
      Automation *automation = system_->automation();
      auto breakpoints = automation->GetBreakpoints();
      static bool adding = false;
      if (ImGui::Button("Add")) {
        adding = true;
      }
      ImGui::Separator();
      for (const auto &breakpoint : breakpoints) {
        ImGui::Columns(3);
        uint8_t bank = breakpoint.address >> 16;
        uint16_t addr = breakpoint.address & 0x0000ffff;
        ImGui::LabelText("Address", "%02x:%04x", bank, addr);
        ImGui::NextColumn();
        ImGui::LabelText("Func", "%s", breakpoint.lua_function_name.c_str());
        ImGui::NextColumn();
        if (ImGui::Button("Delete")) {
          automation->ClearBreakpoint(breakpoint.address);
        }
        ImGui::NextColumn();
      }
      ImGui::Columns(1);
      if (adding) {
        ImGui::Columns(3);
        static uint32_t addr = 0;
        static char func[64];
        ImGui::InputScalar("Addr", ImGuiDataType_U32, &addr, nullptr, nullptr,
                           "%06X", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::NextColumn();
        ImGui::InputText("Func", func, 64);
        ImGui::NextColumn();
        if (ImGui::Button("OK") && func[0]) {
          automation->AddBreakpoint(addr, func);
          adding = false;
        }
        ImGui::NextColumn();
      }
      ImGui::Columns(1);
    }

    if (ImGui::CollapsingHeader("Memory Inspect")) {
      static std::vector<std::pair<uint32_t, uint8_t>> inspect_points;
      static bool adding = false;
      if (ImGui::Button("Add")) {
        adding = true;
      }
      ImGui::Separator();
      if (adding) {
        ImGui::Columns(3);
        static uint32_t addr = 0;
        static uint8_t bytes = 0x10;
        ImGui::InputScalar("Addr", ImGuiDataType_U32, &addr, nullptr, nullptr,
                           "%06X", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::NextColumn();
        ImGui::InputScalar("Size", ImGuiDataType_U8, &bytes, nullptr, nullptr,
                           "%02X", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::NextColumn();
        if (ImGui::Button("OK")) {
          // Make sure the same address isn't already there.
          auto found =
              std::find_if(inspect_points.begin(), inspect_points.end(),
                           [](const std::pair<uint32_t, uint8_t> &x) {
                             return x.first == addr;
                           });
          if (found == inspect_points.end()) {
            inspect_points.emplace_back(addr, bytes);
          }
          adding = false;
        }
        ImGui::Columns(1);
      }
      auto it = inspect_points.begin();
      while (it != inspect_points.end()) {
        auto &item = *it;
        bool erase = false;
        uint32_t address = item.first;
        std::string addr_label = Addr(address);
        if (ImGui::TreeNode(addr_label.c_str())) {
          uint32_t end_addr(address + item.second);
          std::vector<uint8_t> buffer;
          for (auto i = address; i < end_addr; i++) {
            buffer.push_back(system_->ReadByte(i));
          }
          ImGui::Columns(5);
          for (size_t i = 0; i < buffer.size(); i += 4) {
            ImGui::Text("%s", Addr(address + i).c_str());
            ImGui::NextColumn();
            for (int x = 0; x < 4; x++) {
              if (i + x < buffer.size()) {
                ImGui::Text("%02X", buffer[i + x]);
              }
              ImGui::NextColumn();
            }
          }
          ImGui::Columns(1);
          if (ImGui::Button("Delete")) {
            erase = true;
          }
          ImGui::TreePop();
        }
        if (erase)
          inspect_points.erase(it);
        else
          it++;
      }
    }
  }
  ImGui::End();

  ImGui::EndFrame();
  ImGui::Render();

  SDL_GL_MakeCurrent(window_, gl_context_);

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  glViewport(0, 0, (int)io_->DisplaySize.x, (int)io_->DisplaySize.y);
  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);

  ImDrawData *draw_data = ImGui::GetDrawData();
  ImGui_ImplOpenGL2_RenderDrawData(draw_data);
  SDL_GL_SwapWindow(window_);
}
