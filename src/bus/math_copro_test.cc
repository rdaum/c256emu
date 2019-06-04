#include <gtest/gtest.h>

#include "bus/math_copro.h"
#include "cpu/cpu_65816.h"

class MathCoprocessorTest : public ::testing::Test {
protected:
  void SetUp() override { bus.RegisterDevice(&copro); }
  SystemBus bus;
  MathCoprocessor copro;
};

TEST_F(MathCoprocessorTest, TestM0) {
  bus.StoreWord(M0_OPERAND_A, 12345);
  bus.StoreWord(M0_OPERAND_B, 22225);
  EXPECT_EQ(bus.ReadLong(M0_RESULT), 12345 * 22225);
}

TEST_F(MathCoprocessorTest, TestM1) {
  bus.StoreWord(M1_OPERAND_A, 12345);
  bus.StoreWord(M1_OPERAND_B, -22222);
  EXPECT_EQ((int32_t)bus.ReadLong(M1_RESULT), 12345 * -22222);
}

TEST_F(MathCoprocessorTest, TestD0) {
  bus.StoreWord(D0_OPERAND_A, 22222);
  bus.StoreWord(D0_OPERAND_B, 12345);
//  EXPECT_EQ(bus.ReadLong(D0_RESULT), 22222/ 12345);
  EXPECT_EQ(bus.ReadLong(D0_REMAINDER), 22222 % 12345);
}

TEST_F(MathCoprocessorTest, TestD1) {
  bus.StoreWord(D1_OPERAND_A, 22222);
  bus.StoreWord(D1_OPERAND_B, -12345);

//  EXPECT_EQ(bus.ReadLong(D1_RESULT), 22222 / -12345);
  EXPECT_EQ(bus.ReadLong(D1_REMAINDER), 22222 % -12345);
}

TEST_F(MathCoprocessorTest, TestAdd32) {
  bus.StoreLong(ADDER32_OPERAND_A, 222222);
  bus.StoreLong(ADDER32_OPERAND_B, -422222);
  EXPECT_EQ((int32_t)bus.ReadLong(ADDER32_RESULT), 222222 + -422222);
}

// TODO: Store 32 bit
