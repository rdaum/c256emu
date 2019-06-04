# c256emu

This is a work in progress emulator for the [C256 Foenix](https://c256foenix.com/) microcomputer.

It is written in C++ and renders to a window via SDL2. It's relatively fast, and on my i5 workstation emulates the C256
at around 30mhz and a full 60fps.

It supports the following features at the moment. There are definitely bugs and missing pieces. There are definitely
going to be mismatches with the hardware as I do not have a board to compare against yet. When the Rev C boards are
released, I will test and fix accordingly.

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
  * libgflags-dev
  * libadplug-dev
  * libsrecord-dev
  * libgoogle-glog-dev

Then kick off cmake as usual:

  ```shell
  cmake .
  make
  ```

The build will clone and install its own copy of the Google test framework,
the liblinenoise-ng library, and the 'jm' circular_buffer library.

## Using

Example invocation:

   `./c256emu -kernel_bin=$HOME/kernel.bin`

Will load `kernel.bin` into memory starting at `$18:000` before starting the CPU. This emulates the C256's normal boot
sequence, in which kernel flash memory is copied to the same location. After loading the first page (`$18:0000-$18:ffff`)
is copied to the lower 64k, which initializes the reset vector and initial program state before boot.

Below are the set of arguments the program accepts:

  * `-kernel_bin` (Location of kernel .bin file) type: string default: ""
  * `-kernel_hex` (Location of kernel .hex file) type: string default: ""
  * `-program_hex` (Program HEX file to load (optional)) type: string default: ""
  * `-automation` (enable Lua automation / debug scripting) type: bool
     default: false
  * `-script` (Lua script to run on start (automation only)) type: string
     default: ""
  * `-profile` (enable CPU performance profiling) type: bool default: false

To run the emulator you will need to at minimum provide either a `-kernel_bin` argument or `kernel_hex` argument. Both
arguments are for loading a bootable kernel into the emulated C256's
memory. `-kernel_bin` takes a raw binary file consisting of a 65816 program that will be loaded into memory at $18:0000.
kernel_hex takes any format recognized by libsrecord, such as Intel Hex Extended, etc. The hex format itself will
determine where in memory the kernel is loaded, but should be `$18:0000` and up, as described by the `kernel.bin` argument above.

The program_hex argument allows for additional code to be loaded in at the location of your choosing. Loading in at
`$19:0000`, for example, will override the C256's normal kernel boot with a program of your own choosing.

The `-automation` argument turns on a scripting facility which does the following:

  * Starts an interactive Lua interpreter which is prepopulated with functions for stopping and starting the CPU,
    inspecting memory and CPU state, etc.
  * Allows an initial automation script (script argument) to be passed to do the same things the interactive interpreter
    can do.

The `-profile` argument does some primitive measurements of the emulated FPS and Mhz values.
