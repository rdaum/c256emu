#pragma once

#include <SDL2/SDL.h>
#include <imgui.h>

class System;

class GUI {
public:
  explicit GUI(System *sys) : system_(sys) {}
  ~GUI();

  void Start();

  void Render();

  void ProcessEvent(const SDL_Event &event);

 private:
  System *system_;

  SDL_Window *window_ = nullptr;
  SDL_GLContext gl_context_;
  ImGuiIO *io_;
};