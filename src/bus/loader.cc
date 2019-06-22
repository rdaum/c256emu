#include "loader.h"

#include <experimental/filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::experimental::filesystem;

namespace {

uint8_t hstouc(std::string const& in) {
  static const std::string v{"0123456789ABCDEF"};

  return (uint8_t)v.find(toupper(in[0])) * 16 + v.find(toupper(in[1]));
}

bool ReadHex(std::istream& line_stream, uint8_t* h) {
  line_stream.width(2);
  std::string byte_count_str;
  line_stream >> std::setw(2) >> byte_count_str;
  if (byte_count_str.size() != 2)
    return false;
  *h = hstouc(byte_count_str);
  return true;
}

std::istream& safeGetline(std::istream& is, std::string& t) {
  t.clear();

  // The characters in the stream are read one-by-one using a std::streambuf.
  // That is faster than reading them one-by-one using the std::istream.
  // Code that uses streambuf this way must be guarded by a sentry object.
  // The sentry object performs various tasks,
  // such as thread synchronization and updating the stream state.

  std::istream::sentry se(is, true);
  std::streambuf* sb = is.rdbuf();

  for (;;) {
    int c = sb->sbumpc();
    switch (c) {
      case '\n':
        return is;
      case '\r':
        if (sb->sgetc() == '\n')
          sb->sbumpc();
        return is;
      case std::streambuf::traits_type::eof():
        // Also handle the case when the last line has no line ending
        if (t.empty())
          is.setstate(std::ios::eofbit);
        return is;
      default:
        t += (char)c;
    }
  }
}

}  // namespace

void LoadFromS28(const std::string& filename, SystemBus* system_bus) {
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

void LoadFromIHex(const std::string& filename, SystemBus* system_bus) {
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
    if (record_type == 1) {  // EOF
      return;
    } else if (record_type == 0) {  // DATA
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
    } else if (record_type == 4) {  // Extended Linear Address
      uint8_t page_addr_hi;
      uint8_t page_addr_lo;
      CHECK(ReadHex(line_stream, &page_addr_lo));
      CHECK(ReadHex(line_stream, &page_addr_hi));
      page_addr = (page_addr_hi << 16) | (page_addr_lo << 8);
      LOG(INFO) << " Page: " << page_addr;
    }
  }
}

void Loader::LoadFromHex(const std::string& filename) {
  fs::path path(filename);
  if (path.extension().string() == ".hex") {
    LoadFromIHex(filename, system_bus_);
  } else if (path.extension().string() == ".s28") {
    LoadFromS28(filename, system_bus_);
  } else {
    CHECK(false) << "Unknown file format: " << path.extension();
  }
}

void Loader::LoadFromBin(const std::string& filename, uint32_t base_address) {
  std::ifstream in_file(filename);
  CHECK(in_file.is_open());
  uint32_t address = base_address;
  char byte;
  while (in_file.get(byte)) {
    system_bus_->WriteByte(address, byte);
    address++;
  }
  LOG(INFO) << "Done @ " << address;
}

struct O65Header {
  uint8_t non_c64_marker[2];  // should be $01, $00
  uint8_t magic_marker[3];    // should be $6f, $36, $35
  uint8_t version;            // should be 0

  union {
    struct {
      uint8_t align : 2;
      uint8_t unused1 : 2;
      uint8_t cpu_2 : 4;
      uint8_t unused2 : 1;
      bool bsszero : 1;    // true if plz zero our the bss segment for this file
      bool chain : 1;      // true = another file follows this one
      bool simple : 1;     // true = simple file address
      bool is_object : 1;  // false if executable
      bool
          size : 1;  // determine if we use long or short mode header, true = 32
      bool reloc : 1;  // true = bytewise, 1 = page (256byte)wise reloc allowed
      bool is_65816 : 1;
    } m;
    uint16_t v;
  } mode;
};

struct O65Offsets {
  union {
    struct {
      uint32_t tbase;
      uint32_t tlen;
      uint32_t dbase;
      uint32_t dlen;
      uint32_t bbase;
      uint32_t blen;
      uint32_t zbase;
      uint32_t zlen;
      uint32_t stack;
    } long_mode;
    struct {
      uint16_t tbase;
      uint16_t tlen;
      uint16_t dbase;
      uint16_t dlen;
      uint16_t bbase;
      uint16_t blen;
      uint16_t zbase;
      uint16_t zlen;
      uint16_t stack;
    } word_mode;
  };
};

struct HeaderOption {
  uint8_t otype;
  std::string option_bytes;
};

void Reloc(uint32_t adjust, std::ifstream& in_file,
           std::vector<uint8_t>* seg,
           uint32_t reloc_offset,
           uint8_t type) {
  if (type == 0x80) { // WORD
    uint16_t* word =
        reinterpret_cast<uint16_t*>(&seg->data()[reloc_offset - 1]);
    LOG(INFO) << "WORD relocated: " << std::hex << *word << " to "
              << *word + adjust;
    *word += adjust;
  } else if (type == 0xa0) {  // SEG
    uint8_t* seg_byte = &seg->data()[reloc_offset - 1];
    uint8_t b1 = in_file.get();
    uint8_t b2 = in_file.get();
    //    uint16_t location = (b1<<8) | b2;  // not sure what we can do with
    //    this, most SEG references are just the bank?
    uint8_t rewrite = ((*seg_byte << 16) + adjust) >> 16;
    LOG(INFO) << "SEG relocated: " << std::hex << (int)*seg_byte << " to "
              << (int)rewrite;
    *seg_byte = rewrite;
  } else if (type == 0xc0) {  // SEGADDR
    uint8_t* seg_addr = &seg->data()[reloc_offset - 1];
    uint32_t value;
    memcpy(&value, seg_addr, 3);
    LOG(INFO) << "SEGADDR relocated: " << std::hex << value << " to "
              << value + adjust;
    value += adjust;
    memcpy(seg_addr, &value, 3);
  } else if (type == 0x20) { // LOW
    uint8_t *lo_addr = &seg->data()[reloc_offset - 1];

    LOG(INFO) << "LO relocated: " << std::hex << (int)*lo_addr << " to "
              << (int)*lo_addr + adjust;
    *lo_addr += adjust;
  } else {
    CHECK(false) << "Unhandled reloc type: " << std::hex << (int)type;
  }
}

void Loader::LoadFromO65(const std::string& filename, uint32_t reloc_address) {
  LOG(INFO) << "Relocating relative to " << std::hex << reloc_address;
  std::ifstream in_file(filename);
  CHECK(in_file.is_open());

  // Load and validate the header.
  O65Header header;
  in_file.read(reinterpret_cast<char*>(&header), 8);
  CHECK(!in_file.eof());
  CHECK(header.magic_marker[0] == 'o' && header.magic_marker[1] == '6' &&
        header.magic_marker[2] == '5');

  // Now load offsets, choosing the right size depending on what mode.
  O65Offsets o65_offsets;
  in_file.read(reinterpret_cast<char*>(&o65_offsets),
               header.mode.m.size ? 36 : 18);

  // Now header options
  std::vector<HeaderOption> header_options;

  // Now load header options.
  uint8_t olen = 0;
  while ((olen = in_file.get())) {
    HeaderOption opt;
    opt.otype = in_file.get();
    olen -= 2;  // take out len and type bytes from count
    while (olen--) {
      opt.option_bytes.push_back(in_file.get());
    }
    header_options.push_back(opt);
  };

  // Now load each segment, starting with the text segment.
  uint32_t tbase = header.mode.m.size ? o65_offsets.long_mode.tbase
                                      : o65_offsets.word_mode.tbase;
  uint32_t tlen = header.mode.m.size ? o65_offsets.long_mode.tlen
                                     : o65_offsets.word_mode.tlen;
  LOG(INFO) << "TBASE: " << std::hex << tbase << " TLEN: " << tlen;
  std::vector<uint8_t> t_seg(tlen);
  in_file.read(reinterpret_cast<char*>(t_seg.data()), tlen);
  CHECK(!in_file.eof());

  // And data
  uint32_t dbase = header.mode.m.size ? o65_offsets.long_mode.dbase
                                      : o65_offsets.word_mode.dbase;
  uint32_t dlen = header.mode.m.size ? o65_offsets.long_mode.dlen
                                     : o65_offsets.word_mode.dlen;
  LOG(INFO) << "DBASE: " << std::hex << dbase << " DLEN: " << dlen;

  std::vector<uint8_t> d_seg(dlen);
  in_file.read(reinterpret_cast<char*>(d_seg.data()), dlen);
  CHECK(!in_file.eof());

  uint32_t zpbase = header.mode.m.size ? o65_offsets.long_mode.zbase
                                       : o65_offsets.word_mode.zbase;
  LOG(INFO) << "ZPBASE: " << std::hex << zpbase;

  // Obtain the external (undefined) references list. This is meaningless for us
  // tho.
  uint32_t num_references;
  std::vector<std::string> external_references;
  in_file.read(reinterpret_cast<char*>(&num_references),
               header.mode.m.size ? 4 : 2);
  while (num_references--) {
    std::string ref;
    char c;
    while ((c = in_file.get())) {
      ref.push_back(c);
    }
    external_references.push_back(ref);
  }

  // Now the relocation tables, the fun part
  uint32_t reloc_offset = 0;
  while (true) {
    uint8_t offset = in_file.get();
    if (offset == 0)
      break;
    if (offset == 0xff) {
      reloc_offset += 254;
      continue;
    }
    reloc_offset += offset;
    uint8_t type_seg = in_file.get();
    uint8_t type = type_seg & 0xf0;
    uint8_t segment = (type_seg & 0x0f);
    LOG(INFO) << "RelocEntry: offset: " << std::hex << (int)reloc_offset
              << " relative_offset:" << (int)offset << " type: " << (int)type
              << " segment: " << (int)segment;
    if (segment == 2) {
      Reloc(reloc_address - tbase, in_file, &t_seg, reloc_offset, type);
    } else if (segment == 3) {
      Reloc(reloc_address - dbase, in_file, &d_seg, reloc_offset, type);
    } else if (segment == 5) {
      Reloc(zpbase, in_file, &t_seg, reloc_offset, type);
    } else {
      CHECK(false) << "Unhandled segment: " << std::hex << (int)segment;
    }
    // TODO more rewrites for other segment types, etc.
  };

  // Now load the segments into memory.
  int offset = 0;
  for (int i = 0; i < tlen; i++, offset++) {
    system_bus_->WriteByte(reloc_address + offset, t_seg[i]);
  }
  for (int i = 0; i < dlen; i++, offset++) {
    system_bus_->WriteByte(reloc_address + offset, d_seg[i]);
  }

  // Look for exported global symbols.
}
