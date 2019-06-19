#pragma once

#include <stdint.h>

constexpr uint32_t M0_OPERAND_A = 0x100;
constexpr uint32_t M0_OPERAND_B = 0x102;
constexpr uint32_t M0_RESULT = 0x104;

constexpr uint32_t M1_OPERAND_A = 0x108;
constexpr uint32_t M1_OPERAND_B = 0x10A;
constexpr uint32_t M1_RESULT = 0x10C;

constexpr uint32_t D0_OPERAND_A = 0x110;
constexpr uint32_t D0_OPERAND_B = 0x112;
constexpr uint32_t D0_RESULT = 0x114;
constexpr uint32_t D0_REMAINDER = 0x116;

constexpr uint32_t D1_OPERAND_A = 0x118;
constexpr uint32_t D1_OPERAND_B = 0x11A;
constexpr uint32_t D1_RESULT = 0x11C;
constexpr uint32_t D1_REMAINDER = 0x11E;

constexpr uint32_t ADDER32_OPERAND_A = 0x120;
constexpr uint32_t ADDER32_OPERAND_B = 0x124;
constexpr uint32_t ADDER32_RESULT = 0x128;

class MathCoprocessor {
 public:
  MathCoprocessor();

  // SystemBusDevice implementation
  void StoreByte(uint32_t addr, uint8_t v);
  uint8_t ReadByte(uint32_t addr);

  union IVal {
    int16_t s_int;
    uint16_t u_int;
    uint8_t bytes[2];
  };
  union LongVal {
    uint8_t bytes[4];
    uint32_t uint_32;
    int32_t int_32;
  };
  union WordVal {
    uint8_t bytes[2];
    uint16_t uint_16;
    int16_t int_16;
  };
  struct MulRegisters {
    IVal a;
    IVal b;
    LongVal result;
  };
  struct DivRegisters {
    IVal a;
    IVal b;
    WordVal result;
    WordVal remainder;
  };

 private:
  MulRegisters m0_;
  MulRegisters m1_;
  DivRegisters d0_;
  DivRegisters d1_;

  int32_t adder32_a_;
  int32_t adder32_b_;
  LongVal adder32_r_;
};