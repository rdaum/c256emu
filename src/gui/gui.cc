#include "gui/gui.h"

#include <imgui.h>

#include "gui/imgui_impl_opengl2.h"
#include "gui/imgui_impl_sdl.h"
#include "system.h"

#include <SDL2/SDL_opengl.h>
#include <glog/logging.h>

GUI::~GUI() {
  if (window_) {
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context_);
    SDL_DestroyWindow(window_);
  }
}

void GUI::Start() {

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
  window_ = SDL_CreateWindow("c256emu", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, 400, 200, window_flags);
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
  ImGui::StyleColorsClassic();

  ImGui_ImplSDL2_InitForOpenGL(window_, gl_context_);

  ImGui_ImplOpenGL2_Init();
}

void GUI::ProcessEvent(const SDL_Event &event) {
  if (event.window.windowID == SDL_GetWindowID(window_)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
  }
}

void GUI::Render() {
  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplSDL2_NewFrame(window_);
  ImGui::NewFrame();

  ImGui::Begin("Profile", nullptr, ImGuiWindowFlags_NoResize);
  ImGui::SetWindowSize({0.0f, 0.0f});
  auto profile_info = system_->profile_info();
  ImGui::BeginGroup();
  ImGui::LabelText("FPS", "%f", profile_info.fps);
  ImGui::LabelText("Mhz", "%f", profile_info.mhz_equiv);
  ImGui::EndGroup();
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
