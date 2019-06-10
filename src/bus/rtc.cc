#include "bus/rtc.h"

#include <chrono>
#include <ctime>

namespace {

constexpr uint32_t kRtcSec = 0x0800;      // Seconds Register
constexpr uint32_t kRtcSecAlarm = 0x0801; // Seconds Alarm Register
constexpr uint32_t kRtcMin = 0x0802;      // Minutes Register
constexpr uint32_t kRtcMinAlarm = 0x0803; // Minutes Alarm Register
constexpr uint32_t kRtcHrs = 0x0804;      // Hours Register
constexpr uint32_t kRtcHrsAlarm = 0x0805; // Hours Alarm Register
constexpr uint32_t kRtcDay = 0x0806;      // Day Register
constexpr uint32_t kRtcDayAlarm = 0x0807; // Day Alarm Register
constexpr uint32_t kRtcDOW = 0x0808;      // Day of Week Register
constexpr uint32_t kRtcMonth = 0x0809;    // Month Register
constexpr uint32_t kRtcYear = 0x080A;     // Year Register
constexpr uint32_t kRtcRates = 0x080B;    // Rates Register
constexpr uint32_t kRtcEnable = 0x080C;   // Enables Register
constexpr uint32_t kRtcFlags = 0x080D;    // Flags Register
constexpr uint32_t kRtcCtrl = 0x080E;     // Control Register
constexpr uint32_t kRtcCentury = 0x080F;  // Century Register

uint8_t Trunc(int val) { return val % 10 | (val / 10) << 4; }
} // namespace

void Rtc::StoreByte(uint32_t addr, uint8_t v) {
  LOG(INFO) << "Set: " << addr << " = " << std::hex << (int)v;
  // TODO register settings, alarm
}

uint8_t Rtc::ReadByte(uint32_t addr) {
  auto now = std::chrono::system_clock::now();
  const time_t time = std::chrono::system_clock::to_time_t(now);
  auto localtime = std::localtime(&time);
  if (addr == kRtcSec) {
    return Trunc(localtime->tm_sec);
  }
  if (addr == kRtcMin) {
    return Trunc(localtime->tm_min);
  }
  if (addr == kRtcHrs) {
    return Trunc(localtime->tm_hour);
  }
  if (addr == kRtcDay) {
    return Trunc(localtime->tm_mday);
  }
  if (addr == kRtcMonth) {
    return Trunc(localtime->tm_mon + 1);
  }
  if (addr == kRtcYear) {
    return Trunc(localtime->tm_year % 100);
  }
  if (addr == kRtcCentury) {
    return Trunc(localtime->tm_year / 100);
  }
  // TODO: alarm, flags
  return 0;
}
