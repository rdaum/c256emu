#include "loader.h"

#include <fstream>

#include <srecord/input/file.h>
#include <srecord/record.h>

#include "cpu/cpu_65816.h"

void LoadFromHex(const std::string &filename, SystemBus *system_bus) {
  try {
    auto input = srecord::input_file::guess(filename);
    CHECK(input.get());
    LOG(INFO) << "Loading: " << filename << " ("
              << input->get_file_format_name() << ")";
    srecord::record record;
    while (input->read(record)) {
      Address address(record.get_address());
      size_t num_bytes = record.get_length();
      const srecord::record::data_t *v = record.get_data();
      while (num_bytes--) {
        system_bus->StoreByte(address, *v++);
        address.offset_++;
      }
    }
  } catch (std::exception e) {
    CHECK(false) << e.what();
  }
  return;
}

void LoadFromBin(const std::string &filename, const Address &base_address,
                 SystemBus *system_bus) {
  std::ifstream in_file(filename);
  CHECK(in_file.is_open());
  Address address = base_address;
  char byte;
  while (in_file.get(byte)) {
    system_bus->StoreByte(address, byte);
    address = address.WithOffsetNoWrapAround(1);
  }
  LOG(INFO) << "Done @ " << address;
}