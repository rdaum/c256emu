#include "loader.h"

#include <fstream>

#include <srecord/input/file.h>
#include <srecord/record.h>

void LoadFromHex(const std::string &filename, SystemBus *system_bus) {
  try {
    auto input = srecord::input_file::guess(filename);
    CHECK(input.get());
    LOG(INFO) << "Loading: " << filename << " ("
              << input->get_file_format_name() << ")";
    srecord::record record;
    while (input->read(record)) {
      uint32_t address = record.get_address();
      size_t num_bytes = record.get_length();
      const srecord::record::data_t *v = record.get_data();
      while (num_bytes--) {
        system_bus->WriteByte(address, *v++);
        address++;
      }
    }
  } catch (std::exception &e) {
    CHECK(false) << e.what();
  }
  return;
}

void LoadFromBin(const std::string &filename, uint32_t base_address,
                 SystemBus *system_bus) {
  std::ifstream in_file(filename);
  CHECK(in_file.is_open());
  uint32_t address = base_address;
  char byte;
  while (in_file.get(byte)) {
    system_bus->WriteByte(address, byte);
    address++;
  }
  LOG(INFO) << "Done @ " << address;
}