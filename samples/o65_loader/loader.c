#include "loader.h"

#include <stdbool.h>

#if KTRACE
extern void kprintf(const char* fmt, ...);
#endif

struct O65Header {
  uint8_t non_c64_marker[2];  // should be $01, $00
  uint8_t magic_marker[3];    // should be $6f, $36, $35
  uint8_t version;            // should be 0
  uint16_t mode;
};

struct O65OffsetsLong {
  uint32_t tbase;
  uint32_t tlen;
  uint32_t dbase;
  uint32_t dlen;
  uint32_t bbase;
  uint32_t blen;
  uint32_t zbase;
  uint32_t zlen;
  uint32_t stack;
};

struct O65OffsetsWord {
  uint16_t tbase;
  uint16_t tlen;
  uint16_t dbase;
  uint16_t dlen;
  uint16_t bbase;
  uint16_t blen;
  uint16_t zbase;
  uint16_t zlen;
  uint16_t stack;
};

enum RelocationSegment {
  UNDEFINED = 0,
  ABSOLUTE_VALUE = 1,  // never used
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

uint32_t AdjustAddr(uint32_t cur_segment_base,
                    uint32_t new_segment_base,
                    uint32_t addr) {
  addr -= cur_segment_base;
  addr += new_segment_base;
  return addr;
}

ProgramPtr Copy2(ProgramPtr source, ProgramPtr dest) {
  *dest++ = *source++;
  *dest++ = *source++;
  return source;
}

ProgramPtr Copy3(ProgramPtr source, ProgramPtr dest) {
  *dest++ = *source++;
  *dest++ = *source++;
  *dest++ = *source++;
  return source;
}

ProgramPtr Copy4(ProgramPtr source, ProgramPtr dest) {
  *dest++ = *source++;
  *dest++ = *source++;
  *dest++ = *source++;
  *dest++ = *source++;
  return source;
}

ProgramPtr LoadHeaderOptions(ProgramPtr program) {
  uint8_t olen;
  // Now throw away  header options
  olen = 0;
  while ((olen = *(program++))) {
    uint8_t otype = *(program++);
    olen -= 2;  // take out len and type bytes from count
    while (olen--) {
      (*program++);
    }
  };
  return program;
}

ProgramPtr LoadExternalReferences(ProgramPtr program, bool size) {
  uint32_t num_references;
  num_references = 0;
  if (size) {
    program = Copy4(program, (ProgramPtr)&num_references);
  } else {
    program = Copy2(program, (ProgramPtr)&num_references);
  }
#if KTRACE
  kprintf("%04lx references\n", num_references);
#endif
  // throw away references
  while (num_references--) {
    char c;
    while ((c = (*program++))) {
    }
  }
  return program;
}

ProgramPtr DoReloc(uint32_t cur_segment_base,
                   uint32_t new_segment_base,
                   ProgramPtr reloc_table_ptr,
                   ProgramPtr seg,
                   uint32_t reloc_offset,
                   enum RelocationType type) {
  if (type == RELOC_WORD) {
    uint16_t far* word = (far uint16_t*)(&seg[reloc_offset - 1]);
    uint32_t full_addr = *word + (cur_segment_base & 0x00ff0000);
    uint32_t adjust_full_addr =
        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
    uint32_t w = adjust_full_addr & 0x0000ffff;
#if KTRACE
    kprintf("RELOC_WORD %04x => %04x\n", *word, w);
#endif
    *word = w;
    return reloc_table_ptr;
  }

  if (type == RELOC_SEG) {
    ProgramPtr seg_byte = &(seg[reloc_offset - 1]);
    uint8_t b1 = *(reloc_table_ptr++);
    uint8_t b2 = *(reloc_table_ptr++);
    //    uint16_t location = (b1<<8) | b2;  // not sure what we can do with
    //    this, most SEG references are just the bank?
    uint32_t full_addr = (*seg_byte) << 16;
    uint32_t adjust_full_addr =
        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
    uint8_t new_seg = adjust_full_addr >> 16;
#if KTRACE
    kprintf("RELOC_SEG %02x => %02x\n", *seg_byte, new_seg);
#endif
    *seg_byte = new_seg;
    return reloc_table_ptr;
  }

  if (type == RELOC_SEGADDR) {
    ProgramPtr seg_addr = &(seg[reloc_offset - 1]);
    uint32_t value = 0;
    Copy3(seg_addr, (ProgramPtr)&value);
    value = AdjustAddr(cur_segment_base, new_segment_base, value);
#if KTRACE
    kprintf("RELOC_SEGADDR %04x => %04x\n", *seg_addr, value);
#endif
    Copy3((ProgramPtr)&value, seg_addr);
    return reloc_table_ptr;
  }

  if (type == RELOC_LOW) {
    ProgramPtr lo_addr = &(seg[reloc_offset - 1]);
    uint32_t full_addr = cur_segment_base + *lo_addr;
    uint32_t adjust_full_addr =
        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
    uint8_t new_lo = adjust_full_addr & 0x000000ff;
#if KTRACE
    kprintf("RELOC_LOW %02x => %04x\n", *lo_addr, new_lo);
#endif
    *lo_addr = new_lo;
    return reloc_table_ptr;
  }

  if (type == RELOC_HIGH) {
    ProgramPtr hi_addr = &(seg[reloc_offset - 1]);
    uint32_t full_addr = cur_segment_base + (*hi_addr << 8);
    uint32_t adjust_full_addr =
        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
    uint16_t new_hi = (adjust_full_addr & 0x0000ff00) >> 8;
    uint8_t b1 = *(reloc_table_ptr++);  // not sure what to do with this?
#if KTRACE
    kprintf("RELOC_HIGH %02x => %04x\n", *hi_addr, new_hi);
#endif
    *hi_addr += new_hi;
    return reloc_table_ptr;
  }

#if KTRACE
  kprintf("Unknown relocation type: %02x\n", type);
#endif

  return reloc_table_ptr;
}

ProgramPtr DoRelocations(ProgramPtr reloc_table_ptr,
                         uint32_t reloc_address,
                         ProgramPtr seg_base,
                         uint32_t tbase,
                         uint32_t tlen,
                         uint32_t dbase,
                         uint32_t dlen,
                         uint32_t bssbase,
                         uint32_t zpbase) {
  uint32_t reloc_offset = 0;
  uint8_t offset;
  uint8_t type_seg;
  enum RelocationType type;
  enum RelocationSegment segment;

  while (1) {
    offset = *(reloc_table_ptr++);
    if (offset == 0)
      break;
    if (offset == 0xff) {
      reloc_offset += 0xfe;
      continue;
    }
    reloc_offset += offset;
    type_seg = *(reloc_table_ptr++);
    type = (enum RelocationType)(type_seg & 0xf0);
    segment = (enum RelocationSegment)(type_seg & 0x0f);
#if KTRACE
    kprintf("Reloc type: %02x segment: %01x offset: %08x\n", type, segment,
            reloc_offset);
#endif
    if (segment == TEXT_SEGMENT) {  // text
      reloc_table_ptr = DoReloc(tbase, reloc_address, reloc_table_ptr, seg_base,
                                reloc_offset, type);
    } else if (segment == DATA_SEGMENT) {  // data
      reloc_table_ptr = DoReloc(dbase, reloc_address + tlen, reloc_table_ptr,
                                seg_base, reloc_offset, type);
    } else if (segment == ZP_SEGMENT) {  // zp
      reloc_table_ptr =
          DoReloc(0, zpbase, reloc_table_ptr, seg_base, reloc_offset, type);
    } else if (segment == BSS_SEGMENT) {  // bss
      reloc_table_ptr = DoReloc(bssbase, reloc_address + tlen + dlen,
                                reloc_table_ptr, seg_base, reloc_offset, type);
    }
    // TODO more rewrites for other segment types, etc.
  };

  return reloc_table_ptr;
}

ProgramPtr RelocO65(ProgramPtr program, uint8_t* o65_error) {
  struct O65Header far* header;
  uint32_t tbase, tlen, dbase, dlen, bssbase, bsslen, zpbase, zlen, stack;
  bool size;
  ProgramPtr seg_base, seg_end;
  uint32_t reloc_address;

#if KTRACE
  kprintf("Loading from %08lx...\n", program);
#endif

  // Load and validate the header.
  header = (struct O65Header far*)program;
  if (header->magic_marker[0] != 'o' || header->magic_marker[1] != '6' ||
      header->magic_marker[2] != '5') {
#if KTRACE
    kprintf("Invalid O65 file, missing magic marker\n");
#endif
    *o65_error = O65_INVALID_SIGNATURE;
    return;
  }
  program += 8;

#if KTRACE
  kprintf("Mode: %04x, offsets start %08lx...\n", header->mode, program);
#endif

  // Now load offsets, choosing the right size depending on what mode.
  size = header->mode & 0x2000 ? 1 : 0;
  if (size) {
    struct O65OffsetsLong far* offsets = (struct O65OffsetsLong far*)program;
    tbase = offsets->tbase;
    tlen = offsets->tlen;
    dbase = offsets->dbase;
    dlen = offsets->dlen;
    bssbase = offsets->bbase;
    bsslen = offsets->blen;
    zpbase = offsets->zbase;
    zlen = offsets->zlen;
    stack = offsets->stack;
    program += 36;
  } else {
    struct O65OffsetsWord far* offsets = (struct O65OffsetsWord far*)program;
    tbase = offsets->tbase;
    tlen = offsets->tlen;
    dbase = offsets->dbase;
    dlen = offsets->dlen;
    bssbase = offsets->bbase;
    bsslen = offsets->blen;
    zpbase = offsets->zbase;
    zlen = offsets->zlen;
    stack = offsets->stack;
    program += 18;
  }

#if KTRACE
  kprintf("size: %d tbase: %08lx tlen: %08lx dbase: %08lx dlen: %08lx\n", size,
          tbase, tlen, dbase, dlen);
#endif

  if ((uint32_t)program + tlen + dlen + dlen > 0x200000) {
#if KTRACE
    kprintf("Program size/relocation exceeds physical memory");
#endif
    *o65_error = O65_PROGRAM_SIZE;
    return;
  }
#if KTRACE
  kprintf("header_options_start: %08lx\n", program);
#endif

  // Now throw away  header options
  program = LoadHeaderOptions(program);

#if KTRACE
  kprintf("segments_start: %08lx\n", program);
#endif

  // Segments start here.
  seg_base = program;

  // Jump ahead to relocation tables, which are at the end of the segments.
  program += tlen + dlen;

  seg_end = program;

#if KTRACE
  kprintf("seg_base: %08lx seg_end: %08lx\n", seg_base, seg_end);
#endif

  // Obtain the external (undefined) references list. This is meaningless for us
  // tho.
  program = LoadExternalReferences(program, size);

#if KTRACE
  kprintf("relocations begin: %08lx\n", program);
#endif

  // Now the relocation tables, the fun part
  reloc_address = (uint32_t)seg_base;
  program = DoRelocations(program, reloc_address, seg_base, tbase, tlen, dbase,
                          dlen, bssbase, zpbase);

#if KTRACE
  kprintf("Done relocations. Clearing bss...\n");
#endif
  // Could look for exported symbols. Don't need to now.

  // Now zero out the BSS, which will wipe out relocation tables and symbols
  // BSS
  while (bsslen--) {
    *(seg_end++) = 0;
  }

#if KTRACE
  kprintf("Done.\n");
#endif
  *o65_error = O65_LOAD_OK;
  return seg_base;
}
