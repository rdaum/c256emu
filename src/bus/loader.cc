#include "loader.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace {

uint8_t hstouc(std::string const &in) {
  static const std::string v{"0123456789ABCDEF"};

  return (uint8_t)v.find(toupper(in[0])) * 16 + v.find(toupper(in[1]));
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

    std::istream& safeGetline(std::istream& is, std::string& t)
    {
        t.clear();

        // The characters in the stream are read one-by-one using a std::streambuf.
        // That is faster than reading them one-by-one using the std::istream.
        // Code that uses streambuf this way must be guarded by a sentry object.
        // The sentry object performs various tasks,
        // such as thread synchronization and updating the stream state.

        std::istream::sentry se(is, true);
        std::streambuf* sb = is.rdbuf();

        for(;;) {
            int c = sb->sbumpc();
            switch (c) {
                case '\n':
                    return is;
                case '\r':
                    if(sb->sgetc() == '\n')
                        sb->sbumpc();
                    return is;
                case std::streambuf::traits_type::eof():
                    // Also handle the case when the last line has no line ending
                    if(t.empty())
                        is.setstate(std::ios::eofbit);
                    return is;
                default:
                    t += (char)c;
            }
        }
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
  std::ifstream hex(filename);
  CHECK(hex);
  int line_no = 0;
  uint32_t page_addr = 0;
  LOG(INFO) << "Loading: " << filename;
  while (!hex.eof()) {
    // Read record type.
    std::string line;
    safeGetline(hex, line);
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
      LOG(INFO) <<" Page: " << page_addr;
    }
  }
}

void LoadFromHex(const std::string &filename, SystemBus *system_bus) {
  fs::path path(filename);
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