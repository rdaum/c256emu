#include "loader.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <boost/filesystem.hpp>

namespace {

unsigned char hstouc(std::string const &in) {
  static const std::string v{"0123456789ABCDEF"};

  return v.find(toupper(in[0])) * 16 + v.find(toupper(in[1]));
}

bool ReadHex(std::istream &line_stream, uint8_t *h) {
  line_stream.width(2);
  std::string byte_count_str;
  line_stream >> std::setw(2) >> byte_count_str;
  if (byte_count_str.size() != 2)
    return false;
  *h = hstouc(byte_count_str);
  return true;
}

} // namespace

void LoadFromS28(const std::string &filename, SystemBus *system_bus) {
  std::ifstream srec_stream;
  srec_stream.open(filename);
  while (!srec_stream.eof()) {
    std::string line;
    std::getline(srec_stream, line);
    if (line.empty())
      return;
    CHECK_EQ(line[0], 'S');
    char rec_type = line[1];

    std::stringstream line_stream(line.substr(2));

    // 24 bit data load
    if (rec_type == '2') {
      uint8_t byte_count;

      CHECK(ReadHex(line_stream, &byte_count));
      CHECK_EQ(byte_count * 2, (uint16_t)line.size() - 5);
      uint8_t address[3];
      CHECK(ReadHex(line_stream, &address[0]));
      CHECK(ReadHex(line_stream, &address[1]));
      CHECK(ReadHex(line_stream, &address[2]));
      byte_count -= 4;
      uint32_t addr = address[0] << 16 | address[1] << 8 | address[2];
      while (byte_count--) {
        uint8_t byte;
        CHECK(ReadHex(line_stream, &byte));

        system_bus->WriteByte(addr++, byte);
      }
      uint8_t checksum;
      CHECK(ReadHex(line_stream, &checksum));
    }
  }
}

void LoadFromIHex(const std::string &filename, SystemBus *system_bus) {
  std::ifstream hex;
  hex.open(filename);
  int line_no = 0;
  uint32_t page_addr = 0;
  while (!hex.eof()) {
    // Read record type.
    std::string line;
    std::getline(hex, line);
    if (line.empty())
      break;
    line_no++;
    CHECK(line[0] == ':') << "Invalid start code in line #" << line_no << ": '"
                          << line << "'";
    std::stringstream line_stream(line.substr(1));
    uint8_t sum = 0;

    uint8_t byte_count;
    CHECK(ReadHex(line_stream, &byte_count));
    CHECK_EQ(byte_count * 2, (uint16_t)line.size() - 11);
    sum += byte_count;
    uint8_t addr_hi, addr_low;
    CHECK(ReadHex(line_stream, &addr_hi));
    CHECK(ReadHex(line_stream, &addr_low));
    sum += addr_hi;
    sum += addr_low;
    uint16_t address = (addr_hi << 8) | addr_low;
    uint8_t record_type;
    CHECK(ReadHex(line_stream, &record_type));
    sum += record_type;
    if (record_type == 1) { // EOF
      return;
    } else if (record_type == 0) { // DATA
      while (byte_count--) {
        uint8_t data;
        CHECK(ReadHex(line_stream, &data));
        sum += data;
        uint32_t addr = page_addr + (address++);

        system_bus->WriteByte(addr, data);
      }
      uint8_t checksum;
      CHECK(ReadHex(line_stream, &checksum));
      checksum += sum;
      CHECK_EQ(checksum, 0);
    } else if (record_type == 4) { // Extended Linear Address
      uint8_t page_addr_hi;
      uint8_t page_addr_lo;
      CHECK(ReadHex(line_stream, &page_addr_lo));
      CHECK(ReadHex(line_stream, &page_addr_hi));
      page_addr = (page_addr_hi << 16) | (page_addr_lo << 8);
    }
  }
}

void LoadFromHex(const std::string &filename, SystemBus *system_bus) {
  boost::filesystem::path path(filename);
  if (path.extension().string() == ".hex") {
    LoadFromIHex(filename, system_bus);
  } else if (path.extension().string() == ".s28") {
    LoadFromS28(filename, system_bus);
  } else {
    CHECK(false) << "Unknown file format: " << path.extension();
  }
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