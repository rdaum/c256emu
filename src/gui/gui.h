#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include <SDL2/SDL.h>
#include <imgui.h>

class System;

class GUI {
public:
  explicit GUI(System *sys) : system_(sys) {}
  ~GUI();

  void Start();
  void Stop();
  void ProcessEvent(const SDL_Event &event);

 private:
  void Render();
  void Close();

  std::mutex gui_mutex_;
  std::thread gui_thread_;

  std::atomic_bool running_;

  System *system_;

  SDL_Window *window_ = nullptr;
  SDL_GLContext gl_context_;
  ImGuiIO *io_;
};