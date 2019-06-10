#include <gtest/gtest.h>

#include "bus/math_copro.h"
#include "cpu.h"

class MathCoprocessorTest : public ::testing::Test {
protected:
  void SetUp() override {
    bus.SetIo([this](uint32_t addr) -> uint8_t {
      return copro.ReadByte(addr);
    }, [this](uint32_t addr, uint8_t val) {
      copro.StoreByte(addr, val);
    }, 0, 0);
  }
  SimpleSystemBus<16> bus;
  MathCoprocessor copro;
};

TEST_F(MathCoprocessorTest, TestM0) {
  bus.PokeU16LE(M0_OPERAND_A, 12345);
  bus.PokeU16LE(M0_OPERAND_B, 22225);
  EXPECT_EQ(bus.PeekU32LE(M0_RESULT), 12345 * 22225);
}

TEST_F(MathCoprocessorTest, TestM1) {
  bus.PokeU16LE(M1_OPERAND_A, 12345);
  bus.PokeU16LE(M1_OPERAND_B, -22222);
  EXPECT_EQ((int32_t)bus.PeekU32LE(M1_RESULT), 12345 * -22222);
}

TEST_F(MathCoprocessorTest, TestD0) {
  bus.PokeU16LE(D0_OPERAND_A, 22222);
  bus.PokeU16LE(D0_OPERAND_B, 12345);
//  EXPECT_EQ(bus.PeekU32LE(D0_RESULT), 22222/ 12345);
  EXPECT_EQ(bus.PeekU32LE(D0_REMAINDER), 22222 % 12345);
}

TEST_F(MathCoprocessorTest, TestD1) {
  bus.PokeU16LE(D1_OPERAND_A, 22222);
  bus.PokeU16LE(D1_OPERAND_B, -12345);

//  EXPECT_EQ(bus.PeekU32LE(D1_RESULT), 22222 / -12345);
  EXPECT_EQ(bus.PeekU32LE(D1_REMAINDER), 22222 % -12345);
}

TEST_F(MathCoprocessorTest, TestAdd32) {
  bus.PokeU32LE(ADDER32_OPERAND_A, 222222);
  bus.PokeU32LE(ADDER32_OPERAND_B, -422222);
  EXPECT_EQ((int32_t)bus.PeekU32LE(ADDER32_RESULT), 222222 + -422222);
}

// TODO: Store 32 bit
