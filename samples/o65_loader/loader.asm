;:ts=8
R0	equ	1
R1	equ	5
R2	equ	9
R3	equ	13
;#include "loader.h"
;
;#include <stdbool.h>
;
;#if KTRACE
;extern void kprintf(const char* fmt, ...);
;#endif 
;
;struct O65Header {
;  uint8_t non_c64_marker[2];  // should be $01, $00
;  uint8_t magic_marker[3];    // should be $6f, $36, $35
;  uint8_t version;            // should be 0
;  uint16_t mode;
;};
;
;struct O65OffsetsLong {
;  uint32_t tbase;
;  uint32_t tlen;
;  uint32_t dbase;
;  uint32_t dlen;
;  uint32_t bbase;
;  uint32_t blen;
;  uint32_t zbase;
;  uint32_t zlen;
;  uint32_t stack;
;};
;
;struct O65OffsetsWord {
;  uint16_t tbase;
;  uint16_t tlen;
;  uint16_t dbase;
;  uint16_t dlen;
;  uint16_t bbase;
;  uint16_t blen;
;  uint16_t zbase;
;  uint16_t zlen;
;  uint16_t stack;
;};
;
;enum RelocationSegment {
;  UNDEFINED = 0,
;  ABSOLUTE_VALUE = 1,  // never used
;  TEXT_SEGMENT = 2,
;  DATA_SEGMENT = 3,
;  BSS_SEGMENT = 4,
;  ZP_SEGMENT = 5,
;};
;
;enum RelocationType {
;  RELOC_WORD = 0x80,
;  RELOC_HIGH = 0x40,
;  RELOC_LOW = 0x20,
;  RELOC_SEGADDR = 0xc0,
;  RELOC_SEG = 0xa0,
;};
;
;uint32_t AdjustAddr(uint32_t cur_segment_base,
;                    uint32_t new_segment_base,
;                    uint32_t addr) {
	code
	xdef	~~AdjustAddr
	func
~~AdjustAddr:
	longa	on
	longi	on
	tsc
	sec
	sbc	#L2
	tcs
	phd
	tcd
cur_segment_base_0	set	4
new_segment_base_0	set	8
addr_0	set	12
;  addr -= cur_segment_base;
	sec
	lda	<L2+addr_0
	sbc	<L2+cur_segment_base_0
	sta	<L2+addr_0
	lda	<L2+addr_0+2
	sbc	<L2+cur_segment_base_0+2
	sta	<L2+addr_0+2
;  addr += new_segment_base;
	clc
	lda	<L2+addr_0
	adc	<L2+new_segment_base_0
	sta	<L2+addr_0
	lda	<L2+addr_0+2
	adc	<L2+new_segment_base_0+2
	sta	<L2+addr_0+2
;  return addr;
	ldx	<L2+addr_0+2
	lda	<L2+addr_0
L4:
	tay
	lda	<L2+2
	sta	<L2+2+12
	lda	<L2+1
	sta	<L2+1+12
	pld
	tsc
	clc
	adc	#L2+12
	tcs
	tya
	rtl
;}
L2	equ	0
L3	equ	1
	ends
	efunc
;
;ProgramPtr Copy2(ProgramPtr source, ProgramPtr dest) {
	code
	xdef	~~Copy2
	func
~~Copy2:
	longa	on
	longi	on
	tsc
	sec
	sbc	#L5
	tcs
	phd
	tcd
source_0	set	4
dest_0	set	8
;  *dest++ = *source++;
	sep	#$20
	longa	off
	lda	[<L5+source_0]
	sta	[<L5+dest_0]
	rep	#$20
	longa	on
	inc	<L5+source_0
	bne	L7
	inc	<L5+source_0+2
L7:
	inc	<L5+dest_0
	bne	L8
	inc	<L5+dest_0+2
L8:
;  *dest++ = *source++;
	sep	#$20
	longa	off
	lda	[<L5+source_0]
	sta	[<L5+dest_0]
	rep	#$20
	longa	on
	inc	<L5+source_0
	bne	L9
	inc	<L5+source_0+2
L9:
	inc	<L5+dest_0
	bne	L10
	inc	<L5+dest_0+2
L10:
;  return source;
	ldx	<L5+source_0+2
	lda	<L5+source_0
L11:
	tay
	lda	<L5+2
	sta	<L5+2+8
	lda	<L5+1
	sta	<L5+1+8
	pld
	tsc
	clc
	adc	#L5+8
	tcs
	tya
	rtl
;}
L5	equ	0
L6	equ	1
	ends
	efunc
;
;ProgramPtr Copy3(ProgramPtr source, ProgramPtr dest) {
	code
	xdef	~~Copy3
	func
~~Copy3:
	longa	on
	longi	on
	tsc
	sec
	sbc	#L12
	tcs
	phd
	tcd
source_0	set	4
dest_0	set	8
;  *dest++ = *source++;
	sep	#$20
	longa	off
	lda	[<L12+source_0]
	sta	[<L12+dest_0]
	rep	#$20
	longa	on
	inc	<L12+source_0
	bne	L14
	inc	<L12+source_0+2
L14:
	inc	<L12+dest_0
	bne	L15
	inc	<L12+dest_0+2
L15:
;  *dest++ = *source++;
	sep	#$20
	longa	off
	lda	[<L12+source_0]
	sta	[<L12+dest_0]
	rep	#$20
	longa	on
	inc	<L12+source_0
	bne	L16
	inc	<L12+source_0+2
L16:
	inc	<L12+dest_0
	bne	L17
	inc	<L12+dest_0+2
L17:
;  *dest++ = *source++;
	sep	#$20
	longa	off
	lda	[<L12+source_0]
	sta	[<L12+dest_0]
	rep	#$20
	longa	on
	inc	<L12+source_0
	bne	L18
	inc	<L12+source_0+2
L18:
	inc	<L12+dest_0
	bne	L19
	inc	<L12+dest_0+2
L19:
;  return source;
	ldx	<L12+source_0+2
	lda	<L12+source_0
L20:
	tay
	lda	<L12+2
	sta	<L12+2+8
	lda	<L12+1
	sta	<L12+1+8
	pld
	tsc
	clc
	adc	#L12+8
	tcs
	tya
	rtl
;}
L12	equ	0
L13	equ	1
	ends
	efunc
;
;ProgramPtr Copy4(ProgramPtr source, ProgramPtr dest) {
	code
	xdef	~~Copy4
	func
~~Copy4:
	longa	on
	longi	on
	tsc
	sec
	sbc	#L21
	tcs
	phd
	tcd
source_0	set	4
dest_0	set	8
;  *dest++ = *source++;
	sep	#$20
	longa	off
	lda	[<L21+source_0]
	sta	[<L21+dest_0]
	rep	#$20
	longa	on
	inc	<L21+source_0
	bne	L23
	inc	<L21+source_0+2
L23:
	inc	<L21+dest_0
	bne	L24
	inc	<L21+dest_0+2
L24:
;  *dest++ = *source++;
	sep	#$20
	longa	off
	lda	[<L21+source_0]
	sta	[<L21+dest_0]
	rep	#$20
	longa	on
	inc	<L21+source_0
	bne	L25
	inc	<L21+source_0+2
L25:
	inc	<L21+dest_0
	bne	L26
	inc	<L21+dest_0+2
L26:
;  *dest++ = *source++;
	sep	#$20
	longa	off
	lda	[<L21+source_0]
	sta	[<L21+dest_0]
	rep	#$20
	longa	on
	inc	<L21+source_0
	bne	L27
	inc	<L21+source_0+2
L27:
	inc	<L21+dest_0
	bne	L28
	inc	<L21+dest_0+2
L28:
;  *dest++ = *source++;
	sep	#$20
	longa	off
	lda	[<L21+source_0]
	sta	[<L21+dest_0]
	rep	#$20
	longa	on
	inc	<L21+source_0
	bne	L29
	inc	<L21+source_0+2
L29:
	inc	<L21+dest_0
	bne	L30
	inc	<L21+dest_0+2
L30:
;  return source;
	ldx	<L21+source_0+2
	lda	<L21+source_0
L31:
	tay
	lda	<L21+2
	sta	<L21+2+8
	lda	<L21+1
	sta	<L21+1+8
	pld
	tsc
	clc
	adc	#L21+8
	tcs
	tya
	rtl
;}
L21	equ	0
L22	equ	1
	ends
	efunc
;
;ProgramPtr LoadHeaderOptions(ProgramPtr program) {
	code
	xdef	~~LoadHeaderOptions
	func
~~LoadHeaderOptions:
	longa	on
	longi	on
	tsc
	sec
	sbc	#L32
	tcs
	phd
	tcd
program_0	set	4
;  uint8_t olen;
;  // Now throw away  header options
;  olen = 0;
olen_1	set	0
	sep	#$20
	longa	off
	stz	<L33+olen_1
	rep	#$20
	longa	on
;  while ((olen = *(program++))) {
L10001:
	lda	<L32+program_0
	sta	<R0
	lda	<L32+program_0+2
	sta	<R0+2
	inc	<L32+program_0
	bne	L34
	inc	<L32+program_0+2
L34:
	sep	#$20
	longa	off
	lda	[<R0]
	sta	<L33+olen_1
	rep	#$20
	longa	on
	lda	<L33+olen_1
	and	#$ff
	bne	L35
	brl	L10002
L35:
;    uint8_t otype = *(program++);
;    olen -= 2;  // take out len and type bytes from count
otype_2	set	1
	sep	#$20
	longa	off
	lda	[<L32+program_0]
	sta	<L33+otype_2
	rep	#$20
	longa	on
	inc	<L32+program_0
	bne	L36
	inc	<L32+program_0+2
L36:
	lda	<L33+olen_1
	and	#$ff
	sta	<R0
	clc
	lda	#$fffe
	adc	<R0
	sta	<R1
	sep	#$20
	longa	off
	lda	<R1
	sta	<L33+olen_1
	rep	#$20
	longa	on
;    while (olen--) {
L10003:
	sep	#$20
	longa	off
	lda	<L33+olen_1
	sta	<R0
	rep	#$20
	longa	on
	sep	#$20
	longa	off
	dec	<L33+olen_1
	rep	#$20
	longa	on
	lda	<R0
	and	#$ff
	bne	L37
	brl	L10004
L37:
;      (*program++);
	inc	<L32+program_0
	bne	L38
	inc	<L32+program_0+2
L38:
;    }
	brl	L10003
L10004:
;  };
	brl	L10001
L10002:
;  return program;
	ldx	<L32+program_0+2
	lda	<L32+program_0
L39:
	tay
	lda	<L32+2
	sta	<L32+2+4
	lda	<L32+1
	sta	<L32+1+4
	pld
	tsc
	clc
	adc	#L32+4
	tcs
	tya
	rtl
;}
L32	equ	10
L33	equ	9
	ends
	efunc
;
;ProgramPtr LoadExternalReferences(ProgramPtr program, bool size) {
	code
	xdef	~~LoadExternalReferences
	func
~~LoadExternalReferences:
	longa	on
	longi	on
	tsc
	sec
	sbc	#L40
	tcs
	phd
	tcd
program_0	set	4
size_0	set	8
;  uint32_t num_references;
;  num_references = 0;
num_references_1	set	0
	stz	<L41+num_references_1
	stz	<L41+num_references_1+2
;  if (size) {
	lda	<L40+size_0
	bne	L42
	brl	L10005
L42:
;    program = Copy4(program, (ProgramPtr)&num_references);
	pea	#0
	clc
	tdc
	adc	#<L41+num_references_1
	pha
	pei	<L40+program_0+2
	pei	<L40+program_0
	jsl	~~Copy4
	sta	<L40+program_0
	stx	<L40+program_0+2
;  } else {
	brl	L10006
L10005:
;    program = Copy2(program, (ProgramPtr)&num_references);
	pea	#0
	clc
	tdc
	adc	#<L41+num_references_1
	pha
	pei	<L40+program_0+2
	pei	<L40+program_0
	jsl	~~Copy2
	sta	<L40+program_0
	stx	<L40+program_0+2
;  }
L10006:
;#if KTRACE
;  kprintf("%04lx references\n", num_references);
;#endif  
;  // throw away references
;  while (num_references--) {
L10007:
	lda	<L41+num_references_1
	sta	<R0
	lda	<L41+num_references_1+2
	sta	<R0+2
	lda	<L41+num_references_1
	bne	L43
	dec	<L41+num_references_1+2
L43:
	dec	<L41+num_references_1
	lda	<R0
	ora	<R0+2
	bne	L44
	brl	L10008
L44:
;    char c;
;    while ((c = (*program++))) {
c_2	set	4
L10009:
	lda	<L40+program_0
	sta	<R0
	lda	<L40+program_0+2
	sta	<R0+2
	inc	<L40+program_0
	bne	L45
	inc	<L40+program_0+2
L45:
	sep	#$20
	longa	off
	lda	[<R0]
	sta	<L41+c_2
	rep	#$20
	longa	on
	lda	<L41+c_2
	and	#$ff
	bne	L46
	brl	L10010
L46:
;    }
	brl	L10009
L10010:
;  }
	brl	L10007
L10008:
;  return program;
	ldx	<L40+program_0+2
	lda	<L40+program_0
L47:
	tay
	lda	<L40+2
	sta	<L40+2+6
	lda	<L40+1
	sta	<L40+1+6
	pld
	tsc
	clc
	adc	#L40+6
	tcs
	tya
	rtl
;}
L40	equ	9
L41	equ	5
	ends
	efunc
;
;ProgramPtr DoReloc(uint32_t cur_segment_base,
;                   uint32_t new_segment_base,
;                   ProgramPtr reloc_table_ptr,
;                   ProgramPtr seg,
;                   uint32_t reloc_offset,
;                   enum RelocationType type) {
	code
	xdef	~~DoReloc
	func
~~DoReloc:
	longa	on
	longi	on
	tsc
	sec
	sbc	#L48
	tcs
	phd
	tcd
cur_segment_base_0	set	4
new_segment_base_0	set	8
reloc_table_ptr_0	set	12
seg_0	set	16
reloc_offset_0	set	20
type_0	set	24
;  if (type == RELOC_WORD) {
	lda	<L48+type_0
	cmp	#<$80
	beq	L50
	brl	L10011
L50:
;    uint16_t far* word = (far uint16_t*)(&seg[reloc_offset - 1]);
;    uint32_t full_addr = *word + (cur_segment_base & 0x00ff0000);
;    uint32_t adjust_full_addr =
;        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
;    uint32_t w = adjust_full_addr & 0x0000ffff;
;#if KTRACE    
;    kprintf("RELOC_WORD %04x => %04x\n", *word, w);
;#endif    
;    *word = w;
word_2	set	0
full_addr_2	set	4
adjust_full_addr_2	set	8
w_2	set	12
	clc
	lda	#$ffff
	adc	<L48+reloc_offset_0
	sta	<R0
	lda	#$ffff
	adc	<L48+reloc_offset_0+2
	sta	<R0+2
	clc
	lda	<L48+seg_0
	adc	<R0
	sta	<L49+word_2
	lda	<L48+seg_0+2
	adc	<R0+2
	sta	<L49+word_2+2
	stz	<R1
	lda	<L48+cur_segment_base_0+2
	and	#^$ff0000
	sta	<R1+2
	lda	[<L49+word_2]
	sta	<R2
	stz	<R2+2
	clc
	lda	<R2
	adc	<R1
	sta	<L49+full_addr_2
	lda	<R2+2
	adc	<R1+2
	sta	<L49+full_addr_2+2
	pei	<L49+full_addr_2+2
	pei	<L49+full_addr_2
	pei	<L48+new_segment_base_0+2
	pei	<L48+new_segment_base_0
	pei	<L48+cur_segment_base_0+2
	pei	<L48+cur_segment_base_0
	jsl	~~AdjustAddr
	sta	<L49+adjust_full_addr_2
	stx	<L49+adjust_full_addr_2+2
	lda	<L49+adjust_full_addr_2
	sta	<L49+w_2
	stz	<L49+w_2+2
	lda	<L49+w_2
	sta	[<L49+word_2]
;    return reloc_table_ptr;
	ldx	<L48+reloc_table_ptr_0+2
	lda	<L48+reloc_table_ptr_0
L51:
	tay
	lda	<L48+2
	sta	<L48+2+22
	lda	<L48+1
	sta	<L48+1+22
	pld
	tsc
	clc
	adc	#L48+22
	tcs
	tya
	rtl
;  }
;
;  if (type == RELOC_SEG) {
L10011:
	lda	<L48+type_0
	cmp	#<$a0
	beq	L52
	brl	L10012
L52:
;    ProgramPtr seg_byte = &(seg[reloc_offset - 1]);
;    uint8_t b1 = *(reloc_table_ptr++);
;    uint8_t b2 = *(reloc_table_ptr++);
;    //    uint16_t location = (b1<<8) | b2;  // not sure what we can do with
;    //    this, most SEG references are just the bank?
;    uint32_t full_addr = (*seg_byte) << 16;
;    uint32_t adjust_full_addr =
;        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
;    uint8_t new_seg = adjust_full_addr >> 16;
;#if KTRACE    
;    kprintf("RELOC_SEG %02x => %02x\n", *seg_byte, new_seg);
;#endif    
;    *seg_byte = new_seg;
seg_byte_3	set	0
b1_3	set	4
b2_3	set	5
full_addr_3	set	6
adjust_full_addr_3	set	10
new_seg_3	set	14
	clc
	lda	#$ffff
	adc	<L48+reloc_offset_0
	sta	<R0
	lda	#$ffff
	adc	<L48+reloc_offset_0+2
	sta	<R0+2
	clc
	lda	<L48+seg_0
	adc	<R0
	sta	<L49+seg_byte_3
	lda	<L48+seg_0+2
	adc	<R0+2
	sta	<L49+seg_byte_3+2
	lda	<L48+reloc_table_ptr_0
	sta	<R1
	lda	<L48+reloc_table_ptr_0+2
	sta	<R1+2
	inc	<L48+reloc_table_ptr_0
	bne	L53
	inc	<L48+reloc_table_ptr_0+2
L53:
	sep	#$20
	longa	off
	lda	[<R1]
	sta	<L49+b1_3
	rep	#$20
	longa	on
	lda	<L48+reloc_table_ptr_0
	sta	<R1
	lda	<L48+reloc_table_ptr_0+2
	sta	<R1+2
	inc	<L48+reloc_table_ptr_0
	bne	L54
	inc	<L48+reloc_table_ptr_0+2
L54:
	sep	#$20
	longa	off
	lda	[<R1]
	sta	<L49+b2_3
	rep	#$20
	longa	on
	sep	#$20
	longa	off
	lda	[<L49+seg_byte_3]
	rep	#$20
	longa	on
	and	#$ff
	ldx	#<$10
	xref	~~~asl
	jsl	~~~asl
	sta	<R1
	ldy	#$0
	lda	<R1
	bpl	L55
	dey
L55:
	sta	<L49+full_addr_3
	sty	<L49+full_addr_3+2
	pei	<L49+full_addr_3+2
	pei	<L49+full_addr_3
	pei	<L48+new_segment_base_0+2
	pei	<L48+new_segment_base_0
	pei	<L48+cur_segment_base_0+2
	pei	<L48+cur_segment_base_0
	jsl	~~AdjustAddr
	sta	<L49+adjust_full_addr_3
	stx	<L49+adjust_full_addr_3+2
	pei	<L49+adjust_full_addr_3+2
	pei	<L49+adjust_full_addr_3
	lda	#$10
	xref	~~~llsr
	jsl	~~~llsr
	sta	<R1
	stx	<R1+2
	sep	#$20
	longa	off
	lda	<R1
	sta	<L49+new_seg_3
	rep	#$20
	longa	on
	sep	#$20
	longa	off
	lda	<L49+new_seg_3
	sta	[<L49+seg_byte_3]
	rep	#$20
	longa	on
;    return reloc_table_ptr;
	ldx	<L48+reloc_table_ptr_0+2
	lda	<L48+reloc_table_ptr_0
	brl	L51
;  }
;
;  if (type == RELOC_SEGADDR) {
L10012:
	lda	<L48+type_0
	cmp	#<$c0
	beq	L56
	brl	L10013
L56:
;    ProgramPtr seg_addr = &(seg[reloc_offset - 1]);
;    uint32_t value = 0;
;    Copy3(seg_addr, (ProgramPtr)&value);
seg_addr_4	set	0
value_4	set	4
	clc
	lda	#$ffff
	adc	<L48+reloc_offset_0
	sta	<R0
	lda	#$ffff
	adc	<L48+reloc_offset_0+2
	sta	<R0+2
	clc
	lda	<L48+seg_0
	adc	<R0
	sta	<L49+seg_addr_4
	lda	<L48+seg_0+2
	adc	<R0+2
	sta	<L49+seg_addr_4+2
	stz	<L49+value_4
	stz	<L49+value_4+2
	pea	#0
	clc
	tdc
	adc	#<L49+value_4
	pha
	pei	<L49+seg_addr_4+2
	pei	<L49+seg_addr_4
	jsl	~~Copy3
;    value = AdjustAddr(cur_segment_base, new_segment_base, value);
	pei	<L49+value_4+2
	pei	<L49+value_4
	pei	<L48+new_segment_base_0+2
	pei	<L48+new_segment_base_0
	pei	<L48+cur_segment_base_0+2
	pei	<L48+cur_segment_base_0
	jsl	~~AdjustAddr
	sta	<L49+value_4
	stx	<L49+value_4+2
;#if KTRACE    
;    kprintf("RELOC_SEGADDR %04x => %04x\n", *seg_addr, value);
;#endif    
;    Copy3((ProgramPtr)&value, seg_addr);
	pei	<L49+seg_addr_4+2
	pei	<L49+seg_addr_4
	pea	#0
	clc
	tdc
	adc	#<L49+value_4
	pha
	jsl	~~Copy3
;    return reloc_table_ptr;
	ldx	<L48+reloc_table_ptr_0+2
	lda	<L48+reloc_table_ptr_0
	brl	L51
;  }
;
;  if (type == RELOC_LOW) {
L10013:
	lda	<L48+type_0
	cmp	#<$20
	beq	L57
	brl	L10014
L57:
;    ProgramPtr lo_addr = &(seg[reloc_offset - 1]);
;    uint32_t full_addr = cur_segment_base + *lo_addr;
;    uint32_t adjust_full_addr =
;        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
;    uint8_t new_lo = adjust_full_addr & 0x000000ff;
;#if KTRACE    
;    kprintf("RELOC_LOW %02x => %04x\n", *lo_addr, new_lo);
;#endif    
;    *lo_addr = new_lo;
lo_addr_5	set	0
full_addr_5	set	4
adjust_full_addr_5	set	8
new_lo_5	set	12
	clc
	lda	#$ffff
	adc	<L48+reloc_offset_0
	sta	<R0
	lda	#$ffff
	adc	<L48+reloc_offset_0+2
	sta	<R0+2
	clc
	lda	<L48+seg_0
	adc	<R0
	sta	<L49+lo_addr_5
	lda	<L48+seg_0+2
	adc	<R0+2
	sta	<L49+lo_addr_5+2
	sep	#$20
	longa	off
	lda	[<L49+lo_addr_5]
	rep	#$20
	longa	on
	and	#$ff
	sta	<R1
	stz	<R1+2
	clc
	lda	<R1
	adc	<L48+cur_segment_base_0
	sta	<L49+full_addr_5
	lda	<R1+2
	adc	<L48+cur_segment_base_0+2
	sta	<L49+full_addr_5+2
	pei	<L49+full_addr_5+2
	pei	<L49+full_addr_5
	pei	<L48+new_segment_base_0+2
	pei	<L48+new_segment_base_0
	pei	<L48+cur_segment_base_0+2
	pei	<L48+cur_segment_base_0
	jsl	~~AdjustAddr
	sta	<L49+adjust_full_addr_5
	stx	<L49+adjust_full_addr_5+2
	lda	<L49+adjust_full_addr_5
	and	#<$ff
	sta	<R1
	stz	<R1+2
	sep	#$20
	longa	off
	lda	<R1
	sta	<L49+new_lo_5
	rep	#$20
	longa	on
	sep	#$20
	longa	off
	lda	<L49+new_lo_5
	sta	[<L49+lo_addr_5]
	rep	#$20
	longa	on
;    return reloc_table_ptr;
	ldx	<L48+reloc_table_ptr_0+2
	lda	<L48+reloc_table_ptr_0
	brl	L51
;  }
;
;  if (type == RELOC_HIGH) {
L10014:
	lda	<L48+type_0
	cmp	#<$40
	beq	L58
	brl	L10015
L58:
;    ProgramPtr hi_addr = &(seg[reloc_offset - 1]);
;    uint32_t full_addr = cur_segment_base + (*hi_addr << 8);
;    uint32_t adjust_full_addr =
;        AdjustAddr(cur_segment_base, new_segment_base, full_addr);
;    uint16_t new_hi = (adjust_full_addr & 0x0000ff00) >> 8;
;    uint8_t b1 = *(reloc_table_ptr++);  // not sure what to do with this?
;#if KTRACE    
;    kprintf("RELOC_HIGH %02x => %04x\n", *hi_addr, new_hi);
;#endif    
;    *hi_addr += new_hi;
hi_addr_6	set	0
full_addr_6	set	4
adjust_full_addr_6	set	8
new_hi_6	set	12
b1_6	set	14
	clc
	lda	#$ffff
	adc	<L48+reloc_offset_0
	sta	<R0
	lda	#$ffff
	adc	<L48+reloc_offset_0+2
	sta	<R0+2
	clc
	lda	<L48+seg_0
	adc	<R0
	sta	<L49+hi_addr_6
	lda	<L48+seg_0+2
	adc	<R0+2
	sta	<L49+hi_addr_6+2
	sep	#$20
	longa	off
	lda	[<L49+hi_addr_6]
	rep	#$20
	longa	on
	and	#$ff
	sta	<R2
	lda	<R2
	xba
	and	#$ff00
	sta	<R1
	ldy	#$0
	lda	<R1
	bpl	L59
	dey
L59:
	sta	<R1
	sty	<R1+2
	clc
	lda	<R1
	adc	<L48+cur_segment_base_0
	sta	<L49+full_addr_6
	lda	<R1+2
	adc	<L48+cur_segment_base_0+2
	sta	<L49+full_addr_6+2
	pei	<L49+full_addr_6+2
	pei	<L49+full_addr_6
	pei	<L48+new_segment_base_0+2
	pei	<L48+new_segment_base_0
	pei	<L48+cur_segment_base_0+2
	pei	<L48+cur_segment_base_0
	jsl	~~AdjustAddr
	sta	<L49+adjust_full_addr_6
	stx	<L49+adjust_full_addr_6+2
	lda	<L49+adjust_full_addr_6
	and	#<$ff00
	sta	<R2
	stz	<R2+2
	pei	<R2+2
	pei	<R2
	lda	#$8
	xref	~~~llsr
	jsl	~~~llsr
	sta	<R1
	stx	<R1+2
	lda	<R1
	sta	<L49+new_hi_6
	sep	#$20
	longa	off
	lda	[<L48+reloc_table_ptr_0]
	sta	<L49+b1_6
	rep	#$20
	longa	on
	inc	<L48+reloc_table_ptr_0
	bne	L60
	inc	<L48+reloc_table_ptr_0+2
L60:
	sep	#$20
	longa	off
	lda	[<L49+hi_addr_6]
	rep	#$20
	longa	on
	and	#$ff
	sta	<R0
	clc
	lda	<R0
	adc	<L49+new_hi_6
	sta	<R1
	sep	#$20
	longa	off
	lda	<R1
	sta	[<L49+hi_addr_6]
	rep	#$20
	longa	on
;    return reloc_table_ptr;
	ldx	<L48+reloc_table_ptr_0+2
	lda	<L48+reloc_table_ptr_0
	brl	L51
;  }
;
;#if KTRACE  
;  kprintf("Unknown relocation type: %02x\n", type);
;#endif
;
;  return reloc_table_ptr;
L10015:
	ldx	<L48+reloc_table_ptr_0+2
	lda	<L48+reloc_table_ptr_0
	brl	L51
;}
L48	equ	28
L49	equ	13
	ends
	efunc
;
;ProgramPtr DoRelocations(ProgramPtr reloc_table_ptr,
;                         uint32_t reloc_address,
;                         ProgramPtr seg_base,
;                         uint32_t tbase,
;                         uint32_t tlen,
;                         uint32_t dbase,
;                         uint32_t dlen,
;                         uint32_t bssbase,
;                         uint32_t zpbase) {
	code
	xdef	~~DoRelocations
	func
~~DoRelocations:
	longa	on
	longi	on
	tsc
	sec
	sbc	#L61
	tcs
	phd
	tcd
reloc_table_ptr_0	set	4
reloc_address_0	set	8
seg_base_0	set	12
tbase_0	set	16
tlen_0	set	20
dbase_0	set	24
dlen_0	set	28
bssbase_0	set	32
zpbase_0	set	36
;  uint32_t reloc_offset = 0;
;  uint8_t offset;
;  uint8_t type_seg;
;  enum RelocationType type;
;  enum RelocationSegment segment;
;
;  while (1) {
reloc_offset_1	set	0
offset_1	set	4
type_seg_1	set	5
type_1	set	6
segment_1	set	8
	stz	<L62+reloc_offset_1
	stz	<L62+reloc_offset_1+2
L10016:
;    offset = *(reloc_table_ptr++);
	sep	#$20
	longa	off
	lda	[<L61+reloc_table_ptr_0]
	sta	<L62+offset_1
	rep	#$20
	longa	on
	inc	<L61+reloc_table_ptr_0
	bne	L63
	inc	<L61+reloc_table_ptr_0+2
L63:
;    if (offset == 0)
;      break;
	lda	<L62+offset_1
	and	#$ff
	bne	L64
	brl	L10017
L64:
;    if (offset == 0xff) {
	sep	#$20
	longa	off
	lda	<L62+offset_1
	cmp	#<$ff
	rep	#$20
	longa	on
	beq	L65
	brl	L10018
L65:
;      reloc_offset += 0xfe;
	clc
	lda	#$fe
	adc	<L62+reloc_offset_1
	sta	<L62+reloc_offset_1
	bcc	L66
	inc	<L62+reloc_offset_1+2
L66:
;      continue;
	brl	L10016
;    }
;    reloc_offset += offset;
L10018:
	lda	<L62+offset_1
	and	#$ff
	sta	<R0
	stz	<R0+2
	clc
	lda	<R0
	adc	<L62+reloc_offset_1
	sta	<L62+reloc_offset_1
	lda	<R0+2
	adc	<L62+reloc_offset_1+2
	sta	<L62+reloc_offset_1+2
;    type_seg = *(reloc_table_ptr++);
	sep	#$20
	longa	off
	lda	[<L61+reloc_table_ptr_0]
	sta	<L62+type_seg_1
	rep	#$20
	longa	on
	inc	<L61+reloc_table_ptr_0
	bne	L67
	inc	<L61+reloc_table_ptr_0+2
L67:
;    type = (enum RelocationType)(type_seg & 0xf0);
	lda	<L62+type_seg_1
	and	#<$f0
	sta	<L62+type_1
;    segment = (enum RelocationSegment)(type_seg & 0x0f);
	lda	<L62+type_seg_1
	and	#<$f
	sta	<L62+segment_1
;#if KTRACE    
;    kprintf("Reloc type: %02x segment: %01x offset: %08x\n", type, segment,
;            reloc_offset);
;#endif            
;    if (segment == TEXT_SEGMENT) {  // text
	lda	<L62+segment_1
	cmp	#<$2
	beq	L68
	brl	L10019
L68:
;      reloc_table_ptr = DoReloc(tbase, reloc_address, reloc_table_ptr, seg_base,
;                                reloc_offset, type);
	pei	<L62+type_1
	pei	<L62+reloc_offset_1+2
	pei	<L62+reloc_offset_1
	pei	<L61+seg_base_0+2
	pei	<L61+seg_base_0
	pei	<L61+reloc_table_ptr_0+2
	pei	<L61+reloc_table_ptr_0
	pei	<L61+reloc_address_0+2
	pei	<L61+reloc_address_0
	pei	<L61+tbase_0+2
	pei	<L61+tbase_0
	jsl	~~DoReloc
	sta	<L61+reloc_table_ptr_0
	stx	<L61+reloc_table_ptr_0+2
;    } else if (segment == DATA_SEGMENT) {  // data
	brl	L10020
L10019:
	lda	<L62+segment_1
	cmp	#<$3
	beq	L69
	brl	L10021
L69:
;      reloc_table_ptr = DoReloc(dbase, reloc_address + tlen, reloc_table_ptr,
;                                seg_base, reloc_offset, type);
	pei	<L62+type_1
	pei	<L62+reloc_offset_1+2
	pei	<L62+reloc_offset_1
	pei	<L61+seg_base_0+2
	pei	<L61+seg_base_0
	pei	<L61+reloc_table_ptr_0+2
	pei	<L61+reloc_table_ptr_0
	clc
	lda	<L61+reloc_address_0
	adc	<L61+tlen_0
	sta	<R0
	lda	<L61+reloc_address_0+2
	adc	<L61+tlen_0+2
	sta	<R0+2
	pei	<R0+2
	pei	<R0
	pei	<L61+dbase_0+2
	pei	<L61+dbase_0
	jsl	~~DoReloc
	sta	<L61+reloc_table_ptr_0
	stx	<L61+reloc_table_ptr_0+2
;    } else if (segment == ZP_SEGMENT) {  // zp
	brl	L10022
L10021:
	lda	<L62+segment_1
	cmp	#<$5
	beq	L70
	brl	L10023
L70:
;      reloc_table_ptr =
;          DoReloc(0, zpbase, reloc_table_ptr, seg_base, reloc_offset, type);
	pei	<L62+type_1
	pei	<L62+reloc_offset_1+2
	pei	<L62+reloc_offset_1
	pei	<L61+seg_base_0+2
	pei	<L61+seg_base_0
	pei	<L61+reloc_table_ptr_0+2
	pei	<L61+reloc_table_ptr_0
	pei	<L61+zpbase_0+2
	pei	<L61+zpbase_0
	pea	#^$0
	pea	#<$0
	jsl	~~DoReloc
	sta	<L61+reloc_table_ptr_0
	stx	<L61+reloc_table_ptr_0+2
;    } else if (segment == BSS_SEGMENT) {  // bss
	brl	L10024
L10023:
	lda	<L62+segment_1
	cmp	#<$4
	beq	L71
	brl	L10025
L71:
;      reloc_table_ptr = DoReloc(bssbase, reloc_address + tlen + dlen,
;                                reloc_table_ptr, seg_base, reloc_offset, type);
	pei	<L62+type_1
	pei	<L62+reloc_offset_1+2
	pei	<L62+reloc_offset_1
	pei	<L61+seg_base_0+2
	pei	<L61+seg_base_0
	pei	<L61+reloc_table_ptr_0+2
	pei	<L61+reloc_table_ptr_0
	clc
	lda	<L61+reloc_address_0
	adc	<L61+tlen_0
	sta	<R0
	lda	<L61+reloc_address_0+2
	adc	<L61+tlen_0+2
	sta	<R0+2
	clc
	lda	<R0
	adc	<L61+dlen_0
	sta	<R1
	lda	<R0+2
	adc	<L61+dlen_0+2
	sta	<R1+2
	pei	<R1+2
	pei	<R1
	pei	<L61+bssbase_0+2
	pei	<L61+bssbase_0
	jsl	~~DoReloc
	sta	<L61+reloc_table_ptr_0
	stx	<L61+reloc_table_ptr_0+2
;    }
;    // TODO more rewrites for other segment types, etc.
;  };
L10025:
L10024:
L10022:
L10020:
	brl	L10016
L10017:
;
;  return reloc_table_ptr;
	ldx	<L61+reloc_table_ptr_0+2
	lda	<L61+reloc_table_ptr_0
L72:
	tay
	lda	<L61+2
	sta	<L61+2+36
	lda	<L61+1
	sta	<L61+1+36
	pld
	tsc
	clc
	adc	#L61+36
	tcs
	tya
	rtl
;}
L61	equ	18
L62	equ	9
	ends
	efunc
;
;ProgramPtr RelocO65(ProgramPtr program, uint8_t* o65_error) {
	code
	xdef	~~RelocO65
	func
~~RelocO65:
	longa	on
	longi	on
	tsc
	sec
	sbc	#L73
	tcs
	phd
	tcd
program_0	set	4
o65_error_0	set	8
;  struct O65Header far* header;
;  uint32_t tbase, tlen, dbase, dlen, bssbase, bsslen, zpbase, zlen, stack;
;  bool size;
;  ProgramPtr seg_base, seg_end;
;  uint32_t reloc_address;
;
;#if KTRACE
;  kprintf("Loading from %08lx...\n", program);
;#endif  
;
;  // Load and validate the header.
;  header = (struct O65Header far*)program;
header_1	set	0
tbase_1	set	4
tlen_1	set	8
dbase_1	set	12
dlen_1	set	16
bssbase_1	set	20
bsslen_1	set	24
zpbase_1	set	28
zlen_1	set	32
stack_1	set	36
size_1	set	40
seg_base_1	set	42
seg_end_1	set	46
reloc_address_1	set	50
	lda	<L73+program_0
	sta	<L74+header_1
	lda	<L73+program_0+2
	sta	<L74+header_1+2
;  if (header->magic_marker[0] != 'o' || header->magic_marker[1] != '6' ||
;      header->magic_marker[2] != '5') {
	sep	#$20
	longa	off
	ldy	#$2
	lda	[<L74+header_1],Y
	cmp	#<$6f
	rep	#$20
	longa	on
	beq	L76
	brl	L75
L76:
	sep	#$20
	longa	off
	ldy	#$3
	lda	[<L74+header_1],Y
	cmp	#<$36
	rep	#$20
	longa	on
	beq	L77
	brl	L75
L77:
	sep	#$20
	longa	off
	ldy	#$4
	lda	[<L74+header_1],Y
	cmp	#<$35
	rep	#$20
	longa	on
	bne	L78
	brl	L10026
L78:
L75:
;#if KTRACE        
;    kprintf("Invalid O65 file, missing magic marker\n");
;#endif    
;    *o65_error = O65_INVALID_SIGNATURE;
	sep	#$20
	longa	off
	lda	#$1
	sta	[<L73+o65_error_0]
	rep	#$20
	longa	on
;    return;
L79:
	tay
	lda	<L73+2
	sta	<L73+2+8
	lda	<L73+1
	sta	<L73+1+8
	pld
	tsc
	clc
	adc	#L73+8
	tcs
	tya
	rtl
;  }
;  program += 8;
L10026:
	clc
	lda	#$8
	adc	<L73+program_0
	sta	<L73+program_0
	bcc	L80
	inc	<L73+program_0+2
L80:
;
;#if KTRACE
;  kprintf("Mode: %04x, offsets start %08lx...\n", header->mode, program);
;#endif  
;
;  // Now load offsets, choosing the right size depending on what mode.
;  size = header->mode & 0x2000 ? 1 : 0;
	ldy	#$6
	lda	[<L74+header_1],Y
	and	#<$2000
	bne	L82
	brl	L81
L82:
	lda	#$1
	bra	L83
L81:
	lda	#$0
L83:
	sta	<L74+size_1
;  if (size) {
	lda	<L74+size_1
	bne	L84
	brl	L10027
L84:
;    struct O65OffsetsLong far* offsets = (struct O65OffsetsLong far*)program;
;    tbase = offsets->tbase;
offsets_2	set	54
	lda	<L73+program_0
	sta	<L74+offsets_2
	lda	<L73+program_0+2
	sta	<L74+offsets_2+2
	lda	[<L74+offsets_2]
	sta	<L74+tbase_1
	ldy	#$2
	lda	[<L74+offsets_2],Y
	sta	<L74+tbase_1+2
;    tlen = offsets->tlen;
	ldy	#$4
	lda	[<L74+offsets_2],Y
	sta	<L74+tlen_1
	ldy	#$6
	lda	[<L74+offsets_2],Y
	sta	<L74+tlen_1+2
;    dbase = offsets->dbase;
	ldy	#$8
	lda	[<L74+offsets_2],Y
	sta	<L74+dbase_1
	ldy	#$a
	lda	[<L74+offsets_2],Y
	sta	<L74+dbase_1+2
;    dlen = offsets->dlen;
	ldy	#$c
	lda	[<L74+offsets_2],Y
	sta	<L74+dlen_1
	ldy	#$e
	lda	[<L74+offsets_2],Y
	sta	<L74+dlen_1+2
;    bssbase = offsets->bbase;
	ldy	#$10
	lda	[<L74+offsets_2],Y
	sta	<L74+bssbase_1
	ldy	#$12
	lda	[<L74+offsets_2],Y
	sta	<L74+bssbase_1+2
;    bsslen = offsets->blen;
	ldy	#$14
	lda	[<L74+offsets_2],Y
	sta	<L74+bsslen_1
	ldy	#$16
	lda	[<L74+offsets_2],Y
	sta	<L74+bsslen_1+2
;    zpbase = offsets->zbase;
	ldy	#$18
	lda	[<L74+offsets_2],Y
	sta	<L74+zpbase_1
	ldy	#$1a
	lda	[<L74+offsets_2],Y
	sta	<L74+zpbase_1+2
;    zlen = offsets->zlen;
	ldy	#$1c
	lda	[<L74+offsets_2],Y
	sta	<L74+zlen_1
	ldy	#$1e
	lda	[<L74+offsets_2],Y
	sta	<L74+zlen_1+2
;    stack = offsets->stack;
	ldy	#$20
	lda	[<L74+offsets_2],Y
	sta	<L74+stack_1
	ldy	#$22
	lda	[<L74+offsets_2],Y
	sta	<L74+stack_1+2
;    program += 36;
	clc
	lda	#$24
	adc	<L73+program_0
	sta	<L73+program_0
	bcc	L85
	inc	<L73+program_0+2
L85:
;  } else {
	brl	L10028
L10027:
;    struct O65OffsetsWord far* offsets = (struct O65OffsetsWord far*)program;
;    tbase = offsets->tbase;
offsets_3	set	54
	lda	<L73+program_0
	sta	<L74+offsets_3
	lda	<L73+program_0+2
	sta	<L74+offsets_3+2
	lda	[<L74+offsets_3]
	sta	<L74+tbase_1
	stz	<L74+tbase_1+2
;    tlen = offsets->tlen;
	ldy	#$2
	lda	[<L74+offsets_3],Y
	sta	<L74+tlen_1
	stz	<L74+tlen_1+2
;    dbase = offsets->dbase;
	ldy	#$4
	lda	[<L74+offsets_3],Y
	sta	<L74+dbase_1
	stz	<L74+dbase_1+2
;    dlen = offsets->dlen;
	ldy	#$6
	lda	[<L74+offsets_3],Y
	sta	<L74+dlen_1
	stz	<L74+dlen_1+2
;    bssbase = offsets->bbase;
	ldy	#$8
	lda	[<L74+offsets_3],Y
	sta	<L74+bssbase_1
	stz	<L74+bssbase_1+2
;    bsslen = offsets->blen;
	ldy	#$a
	lda	[<L74+offsets_3],Y
	sta	<L74+bsslen_1
	stz	<L74+bsslen_1+2
;    zpbase = offsets->zbase;
	ldy	#$c
	lda	[<L74+offsets_3],Y
	sta	<L74+zpbase_1
	stz	<L74+zpbase_1+2
;    zlen = offsets->zlen;
	ldy	#$e
	lda	[<L74+offsets_3],Y
	sta	<L74+zlen_1
	stz	<L74+zlen_1+2
;    stack = offsets->stack;
	ldy	#$10
	lda	[<L74+offsets_3],Y
	sta	<L74+stack_1
	stz	<L74+stack_1+2
;    program += 18;
	clc
	lda	#$12
	adc	<L73+program_0
	sta	<L73+program_0
	bcc	L86
	inc	<L73+program_0+2
L86:
;  }
L10028:
;
;#if KTRACE
;  kprintf("size: %d tbase: %08lx tlen: %08lx dbase: %08lx dlen: %08lx\n", size,
;          tbase, tlen, dbase, dlen);
;#endif          
;
;  if ((uint32_t)program + tlen + dlen + dlen > 0x200000) {
	clc
	lda	<L73+program_0
	adc	<L74+tlen_1
	sta	<R0
	lda	<L73+program_0+2
	adc	<L74+tlen_1+2
	sta	<R0+2
	clc
	lda	<R0
	adc	<L74+dlen_1
	sta	<R1
	lda	<R0+2
	adc	<L74+dlen_1+2
	sta	<R1+2
	clc
	lda	<R1
	adc	<L74+dlen_1
	sta	<R0
	lda	<R1+2
	adc	<L74+dlen_1+2
	sta	<R0+2
	lda	#$0
	cmp	<R0
	lda	#$20
	sbc	<R0+2
	bcc	L87
	brl	L10029
L87:
;#if KTRACE
;    kprintf("Program size/relocation exceeds physical memory");
;#endif    
;    *o65_error = O65_PROGRAM_SIZE;
	sep	#$20
	longa	off
	lda	#$2
	sta	[<L73+o65_error_0]
	rep	#$20
	longa	on
;    return;
	brl	L79
;  }
;#if KTRACE  
;  kprintf("header_options_start: %08lx\n", program);
;#endif  
;
;  // Now throw away  header options
;  program = LoadHeaderOptions(program);
L10029:
	pei	<L73+program_0+2
	pei	<L73+program_0
	jsl	~~LoadHeaderOptions
	sta	<L73+program_0
	stx	<L73+program_0+2
;
;#if KTRACE  
;  kprintf("segments_start: %08lx\n", program);
;#endif
;
;  // Segments start here.
;  seg_base = program;
	lda	<L73+program_0
	sta	<L74+seg_base_1
	lda	<L73+program_0+2
	sta	<L74+seg_base_1+2
;
;  // Jump ahead to relocation tables, which are at the end of the segments.
;  program += tlen + dlen;
	clc
	lda	<L74+tlen_1
	adc	<L74+dlen_1
	sta	<R0
	lda	<L74+tlen_1+2
	adc	<L74+dlen_1+2
	sta	<R0+2
	clc
	lda	<L73+program_0
	adc	<R0
	sta	<L73+program_0
	lda	<L73+program_0+2
	adc	<R0+2
	sta	<L73+program_0+2
;
;  seg_end = program;
	lda	<L73+program_0
	sta	<L74+seg_end_1
	lda	<L73+program_0+2
	sta	<L74+seg_end_1+2
;
;#if KTRACE  
;  kprintf("seg_base: %08lx seg_end: %08lx\n", seg_base, seg_end);
;#endif  
;
;  // Obtain the external (undefined) references list. This is meaningless for us
;  // tho.
;  program = LoadExternalReferences(program, size);
	pei	<L74+size_1
	pei	<L73+program_0+2
	pei	<L73+program_0
	jsl	~~LoadExternalReferences
	sta	<L73+program_0
	stx	<L73+program_0+2
;
;#if KTRACE  
;  kprintf("relocations begin: %08lx\n", program);
;#endif  
;
;  // Now the relocation tables, the fun part
;  reloc_address = (uint32_t)seg_base;
	lda	<L74+seg_base_1
	sta	<L74+reloc_address_1
	lda	<L74+seg_base_1+2
	sta	<L74+reloc_address_1+2
;  program = DoRelocations(program, reloc_address, seg_base, tbase, tlen, dbase,
;                          dlen, bssbase, zpbase);
	pei	<L74+zpbase_1+2
	pei	<L74+zpbase_1
	pei	<L74+bssbase_1+2
	pei	<L74+bssbase_1
	pei	<L74+dlen_1+2
	pei	<L74+dlen_1
	pei	<L74+dbase_1+2
	pei	<L74+dbase_1
	pei	<L74+tlen_1+2
	pei	<L74+tlen_1
	pei	<L74+tbase_1+2
	pei	<L74+tbase_1
	pei	<L74+seg_base_1+2
	pei	<L74+seg_base_1
	pei	<L74+reloc_address_1+2
	pei	<L74+reloc_address_1
	pei	<L73+program_0+2
	pei	<L73+program_0
	jsl	~~DoRelocations
	sta	<L73+program_0
	stx	<L73+program_0+2
;
;#if KTRACE  
;  kprintf("Done relocations. Clearing bss...\n");
;#endif  
;  // Could look for exported symbols. Don't need to now.
;
;  // Now zero out the BSS, which will wipe out relocation tables and symbols
;  // BSS
;  while (bsslen--) {
L10030:
	lda	<L74+bsslen_1
	sta	<R0
	lda	<L74+bsslen_1+2
	sta	<R0+2
	lda	<L74+bsslen_1
	bne	L88
	dec	<L74+bsslen_1+2
L88:
	dec	<L74+bsslen_1
	lda	<R0
	ora	<R0+2
	bne	L89
	brl	L10031
L89:
;    *(seg_end++) = 0;
	sep	#$20
	longa	off
	lda	#$0
	sta	[<L74+seg_end_1]
	rep	#$20
	longa	on
	inc	<L74+seg_end_1
	bne	L90
	inc	<L74+seg_end_1+2
L90:
;  }
	brl	L10030
L10031:
;
;#if KTRACE  
;  kprintf("Done.\n");
;#endif  
;  *o65_error = O65_LOAD_OK;
	sep	#$20
	longa	off
	lda	#$0
	sta	[<L73+o65_error_0]
	rep	#$20
	longa	on
;  return seg_base;
	ldx	<L74+seg_base_1+2
	lda	<L74+seg_base_1
	brl	L79
;}
L73	equ	66
L74	equ	9
	ends
	efunc
;
	end
