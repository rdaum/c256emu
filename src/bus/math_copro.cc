#include "bus/math_copro.h"

#include <glog/logging.h>
#include <functional>
namespace {
template <typename T, typename R>
R Multiply(T a, T b) {
  return a * b;
}
template <typename T, typename R>
R Divide(T a, T b) {
  if (b == 0)
    return 0;
  return a / b;
}
template <typename T, typename R>
R Remainder(T a, T b) {
  if (b == 0)
    return 0;
  return a % b;
}
template <typename T, typename R>
R Add(T a, T b) {
  return a + b;
}

bool Set16(uint32_t addr,
           uint32_t start_addr,
           MathCoprocessor::IVal* d,
           uint8_t v) {
  uint16_t o = addr - start_addr;
  if (o > 1)
    return false;
  d->bytes[o] = v;
  return true;
}

bool Set32(uint32_t addr, uint32_t start_addr, int32_t* d, uint8_t v) {
  uint16_t o = addr - start_addr;
  if (o > 3)
    return false;
  if (o == 3) {
    *d = ((*d) & 0x00ffffff) | (v << 24);
    return true;
  }
  if (o == 2) {
    *d = ((*d) & 0xff00ffff) | (v << 16);
    return true;
  }
  if (o == 1) {
    *d = ((*d) & 0xffff00ff) | (v << 8);
    return true;
  }
  if (o == 0) {
    *d = ((*d) & 0xffffff00) | v;
    return true;
  }
  return false;
}

bool Get32(uint32_t addr,
           uint32_t start_addr,
           const MathCoprocessor::LongVal& r,
           uint8_t* result) {
  uint16_t o = addr - start_addr;
  if (o > 3)
    return false;
  *result = r.bytes[o];
  return true;
}

bool Get16(uint32_t addr,
           uint32_t start_addr,
           const MathCoprocessor::WordVal& r,
           uint8_t* result) {
  uint16_t o = addr - start_addr;
  if (o > 2)
    return false;
  *result = r.bytes[o];
  return true;
}

MathCoprocessor::MulRegisters ZeroMul() {
  MathCoprocessor::MulRegisters o;
  o.a.u_int = 0;
  o.b.u_int = 0;
  o.result.uint_32 = 0;
  return o;
}

MathCoprocessor::DivRegisters ZeroDiv() {
  MathCoprocessor::DivRegisters o;
  o.a.u_int = 0;
  o.b.u_int = 0;
  o.result.uint_16 = 0;
  o.remainder.uint_16 = 0;
  return o;
}
}  // namespace

MathCoprocessor::MathCoprocessor()
    : m0_(ZeroMul()),
      m1_(ZeroMul()),
      d0_(ZeroDiv()),
      d1_(ZeroDiv()),
      adder32_a_(0),
      adder32_b_(0) {
  adder32_r_.uint_32 = 0;
}

void MathCoprocessor::StoreByte(uint32_t addr, uint8_t v) {
  if (Set16(addr, M0_OPERAND_A, &m0_.a, v) ||
      Set16(addr, M0_OPERAND_B, &m0_.b, v)) {
    m0_.result.uint_32 = Multiply<uint16_t, uint32_t>(m0_.a.u_int, m0_.b.u_int);
    return;
  }
  if (Set16(addr, M1_OPERAND_A, &m1_.a, v) ||
      Set16(addr, M1_OPERAND_B, &m1_.b, v)) {
    m1_.result.int_32 = Multiply<int16_t, int32_t>(m1_.a.s_int, m1_.b.s_int);
    return;
  }
  if (Set16(addr, D0_OPERAND_A, &d0_.a, v) ||
      Set16(addr, D0_OPERAND_B, &d0_.b, v)) {
    d0_.result.uint_16 = Divide<uint16_t, uint16_t>(d0_.a.u_int, d0_.b.u_int);
    d0_.remainder.uint_16 =
        Remainder<uint16_t, uint16_t>(d0_.a.u_int, d0_.b.u_int);
    return;
  }
  if (Set16(addr, D1_OPERAND_A, &d1_.a, v) ||
      Set16(addr, D1_OPERAND_B, &d1_.b, v)) {
    d1_.result.int_16 = Divide<int16_t, int16_t>(d1_.a.s_int, d1_.b.s_int);
    d1_.remainder.int_16 =
        Remainder<int16_t, int16_t>(d1_.a.s_int, d1_.b.s_int);
    return;
  }
  if (Set32(addr, ADDER32_OPERAND_A, &adder32_a_, v) ||
      Set32(addr, ADDER32_OPERAND_B, &adder32_b_, v)) {
    adder32_r_.int_32 = Add<int32_t, int32_t>(adder32_a_, adder32_b_);
    return;
  }
}

uint8_t MathCoprocessor::ReadByte(uint32_t addr) {
  uint8_t result = 0;
  if (Get32(addr, M0_RESULT, m0_.result, &result))
    return result;
  if (Get32(addr, M1_RESULT, m1_.result, &result))
    return result;
  if (Get16(addr, D0_RESULT, d0_.result, &result))
    return result;
  if (Get16(addr, D0_REMAINDER, d0_.remainder, &result))
    return result;
  if (Get16(addr, D1_RESULT, d1_.result, &result))
    return result;
  if (Get16(addr, D1_REMAINDER, d1_.remainder, &result))
    return result;
  if (Get32(addr, ADDER32_RESULT, adder32_r_, &result))
    return result;

  return 0;
}
