#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include <imgui.h>
#include <GLFW/glfw3.h>

class System;

class GUI {
public:
  explicit GUI(System *sys) : system_(sys) {}
  ~GUI();

  void Start();
  void Stop();

 private:
  void Render();
  void Close();

  void DrawProfiler() const;
  void DrawCPUStatus() const;
  void DrawBreakpoints() const;
  void DrawMemoryInspect() const;
  void DrawDisassembler() const;

  std::mutex gui_mutex_;
  std::thread gui_thread_;

  std::atomic_bool running_;

  System *system_;

  GLFWwindow *window_ = nullptr;
  ImGuiIO *io_;
  void DrawVickySettings() const;
void DrawStackInspect() const;
void DrawDirectPageInspect() const;
};