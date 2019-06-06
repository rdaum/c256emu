#include "bus/rtc.h"

#include <chrono>
#include <ctime>

namespace {

constexpr Address kRtcSec(0xaf, 0x0800);      // Seconds Register
constexpr Address kRtcSecAlarm(0xaf, 0x0801); // Seconds Alarm Register
constexpr Address kRtcMin(0xaf, 0x0802);      // Minutes Register
constexpr Address kRtcMinAlarm(0xaf, 0x0803); // Minutes Alarm Register
constexpr Address kRtcHrs(0xaf, 0x0804);      // Hours Register
constexpr Address kRtcHrsAlarm(0xaf, 0x0805); // Hours Alarm Register
constexpr Address kRtcDay(0xaf, 0x0806);      // Day Register
constexpr Address kRtcDayAlarm(0xaf, 0x0807); // Day Alarm Register
constexpr Address kRtcDOW(0xaf, 0x0808);      // Day of Week Register
constexpr Address kRtcMonth(0xaf, 0x0809);    // Month Register
constexpr Address kRtcYear(0xaf, 0x080A);     // Year Register
constexpr Address kRtcRates(0xaf, 0x080B);    // Rates Register
constexpr Address kRtcEnable(0xaf, 0x080C);   // Enables Register
constexpr Address kRtcFlags(0xaf, 0x080D);    // Flags Register
constexpr Address kRtcCtrl(0xaf, 0x080E);     // Control Register
constexpr Address kRtcCentury(0xaf, 0x080F);  // Century Register

uint8_t Trunc(int val) { return val % 10 | (val / 10) << 4; }
} // namespace

void Rtc::StoreByte(const Address &addr, uint8_t v, uint8_t **address) {
  LOG(INFO) << "Set: " << addr << " = " << std::hex << (int)v;
  // TODO register settings, alarm
}

uint8_t Rtc::ReadByte(const Address &addr, uint8_t **address) {
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

bool Rtc::DecodeAddress(const Address &from_addr, Address &to_addr) {
  if (from_addr.InRange(kRtcSec, kRtcCentury)) {
    to_addr = from_addr;
    return true;
  }
  return false;
}
