#include "gui/gui.h"

#include <array>

#include <imgui.h>

#include <circular_buffer.hpp>
#include <glog/logging.h>

#include "bus/vicky.h"
#include "gui/automation_console.h"
#include "gui/imgui_impl_glfw.h"
#include "gui/imgui_impl_opengl2.h"
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

void glfw_error_callback(int error, const char *description) {
  LOG(ERROR) << "Glfw Error: " << error << " (" << description << ")";
}

constexpr const char *kScalingQualitiesLabels[]{"Nearest", "Linear", "Best"};

constexpr std::array<Vicky::ScalingQuality, 3> kScalingQualities{
    Vicky::ScalingQuality ::NEAREST,
    Vicky::ScalingQuality ::LINEAR,
    Vicky::ScalingQuality ::BEST,
};

} // namespace

GUI::~GUI() { Close(); }

void GUI::Start() {
  gui_thread_ = std::thread([this] {
    {
      std::lock_guard<std::mutex> lock(gui_mutex_);

      glfwSetErrorCallback(glfw_error_callback);
      CHECK(glfwInit());

      window_ = glfwCreateWindow(800, 800, "c256emu", nullptr, nullptr);
      CHECK(window_);

      glfwMakeContextCurrent(window_);
      glfwSwapInterval(1); // Enable vsync

      // Setup Dear ImGui context
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      io_ = &ImGui::GetIO();

      // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
      // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
      // // Enable Gamepad Controls

      // Setup Dear ImGui style
      ImGui::StyleColorsDark();
      // ImGui::StyleColorsClassic();

      // Setup Platform/Renderer bindings
      ImGui_ImplGlfw_InitForOpenGL(window_, true);
      ImGui_ImplOpenGL2_Init();
    }
    running_ = true;
    while (running_ && !glfwWindowShouldClose(window_)) {
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
    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
  }
}

void GUI::Stop() {
  running_ = false;
  Close();
}

void GUI::Render() {
  glfwPollEvents();

  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::SetNextWindowPos({0, 0}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(io_->DisplaySize, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("System.Ctrl", nullptr)) {
    // TODO: almost none of this is thread safe with the CPU. locking required.
    DrawProfiler();
    DrawCPUStatus();
    DrawVickySettings();
    DrawBreakpoints();
    DrawMemoryInspect();
    DrawStackInspect();
    DrawDirectPageInspect();
    DrawDisassembler();
  }

  static AutomationConsole console(system_->automation());
  static bool console_open;
  console.Draw("Automation", &console_open);

  ImGui::End();
  ImGui::EndFrame();
  ImGui::Render();

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  int display_w, display_h;
  glfwGetFramebufferSize(window_, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);

  // If you are using this code with non-legacy OpenGL header/contexts (which
  // you should not, prefer using imgui_impl_opengl3.cpp!!), you may need to
  // backup/reset/restore current shader using the commented lines below.
  // GLint last_program;
  // glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
  // glUseProgram(0);
  ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
  // glUseProgram(last_program);

  glfwMakeContextCurrent(window_);
  glfwSwapBuffers(window_);
}

void GUI::DrawDirectPageInspect() const {
  if (ImGui::CollapsingHeader("Direct Page Inspect")) {
    uint16_t dp = system_->cpu()->cpu_state.regs.d.u16;
    std::vector<uint8_t> buffer;
    for (int offset = 0; offset < 0x100; offset++) {
      buffer.push_back(system_->ReadByte(dp + offset));
    }

    ImGui::Columns(5);
    for (size_t i = 0; i < buffer.size(); i += 4) {
      ImGui::Text("%s", Addr(dp + i).c_str());
      ImGui::NextColumn();
      for (int x = 0; x < 4; x++) {
        if (i + x < buffer.size()) {
          ImGui::Text("%02X", buffer[i + x]);
        }
        ImGui::NextColumn();
      }
    }
    ImGui::Columns(1);
  }
}

void GUI::DrawStackInspect() const {
  if (ImGui::CollapsingHeader("Stack Inspect")) {
    uint16_t sp = system_->cpu()->cpu_state.regs.sp.u16;
    std::vector<uint8_t> buffer;
    // Stuff bytes in the buffer descending from sp down 256 bytes.
    for (int offset = 0; offset < 0x100; offset++) {
      buffer.push_back(system_->ReadByte(sp - offset));
    }

    // Draw in ascending order.
    ImGui::Columns(5);
    for (size_t i = 0; i < buffer.size(); i += 4) {
      ImGui::Text("%s", Addr(sp - i).c_str());
      ImGui::NextColumn();
      for (int x = 0; x < 4; x++) {
        if (i + x < buffer.size()) {
          ImGui::Text("%02X", buffer[i + x]);
        }
        ImGui::NextColumn();
      }
    }
    ImGui::Columns(1);
  }
}

void GUI::DrawVickySettings() const {
  ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Appearing);
  if (ImGui::CollapsingHeader("Vicky")) {
    float scale = system_->vicky()->scale();
    if (ImGui::InputFloat("Screen scale", &scale, 0.1)) {
      system_->vicky()->set_scale(scale);
    }
    bool gamma_overide = system_->vicky()->gamma_override();
    if (ImGui::Checkbox("Gamma override", &gamma_overide)) {
      system_->vicky()->set_gamma_override(gamma_overide);
    }
    Vicky::ScalingQuality scaling_quality = system_->vicky()->scaling_quality();
    if (ImGui::BeginCombo("Scaling quality",
                          kScalingQualitiesLabels[(int)scaling_quality])) {
      for (Vicky::ScalingQuality quality : kScalingQualities) {
        if (ImGui::Selectable(kScalingQualitiesLabels[(int)quality])) {
          system_->vicky()->set_scaling_quality(quality);
        }
      }
      ImGui::EndCombo();
    }
  }
}

void GUI::DrawDisassembler() const {
  if (ImGui::CollapsingHeader("Disassembler")) {
    Disassembler *disassembler = system_->cpu()->GetDisassembler();
    std::vector<CpuInstruction> program = disassembler->Disassemble(
        Disassembler::Config(), &system_->cpu()->cpu_state);
    ImGui::Columns(3);
    for (auto instruction : program) {
      ImGui::Text("%s", Addr(instruction.canonical_address).c_str());
      ImGui::NextColumn();
      ImGui::Text("%s", instruction.asm_string.substr(8, 12).c_str());
      ImGui::NextColumn();
      ImGui::Text("%s", instruction.asm_string.substr(20).c_str());
      ImGui::NextColumn();
    }
    ImGui::Columns(1);
  }
}

void GUI::DrawMemoryInspect() const {
  if (ImGui::CollapsingHeader("Memory Inspect")) {
    static std::vector<std::pair<cpuaddr_t, uint8_t>> inspect_points;
    static bool adding_inspect = false;
    if (!adding_inspect && ImGui::Button("Add")) {
      adding_inspect = true;
    }
    if (adding_inspect) {
      ImGui::Columns(4);
      static cpuaddr_t addr = 0;
      static uint8_t bytes = 0x10;
      ImGui::InputScalar("Addr", ImGuiDataType_U32, &addr, nullptr, nullptr,
                         "%06X", ImGuiInputTextFlags_CharsHexadecimal);
      ImGui::NextColumn();
      ImGui::InputScalar("Size", ImGuiDataType_U8, &bytes, nullptr, nullptr,
                         "%02X", ImGuiInputTextFlags_CharsHexadecimal);
      ImGui::NextColumn();
      if (ImGui::Button("OK")) {
        // Make sure the same address isn't already there.
        auto found = std::find_if(inspect_points.begin(), inspect_points.end(),
                                  [](const std::pair<cpuaddr_t, uint8_t> &x) {
                                    return x.first == addr;
                                  });
        if (found == inspect_points.end()) {
          inspect_points.emplace_back(addr, bytes);
        }
        adding_inspect = false;
      }
      ImGui::NextColumn();
      if (ImGui::Button("Cancel")) {
        adding_inspect = false;
      }
      ImGui::NextColumn();
      ImGui::Columns(1);
    }
    ImGui::Separator();

    // Draw the inspections.
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

void GUI::DrawBreakpoints() const {
  static std::vector<cpuaddr_t> breakpoints;

  if (ImGui::CollapsingHeader("Breakpoints")) {
    DebugInterface *debug_interface = system_->GetDebugInterface();
    static bool adding_breakpoint = false;
    if (!adding_breakpoint && ImGui::Button("Add")) {
      adding_breakpoint = true;
    }
    ImGui::Separator();
    auto it = breakpoints.begin();
    while (it != breakpoints.end()) {
      cpuaddr_t breakpoint = *it;
      ImGui::Columns(2);
      uint8_t bank = breakpoint >> 16;
      uint16_t addr = breakpoint & 0x0000ffff;
      ImGui::LabelText("Address", "%02x:%04x", bank, addr);
      ImGui::NextColumn();
      if (ImGui::Button("Delete")) {
        debug_interface->ClearBreakpoint(breakpoint);
        breakpoints.erase(it);
      } else {
        it++;
      }
      ImGui::NextColumn();
    }
    ImGui::Columns(1);
    if (adding_breakpoint) {
      ImGui::Columns(3);
      static cpuaddr_t addr = 0;
      ImGui::InputScalar("Addr", ImGuiDataType_U32, &addr, nullptr, nullptr,
                         "%06X", ImGuiInputTextFlags_CharsHexadecimal);
      ImGui::NextColumn();
      if (ImGui::Button("OK")) {
        breakpoints.push_back(addr);
        system_->cpu()->AddBreakpoint(addr, [](EmulatedCpu *) {});
        adding_breakpoint = false;
      }
      ImGui::NextColumn();
      if (ImGui::Button("Cancel")) {
        adding_breakpoint = false;
      }
      ImGui::NextColumn();
      ImGui::Columns(1);
    }
  }
}
void GUI::DrawCPUStatus() const {
  ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Appearing);
  if (ImGui::CollapsingHeader("CPU")) {
    ImGui::BeginGroup();
    WDC65C816 *cpu = system_->cpu();
    uint32_t program_address = cpu->program_address();
    ImGui::LabelText("PC", "%s (%d)", Addr(program_address).c_str(),
                     program_address);
    ImGui::LabelText("Native/Emulation", "%s",
                     cpu->mode_emulation ? "Emulation" : "Native");
    ImGui::LabelText("Accumulator width", "%s", cpu->mode_long_a ? "16" : "8");
    ImGui::LabelText("Index width", "%s", cpu->mode_long_xy ? "16" : "8");
    ImGui::LabelText("A", "%06x (%d)", cpu->a(), cpu->a());
    ImGui::LabelText("X", "%06x (%d)", cpu->x(), cpu->x());
    ImGui::LabelText("Y", "%06x (%d)", cpu->y(), cpu->x());
    ImGui::LabelText("Stack", "%s (%d)",
                     Addr(cpu->cpu_state.regs.sp.u16).c_str(),
                     cpu->cpu_state.regs.sp.u16);
    ImGui::LabelText("Direct page", "%s (%d)",
                     Addr(cpu->cpu_state.regs.d.u16).c_str(),
                     cpu->cpu_state.regs.d.u16);

    ImGui::Columns(3);
    DebugInterface *debug_interface = system_->GetDebugInterface();
    if (!debug_interface->paused() && ImGui::Button("Pause")) {
      debug_interface->Pause();
    }
    ImGui::NextColumn();

    if (debug_interface->paused()) {
      if (ImGui::Button("Resume")) {
        debug_interface->Resume();
      }
      ImGui::NextColumn();

      if (ImGui::Button("Step")) {
        debug_interface->SingleStep();
      }
      ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::EndGroup();
  }
}

void GUI::DrawProfiler() const {
  ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Appearing);
  static jm::circular_buffer<float, 128> mhz_buffer;
  static jm::circular_buffer<float, 128> fps_buffer;
  if (ImGui::CollapsingHeader("Profile")) {
    auto profile_info = system_->profile_info();
    mhz_buffer.push_back(profile_info.mhz_equiv);
    fps_buffer.push_back(profile_info.fps);
    ImGui::BeginGroup();

    ImGui::LabelText("FPS", "%f", profile_info.fps);
    std::vector<float> linear_fps(fps_buffer.size());
    std::copy(fps_buffer.begin(), fps_buffer.end(), linear_fps.begin());
    ImGui::PlotLines("", linear_fps.data(), linear_fps.size());
    ImGui::LabelText("Mhz", "%f", profile_info.mhz_equiv);

    std::vector<float> linear_mhz(fps_buffer.size());
    std::copy(mhz_buffer.begin(), mhz_buffer.end(), linear_mhz.begin());
    ImGui::PlotLines("", linear_mhz.data(), linear_mhz.size());
    ImGui::EndGroup();
  }
}
