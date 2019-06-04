#include <gtest/gtest.h>

#include "bus/ram_device.h"
#include "cpu/cpu_65816.h"

// Really just here to prove we can write unit tests when we become that
// virtuous.

TEST(MemoryTest, TestSizing) {
  RAMDevice mem64k(Address(0x0, 0x0), Address(0x0, 0xffff));
  EXPECT_EQ(mem64k.Size(), 0xffff);

  RAMDevice mem64k_start_offset(Address(0x0, 0x200), Address(0x0, 0xffff));
  EXPECT_EQ(mem64k_start_offset.Size(), 0xffff - 0x200);

  RAMDevice mem64k_end_offset(Address(0x0, 0x0000),
                              Address(0x0, 0xffff - 0x200));
  EXPECT_EQ(mem64k_end_offset.Size(), 0xffff - 0x200);

  RAMDevice mem64k_start_and_end_offset(Address(0x0, 0x00200),
                                        Address(0x0, 0xffff - 0x200));
  EXPECT_EQ(mem64k_start_and_end_offset.Size(), 0xffff - 0x400);

  RAMDevice across_banks(Address(0x0, 0x00200), Address(0x1, 0xffff - 0x200));
  EXPECT_EQ(across_banks.Size(), 0x01ffff - 0x400);
}

TEST(MemoryTest, TwoBytesStoreRead) {
  RAMDevice memory(Address(0x0, 0x0), Address(0x0, 0xffff));
  SystemBus bus;
  bus.RegisterDevice(&memory);
  bus.StoreWord(Address(0x0, 0x02), 0xcafe);
  EXPECT_EQ(bus.ReadWord(Address(0x0, 0x02)), 0xcafe);
}

TEST(MemoryTest, AddressRead) {
  RAMDevice memory(Address(0x0, 0x0), Address(0x0, 0xffff));
  SystemBus bus;
  bus.RegisterDevice(&memory);
  bus.StoreWord(Address(0x0, 0x00), 0xfeba);
  bus.StoreByte(Address(0x0, 0x02), 0xca);
  EXPECT_EQ(bus.ReadAddressAt(Address(0x0, 0x00)), Address(0xca, 0xfeba));
}