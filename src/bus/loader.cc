#include "loader.h"

#include <experimental/filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

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

std::istream &safeGetline(std::istream &is, std::string &t) {
  t.clear();

  // The characters in the stream are read one-by-one using a std::streambuf.
  // That is faster than reading them one-by-one using the std::istream.
  // Code that uses streambuf this way must be guarded by a sentry object.
  // The sentry object performs various tasks,
  // such as thread synchronization and updating the stream state.

  std::istream::sentry se(is, true);
  std::streambuf *sb = is.rdbuf();

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

} // namespace

bool LoadFromS28(const std::string &filename, SystemBus *system_bus) {
  std::ifstream srec_stream;
  srec_stream.open(filename);
  while (!srec_stream.eof()) {
    std::string line;
    std::getline(srec_stream, line);
    if (line.empty())
      return true;
    CHECK_EQ(line[0], 'S');
    char rec_type = line[1];

    std::stringstream line_stream(line.substr(2));

    // 24 bit data load
    if (rec_type == '2') {
      uint8_t byte_count;

      if (!ReadHex(line_stream, &byte_count)) {
        LOG(ERROR) << "Bad format in " << filename
                   << " (could not read byte count)";
        return false;
      }
      if (byte_count * 2 != (uint16_t)line.size() - 5) {
        LOG(ERROR) << "Bad format in " << filename << " (invalid byte count)";
        return false;
      }
      uint8_t address[3];
      if (!ReadHex(line_stream, &address[0]) ||
          !ReadHex(line_stream, &address[1]) ||
          !ReadHex(line_stream, &address[2])) {
        LOG(ERROR) << "Bad format in " << filename
                   << " (could not read address)";
        return false;
      };
      byte_count -= 4;
      uint32_t addr = address[0] << 16 | address[1] << 8 | address[2];
      while (byte_count--) {
        uint8_t byte;
        if (!ReadHex(line_stream, &byte)) {
          LOG(ERROR) << "Bad format in " << filename
                     << " (could not read data byte)";
          return false;
        }

        system_bus->WriteByte(addr++, byte);
      }
      uint8_t checksum;
      if (!ReadHex(line_stream, &checksum)) {
        LOG(ERROR) << "Bad format in " << filename
                   << " (could not read checksum)";
        return false;
      }
    }
  }
  return true;
}

bool LoadFromIHex(const std::string &filename, SystemBus *system_bus) {
  std::ifstream hex(filename);
  if (!hex) {
    LOG(ERROR) << "Unable to open file: " << filename;
    return false;
  }
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
    if (line[0] != ':') {
      LOG(ERROR) << "Bad format in " << filename
                 << "(Invalid start code in line #" << line_no << ": '" << line
                 << "')";
      return false;
    }
    std::stringstream line_stream(line.substr(1));
    uint8_t sum = 0;

    uint8_t byte_count;
    if (!ReadHex(line_stream, &byte_count)) {
      LOG(ERROR) << " Bad format in " << filename
                 << " (could not read hex value)";
      return false;
    }
    if ((byte_count * 2 != (uint16_t)line.size() - 11)) {
      LOG(ERROR) << "Bad format in " << filename << " (bad byte count)";
      return false;
    }
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
      return true;
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
      if (!ReadHex(line_stream, &page_addr_lo) ||
          !ReadHex(line_stream, &page_addr_hi)) {
        LOG(ERROR) << "Bad format in " << filename
                   << " (could not read extended linear address)";
        return false;
      }
      page_addr = (page_addr_hi << 16) | (page_addr_lo << 8);
    }
  }
  return true;
}

bool Loader::LoadFromHex(const std::string &filename) {
  fs::path path(filename);
  if (path.extension().string() == ".hex") {
    return LoadFromIHex(filename, system_bus_);
  } else if (path.extension().string() == ".s28") {
    LoadFromS28(filename, system_bus_);
  } else {
    LOG(ERROR) << "Unknown file format: " << path.extension();
    return false;
  }
}

bool Loader::LoadFromBin(const std::string &filename, uint32_t base_address) {
  std::ifstream in_file(filename);
  if (!in_file.is_open()) {
    LOG(ERROR) << "Unable to open file: " << filename;
    return false;
  }
  uint32_t address = base_address;
  char byte;
  while (in_file.get(byte)) {
    system_bus_->WriteByte(address, byte);
    address++;
  }
  LOG(INFO) << "Done @ " << address;
  return true;
}

struct O65Header {
  uint8_t non_c64_marker[2]; // should be $01, $00
  uint8_t magic_marker[3];   // should be $6f, $36, $35
  uint8_t version;           // should be 0

  union {
    struct {
      uint8_t align : 2;
      uint8_t unused1 : 2;
      uint8_t cpu_2 : 4;
      uint8_t unused2 : 1;
      bool bsszero : 1;   // true if plz zero our the bss segment for this file
      bool chain : 1;     // true = another file follows this one
      bool simple : 1;    // true = simple file address
      bool is_object : 1; // false if executable
      bool size : 1; // determine if we use long or short mode header, true = 32
      bool reloc : 1; // true = bytewise, 1 = page (256byte)wise reloc allowed
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

enum RelocationSegment {
  UNDEFINED = 0,
  ABSOLUTE_VALUE = 1, // never used
  TEXT_SEGMENT = 2,
  DATA_SEGMENT = 3,
  BSS_SEGMENT = 4,
  ZP_SEGMENT = 5,
};

enum RelocationType {
  RELOC_WORD = 0x80,
  RELOC_HIGH = 0x40,
  RELOC_LOW = 0x20,
  RELOC_SEGADDR = 0xc0,
  RELOC_SEG = 0xa0,
};

uint32_t AdjustAddr(uint32_t cur_segment_base, uint32_t new_segment_base,
                    uint32_t addr) {
  addr -= cur_segment_base;
  addr += new_segment_base;
  return addr;
}

void DoReloc(uint32_t cur_segment_base, uint32_t new_segment_base,
             std::ifstream &in_file, std::vector<uint8_t> *seg,
             uint32_t reloc_offset, RelocationType type, bool verbose) {
  if (type == RELOC_WORD) {
    uint16_t *word =
        reinterpret_cast<uint16_t *>(&seg->data()[reloc_offset - 1]);
    uint32_t full_addr = *word + (cur_segment_base & 0x00ff0000);
    uint32_t adjust_full_addr =
        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
    uint32_t w = adjust_full_addr & 0x0000ffff;
    if (verbose)
      LOG(INFO) << "WORD relocated: " << std::hex << *word << " to " << w;
    *word = w;
    return;
  }

  if (type == RELOC_SEG) {
    uint8_t *seg_byte = &(*seg)[reloc_offset - 1];
    uint8_t b1 = in_file.get();
    uint8_t b2 = in_file.get();
    uint32_t full_addr = (*seg_byte) << 16 | (b1 << 8) | b2;
    uint32_t adjust_full_addr =
        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
    uint8_t new_seg = adjust_full_addr >> 16;
    if (verbose)
      LOG(INFO) << "SEG relocated: " << std::hex << (int)*seg_byte << " to "
                << (int)new_seg;
    *seg_byte = new_seg;
    return;
  }

  if (type == RELOC_SEGADDR) {
    uint8_t *seg_addr = &(*seg)[reloc_offset - 1];
    uint32_t value;
    memcpy(&value, seg_addr, 3);
    if (verbose)
      LOG(INFO) << "SEGADDR relocated: " << std::hex << value << " to "
                << AdjustAddr(cur_segment_base, new_segment_base, value);
    value = AdjustAddr(cur_segment_base, new_segment_base, value);
    memcpy(seg_addr, &value, 3);
    return;
  }

  if (type == RELOC_LOW) {
    uint8_t *lo_addr = &(*seg)[reloc_offset - 1];
    uint32_t full_addr = cur_segment_base + *lo_addr;
    uint32_t adjust_full_addr =
        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
    uint8_t new_lo = adjust_full_addr & 0x000000ff;
    if (verbose)
      LOG(INFO) << "LO relocated: " << std::hex << (int)*lo_addr << " to "
                << (int)new_lo;
    *lo_addr = new_lo;
    return;
  }

  if (type == RELOC_HIGH) {
    uint8_t *hi_addr = &(*seg)[reloc_offset - 1];
    uint8_t b1 = in_file.get();
    uint32_t full_addr = cur_segment_base + (*hi_addr << 8) | b1;
    uint32_t adjust_full_addr =
        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
    uint16_t new_hi = (adjust_full_addr & 0x0000ff00) >> 8;
    if (verbose)
      LOG(INFO) << "HI relocated: " << std::hex << (int)*hi_addr << " to "
                << (int)new_hi;
    *hi_addr += new_hi;
    return;
  }
  CHECK(false) << "Unhandled reloc type: " << std::hex << (int)type;
}

bool Loader::LoadFromO65(const std::string &filename, uint32_t reloc_address,
                         bool verbose) {
  LOG(INFO) << "Relocating '" << filename << "' relative to " << std::hex
            << reloc_address;
  std::ifstream in_file(filename);
  if (!in_file.is_open()) {
    LOG(ERROR) << "Bad file: " << filename << "; could not open";
    return false;
  }

  // Load and validate the header.
  O65Header header;
  in_file.read(reinterpret_cast<char *>(&header), 8);
  if (in_file.eof()) {
    LOG(ERROR) << "Unexpected EOF in o65";
    return false;
  }
  CHECK(header.magic_marker[0] == 'o' && header.magic_marker[1] == '6' &&
        header.magic_marker[2] == '5');

  // Now load offsets, choosing the right size depending on what mode.
  O65Offsets o65_offsets;
  in_file.read(reinterpret_cast<char *>(&o65_offsets),
               header.mode.m.size ? 36 : 18);

  // Now header options
  std::vector<HeaderOption> header_options;

  // Now load header options.
  uint8_t olen = 0;
  while ((olen = in_file.get())) {
    HeaderOption opt;
    opt.otype = in_file.get();
    olen -= 2; // take out len and type bytes from count
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
  if (verbose)
    LOG(INFO) << "TBASE: " << std::hex << tbase << " TLEN: " << tlen;

  // And data
  uint32_t dbase = header.mode.m.size ? o65_offsets.long_mode.dbase
                                      : o65_offsets.word_mode.dbase;
  uint32_t dlen = header.mode.m.size ? o65_offsets.long_mode.dlen
                                     : o65_offsets.word_mode.dlen;
  if (verbose)
    LOG(INFO) << "DBASE: " << std::hex << dbase << " DLEN: " << dlen;

  uint32_t bssbase = header.mode.m.size ? o65_offsets.long_mode.bbase
                                        : o65_offsets.word_mode.bbase;
  uint32_t bsslen = header.mode.m.size ? o65_offsets.long_mode.blen
                                       : o65_offsets.word_mode.blen;
  if (verbose)
    LOG(INFO) << "BSSBASE: " << std::hex << bssbase << " BSSLEN: " << bsslen;

  uint32_t zpbase = header.mode.m.size ? o65_offsets.long_mode.zbase
                                       : o65_offsets.word_mode.zbase;
  if (verbose)
    LOG(INFO) << "ZPBASE: " << std::hex << zpbase;

  uint32_t stack = header.mode.m.size ? o65_offsets.long_mode.stack
                                      : o65_offsets.word_mode.stack;
  if (verbose)
    LOG(INFO) << "STACK: " << std::hex << stack;

  std::vector<uint8_t> program(tlen + dlen);
  in_file.read(reinterpret_cast<char *>(program.data()), tlen);
  CHECK(!in_file.eof());

  // Obtain the external (undefined) references list. This is meaningless for us
  // tho.
  uint32_t num_references;
  std::vector<std::string> external_references;
  in_file.read(reinterpret_cast<char *>(&num_references),
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
      reloc_offset += 0xfe;
      continue;
    }
    reloc_offset += offset;
    uint8_t type_seg = in_file.get();
    RelocationType type = static_cast<RelocationType>(type_seg & 0xf0);
    RelocationSegment segment = static_cast<RelocationSegment>(type_seg & 0x0f);
    if (verbose)
      LOG(INFO) << "RelocEntry: offset: " << std::hex << (int)reloc_offset
                << " relative_offset:" << (int)offset << " type: " << (int)type
                << " segment: " << (int)segment;
    if (segment == TEXT_SEGMENT) { // text
      DoReloc(tbase, reloc_address, in_file, &program, reloc_offset, type,
              verbose);
    } else if (segment == DATA_SEGMENT) { // data
      DoReloc(dbase, reloc_address + tlen, in_file, &program, reloc_offset,
              type, verbose);
    } else if (segment == ZP_SEGMENT) { // zp
      DoReloc(0, zpbase, in_file, &program, reloc_offset, type, verbose);
    } else if (segment == BSS_SEGMENT) { // bss
      DoReloc(bssbase, reloc_address + tlen + dlen, in_file, &program,
              reloc_offset, type, verbose);
    } else {
      LOG(ERROR) << "Unhandled o65 segment: " << std::hex << (int)segment;
      return false;
    }
  };

  // Now load the segments into memory.
  int offset = 0;
  for (offset = 0; offset < tlen + dlen; offset++)
    system_bus_->WriteByte(reloc_address + offset, program[offset]);

  // BSS
  while (bsslen--) {
    system_bus_->WriteByte(reloc_address + (offset++), 0);
  }

  // Look for exported global symbols.

  return true;
}
