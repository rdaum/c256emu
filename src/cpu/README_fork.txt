Forked from Francesco Rigoni's Lib65816 (https://github.com/FrancescoRigoni/Lib65816) with the following changes:

  * Fixed reset vector handling
  * Addition to memory read/write API to allow optimization for contiguous access when the device can provide it.
  * Various bug fixes in opcodes, though I've forgotten the details of most of them.
    (Most related to incorrect handling of long jumps, or 16-bit values.)
  * Removing gratuitous accessor methods. Just a style thing, and a few cycles saved.
  * Formatting, style and file naming to conform to Chromium/Google C++ style guide. (INCOMPLETE)

I may not keep use of this CPU emulator. There's a faster one available I may switch to.

Original README.md:

# Lib65816
Emulator library for the 65816 CPU.<br>
See https://codebutchery.wordpress.com/65816-cpu-emulator/

Disclaimer:
I didn't test this enough to claim that it works well, you are free to use it but you might have to do some bugfixing :)
