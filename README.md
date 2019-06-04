# c256emu

This is a work in progress emulator for the [C256 Foenix](https://www.google.com) microcomputer.

It is written in C++ and renders to via SDL2.

It supports the following features at the moment. There are definitely bugs and missing pieces. There are definitely going to be mismatches with the hardware as I do not have a board to compare against yet. When the Rev C boards are released, I will test and fix accordingly.

## Functionality

#### CPU

A fork of [Lib65816](https://github.com/FrancescoRigoni/Lib65816) with major changes:

  * Bug fixes (will enumerate once I diff back to the original source)
  * Code style changes (moving towards Google C++ style)
  * Optimization changes (mostly in relation to memory addressing / address decoding)

#### Debugger / Automation

Embedded Lua interpretter that can do the following:

  * Set breakpoints
  * Inspect CPU state
  * Read and modify memory
  * Has bugs, needs love. (stepping and resume after suspend currently broken)
  
#### Interrupt controller

  * Seems to service most of the known interrupts correctly, but undoubtably complete.
  
#### Vicky VDP

  * Bitmap graphics
  * Character generator w/ cursor blink.
  * Border
  * Sprites
  * Tile sets are currently buggy/incorrect.
  * Mouse cursor support (untested)

#### CH376 SD card controller functionality:

  * Directory listing
  * File open and read
  * No write support

#### Keyboard support:

  * Keyboard input with scancode conversion (probably incomplete)
  * No mouse support

#### Math coprocessor

  * Seems right.

### Missing / broken emulation / features

  * DMA
  * MIDI
  * Serial
  * Sound (basic framework for OPL2 emulation there but not hooked up to audio mixing)
  * More complete SD card support
  * Fully working debugger

## Building

To build, you will need a C++17 compliant compiler (clang or GCC should work), CMake 3.10 or greater, and will need to install the following build dependencies (Debian packages listed):

  * libsdl2-dev
  * liblua5.1-0-dev
  * libboost1.65-dev
  * libreadline-dev
  * libgflags-dev
  * libadplug-dev
  * libsrecord-de
  * libgoogle-glog-dev

Then kick off cmake as usual. (cmake . followed by make, etc.)

The build will clone and install its own copy of the Google test framework.

