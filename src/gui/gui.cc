#include "gui/gui.h"

#include <array>
#include <vector>

#include <imgui.h>
#include <glog/logging.h>
#include <gflags/gflags.h>

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

void window_close_callback(GLFWwindow *window) {
  System *system = (System *)glfwGetWindowUserPointer(window);
  system->SetStop();
}

template<typename T>
void pop_front(std::vector<T>& vec)
{
  assert(!vec.empty());
  vec.erase(vec.begin());
}

} // namespace

GUI::~GUI() { Close(); }

void GUI::Start(int x, int y) {
  gui_thread_ = std::thread([this, x, y] {
    {
      std::lock_guard<std::mutex> lock(gui_mutex_);

      glfwSetErrorCallback(glfw_error_callback);
      CHECK(glfwInit());

      window_ = glfwCreateWindow(999, 800, "c256emu", nullptr, nullptr);
      CHECK(window_);
      glfwSetWindowPos(window_, x, y);
      glfwMakeContextCurrent(window_);
      glfwSwapInterval(1); // Enable vsync
      glfwSetWindowUserPointer(window_, system_);
      glfwSetWindowCloseCallback(window_, window_close_callback);

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
  ImGui::SetNextWindowSize({333, 800}, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("System.Ctrl", nullptr)) {
    // TODO: almost none of this is thread safe with the CPU. locking required.
    DrawProfiler();
    DrawCPUStatus();
    DrawVickySettings();
    DrawBreakpoints();
    static AutomationConsole console(system_->automation());
    static bool console_open;
    console.Draw("Automation", &console_open);
  }

  ImGui::SetNextWindowPos({333, 0}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({333, 266}, ImGuiCond_FirstUseEver);
  DrawMemoryInspect();

  ImGui::SetNextWindowPos({333, 266}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({333, 266}, ImGuiCond_FirstUseEver);
  DrawStackInspect();

  ImGui::SetNextWindowPos({333, 532}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({333, 266}, ImGuiCond_FirstUseEver);
  DrawDirectPageInspect();

  ImGui::SetNextWindowPos({666, 0}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({333, 800}, ImGuiCond_FirstUseEver);
  DrawDisassembler();

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
  bool is_enabled = system_->direct_page_watch_enabled();
  static bool open = is_enabled;
  if (!ImGui::Begin("Direct Page Inspect", &open)) {
    if (is_enabled)
      system_->set_direct_page_watch_enabled(false);
    return ImGui::End();
  }
  if (!is_enabled) {
    system_->set_direct_page_watch_enabled(true);
    system_->PerformWatches();
  }
  std::vector<uint8_t> buffer = system_->direct_page_watch();
  if (buffer.empty()) {
    ImGui::End();
    return;
  }
  uint16_t dp = system_->cpu()->cpu_state.regs.d.u16;
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
  ImGui::End();
}

void GUI::DrawStackInspect() const {
  bool is_enabled = system_->stack_watch_enabled();
  static bool open = is_enabled;
  if (!ImGui::Begin("Stack Inspect", &open)) {
    if (is_enabled)
      system_->set_stack_watch_enabled(false);
    return ImGui::End();
  }
  if (!is_enabled) {
    system_->set_stack_watch_enabled(true);
    system_->PerformWatches();
  }
  std::vector<uint8_t> buffer = system_->stack_watch();
  if (buffer.empty()) {
    ImGui::End();
    return;
  }
  uint16_t sp = system_->watched_sp();

  uint32_t peek_rtsl = system_->peek_rtsl();
  ImGui::LabelText("RTS/L", "%02x:%04x", peek_rtsl >> 16,
                   peek_rtsl & 0x0000ffff);

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
  ImGui::End();
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
  }
}

void GUI::DrawDisassembler() {
  static bool open = false;
  if (!ImGui::Begin("Disassembler", &open)) {
    return ImGui::End();
  }
  static bool live_trace = false;
  ImGui::Checkbox("Live trace", &live_trace);
  if (!live_trace && !system_->GetDebugInterface()->paused()) {
    return ImGui::End();
  }
  Disassembler *disassembler = system_->cpu()->GetDisassembler();
  ImGui::Columns(3);
  ImVec4 yellow{0xff, 0xd3, 0x00, 0xff};
  ImVec4 red{0xde, 0x17, 0x38, 0xff};
  ImVec4 blue{0x00, 0xb2, 0xff, 0xff};
  ImVec4 white{0xff, 0xff, 0xff, 0xff};

  std::vector<cpuaddr_t> past_addrs;

  CpuTrace &cpu_trace = system_->cpu()->tracing;
  for (int i = cpu_trace.write; i < cpu_trace.addrs.size(); i++) {
    past_addrs.push_back(cpu_trace.addrs[i]);
  }
  for (int i = 0; i < cpu_trace.write; i++) {
    past_addrs.push_back(cpu_trace.addrs[i]);
  }

  for (cpuaddr_t addr : past_addrs) {
    const Disassembler::Config config{1};
    std::vector<CpuInstruction> past_program =
        disassembler->Disassemble(config, addr);
    for (auto instruction : past_program) {
      cpuaddr_t address = instruction.canonical_address;
      ImGui::TextColored(red, "%s", Addr(address).c_str());
      ImGui::NextColumn();
      ImGui::TextColored(yellow, "%s",
                         instruction.asm_string.substr(8, 12).c_str());
      ImGui::NextColumn();
      ImGui::TextColored(yellow, "%s",
                         instruction.asm_string.substr(20).c_str());
      ImGui::NextColumn();
    }
  }

  bool is_first = true;
  ImGui::Separator();
  const Disassembler::Config d_config{28};
  std::vector<CpuInstruction> upcoming_program =
      disassembler->Disassemble(d_config, &system_->cpu()->cpu_state);
  for (auto instruction : upcoming_program) {
    cpuaddr_t address = instruction.canonical_address;

    bool has_breakpoint = system_->automation()->HasBreakpoint(address);
    bool had_breakpoint = has_breakpoint;
    if (ImGui::Checkbox(Addr(address).c_str(), &has_breakpoint)) {
      if (had_breakpoint) {
        system_->automation()->ClearBreakpoint(address);
      } else {
        system_->automation()->AddBreakpoint(address, "");
      }
    }

    ImGui::NextColumn();
    ImGui::TextColored(is_first ? blue : white, "%s",
                       instruction.asm_string.substr(8, 12).c_str());
    ImGui::NextColumn();
    ImGui::TextColored(is_first ? blue : white, "%s",
                       instruction.asm_string.substr(20).c_str());
    ImGui::NextColumn();
    is_first = false;
  }
  ImGui::Columns(1);
  ImGui::End();
}

void GUI::DrawMemoryInspect() const {
  static bool open = false;
  if (!ImGui::Begin("Memory Inspect", &open)) {
    return ImGui::End();
  }
  const auto inspect_points = system_->memory_watches();
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
      auto found = std::find_if(
          inspect_points.begin(), inspect_points.end(),
          [](const System::MemoryWatch &x) { return x.start_addr == addr; });
      if (found == inspect_points.end()) {
        system_->AddMemoryWatch(addr, bytes);
        system_->PerformWatches();
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
    uint32_t address = item.start_addr;
    std::string addr_label = Addr(address);
    if (ImGui::TreeNode(addr_label.c_str())) {
      std::vector<uint8_t> buffer = item.last_results;
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
      system_->DelMemoryWatch(it->start_addr);
    else
      it++;
  }
  ImGui::End();
}

void GUI::DrawBreakpoints() {
  ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Appearing);
  if (ImGui::CollapsingHeader("Breakpoints")) {
    static bool adding_breakpoint = false;
    if (!adding_breakpoint && ImGui::Button("Add")) {
      adding_breakpoint = true;
    }
    ImGui::Separator();
    auto breakpoints = system_->automation()->GetBreakpoints();
    auto it = breakpoints.begin();
    while (it != breakpoints.end()) {
      cpuaddr_t breakpoint = it->address;
      ImGui::Columns(2);
      uint8_t bank = breakpoint >> 16;
      uint16_t addr = breakpoint & 0x0000ffff;
      ImGui::LabelText("Address", "%02x:%04x", bank, addr);
      ImGui::NextColumn();
      if (ImGui::Button("Delete")) {
        system_->automation()->ClearBreakpoint(it->address);
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
        system_->automation()->AddBreakpoint(addr, "");
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
    ImGui::Columns(6);
    if (ImGui::Button("RESET")) {
      system_->BootCPU(false);
    }
    ImGui::NextColumn();
    if (ImGui::Button("REBOOT")) {
      system_->BootCPU(true);
    }
    ImGui::NextColumn();
    if (ImGui::Button("IRQ")) {
      cpu->DoInterrupt(WDC65C816::IRQ);
    }
    ImGui::NextColumn();
    if (ImGui::Button("BRK")) {
      cpu->DoInterrupt(WDC65C816::BRK);
    }
    ImGui::NextColumn();
    if (ImGui::Button("NMI")) {
      cpu->DoInterrupt(WDC65C816::NMI);
    }
    ImGui::NextColumn();
    if (ImGui::Button("COP")) {
      cpu->DoInterrupt(WDC65C816::COP);
    }
    ImGui::Columns(1);
    uint32_t program_address = cpu->program_address();
    ImGui::LabelText("PC", "%s (%d)", Addr(program_address).c_str(),
                     program_address);
    ImGui::LabelText("Cycle #", "%lu", cpu->cpu_state.cycle);

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
        system_->PerformWatches();
      }
    } else {
      ImGui::NextColumn();
    }
    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::LabelText("Mode", "%s",
                     cpu->mode_emulation ? "Emulation" : "Native");
    ImGui::NextColumn();
    ImGui::LabelText("Acc", "%s", cpu->mode_long_a ? "16" : "8");
    ImGui::NextColumn();
    ImGui::LabelText("Index", "%s", cpu->mode_long_xy ? "16" : "8");
    ImGui::NextColumn();

    ImGui::Separator();
    ImGui::LabelText("A", "0x%04x (%d)", cpu->a(), cpu->a());
    ImGui::NextColumn();
    ImGui::LabelText("X", "0x%04x (%d)", cpu->x(), cpu->x());
    ImGui::NextColumn();
    ImGui::LabelText("Y", "0x%04x (%d)", cpu->y(), cpu->y());
    ImGui::NextColumn();

    ImGui::LabelText("C", "%d", cpu->cpu_state.carry);
    ImGui::NextColumn();
    ImGui::LabelText("N", "%d", cpu->cpu_state.negative);
    ImGui::NextColumn();
    ImGui::LabelText("V", "%d", cpu->cpu_state.is_overflow());
    ImGui::NextColumn();

    ImGui::LabelText("D", "%d", cpu->cpu_state.is_decimal());
    ImGui::NextColumn();
    ImGui::LabelText("Z", "%d", cpu->cpu_state.is_zero());
    ImGui::NextColumn();
    ImGui::LabelText("Int", "%d", cpu->cpu_state.interrupts_enabled());
    ImGui::NextColumn();


    ImGui::LabelText("DBR", "0x%02x (%d)",
                     cpu->cpu_state.code_segment_base >> 16,
                     cpu->cpu_state.code_segment_base >> 16);
    ImGui::NextColumn();
    ImGui::LabelText("SP", "0x%04x (%d)", cpu->cpu_state.regs.sp.u16,
                     cpu->cpu_state.regs.sp.u16);
    ImGui::NextColumn();
    ImGui::LabelText("D", "0x%04x (%d)", cpu->cpu_state.regs.d.u16,
                     cpu->cpu_state.regs.d.u16);
    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::Columns(2);
    ImGui::Checkbox("Fast MVN/MVP", &cpu->fast_block_moves);
    ImGui::NextColumn();

    ImGui::Columns(1);
    ImGui::EndGroup();
  }
}

void GUI::DrawProfiler() const {
  ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Appearing);
  static std::vector<float> mhz_buffer;
  static std::vector<float> fps_buffer;
  if (ImGui::CollapsingHeader("Profile")) {
    auto profile_info = system_->profile_info();
    mhz_buffer.push_back(profile_info.mhz_equiv);
    fps_buffer.push_back(profile_info.fps);
    if (mhz_buffer.size() > 128) {
      pop_front(mhz_buffer);
    }
    if (fps_buffer.size() > 128) {
      pop_front(fps_buffer);
    }
    ImGui::BeginGroup();

    ImGui::LabelText("FPS", "%f", profile_info.fps);

    ImGui::PlotLines("", fps_buffer.data(), fps_buffer.size());
    ImGui::LabelText("Mhz", "%f", profile_info.mhz_equiv);

    std::vector<float> linear_mhz(fps_buffer.size());
    std::copy(mhz_buffer.begin(), mhz_buffer.end(), linear_mhz.begin());
    ImGui::PlotLines("", linear_mhz.data(), linear_mhz.size());
    ImGui::EndGroup();
  }
}
