#pragma once

#include "cpu/cpu_65816.h"

constexpr Address M0_OPERAND_A(0x00, 0x100);
constexpr Address M0_OPERAND_B(0x00, 0x102);
constexpr Address M0_RESULT(0x00, 0x104);

constexpr Address M1_OPERAND_A(0x00, 0x108);
constexpr Address M1_OPERAND_B(0x00, 0x10A);
constexpr Address M1_RESULT(0x00, 0x10C);

constexpr Address D0_OPERAND_A(0x00, 0x110);
constexpr Address D0_OPERAND_B(0x00, 0x112);
constexpr Address D0_RESULT(0x00, 0x114);
constexpr Address D0_REMAINDER(0x00, 0x116);

constexpr Address D1_OPERAND_A(0x00, 0x118);
constexpr Address D1_OPERAND_B(0x00, 0x11A);
constexpr Address D1_RESULT(0x00, 0x11C);
constexpr Address D1_REMAINDER(0x00, 0x11E);

constexpr Address ADDER32_OPERAND_A(0x00, 0x120);
constexpr Address ADDER32_OPERAND_B(0x00, 0x124);
constexpr Address ADDER32_RESULT(0x00, 0x128);


class MathCoprocessor : public SystemBusDevice {
 public:
  MathCoprocessor();

  // SystemBusDevice implementation
  void StoreByte(const Address& addr, uint8_t v, uint8_t** address) override;
  uint8_t ReadByte(const Address& addr, uint8_t** address) override;
  bool DecodeAddress(const Address& from_addr, Address& to_addr) override;

  union IVal {
    int16_t s_int;
    uint16_t u_int;
    uint8_t bytes[2];
  };
  union LongVal {
    uint8_t bytes[4];
    uint32_t uint_32;
    int32_t  int_32;
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