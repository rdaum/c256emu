#pragma once

#include <imgui.h>

class Automation;

class AutomationConsole {
public:
  explicit AutomationConsole(Automation *automation);
  ~AutomationConsole();

  void Draw(const char *title, bool *p_open);

private:
  void ClearLog();
  void AddLog(const char *fmt, ...);
  void ExecCommand(const char *command_line);
  static int TextEditCallbackStub(ImGuiInputTextCallbackData *data);
  int TextEditCallback(ImGuiInputTextCallbackData *data);

  Automation *automation_;

  char input_buf_[256];
  ImVector<char *> items_;
  ImVector<const char *> commands_;
  ImVector<char *> history_;
  int history_pos_; // -1: new line, 0..History.Size-1 browsing history.
  ImGuiTextFilter filter_;
  bool auto_scroll_;
  bool scroll_to_bottom_;
};

