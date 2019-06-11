# c256emu

This is a work in progress emulator for the [C256 Foenix](https://c256foenix.com/) microcomputer.

It is written in C++ and renders to a window via SDL2. It's relatively fast, 
and on my i5 workstation easily emulates the C256 at full 14mhz and full 60fps.

It supports the following features at the moment. There are definitely bugs and missing pieces. There are definitely
going to be mismatches with the hardware as I do not have a board to compare against yet. When the Rev C boards are
released, I will test and fix accordingly.

## Functionality

#### CPU

Uses achaulk's 65c816 emulator from [RetroCpu](https://github.com/achaulk)

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

  * libadplug-dev
  * libsrecord-dev

Then kick off cmake as usual:

  ```shell
  cmake -DCMAKE_BUILD_TYPE=Release .
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
  * `-turbo` (turn off frame rate / CPU throttling, go as fast as possible)

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

## Debug / Automation

When `-automation` is passed as an argument the console will present a 
read loop with command history and arrow keys and so on, which can execute Lua expressions while the emulator runs.

The following functions / variables are available:

```lua
-- Pause the CPU emulator at the current instruciotn.
c256emu.stop()

-- Continue if paused.
c256emu.continue()

-- Single step to the next instruction.
c256emu.step()

-- Add breakpoint to invoke the function <func> when the PC hits <addr>
c256emu.add_breakpoint(<addr>, <func>)

-- Clear the breakpoint at <addr>
c256emu.clear_breakpoint(<addr>)

-- Return a list of all breakpoints
c256emu.breakpoints() 

-- Read the byte at <addr>
c256emu.peek(<addr>)

-- Read the word at <addr>
c256emu.peek16(<addr>)

-- Modify the byte at <addr>
c256emu.poke(<addr>, <byte>)

-- Modify the word at <addr>
c256emu.poke16(<addr>, <word>)

-- Return a binary dump of <num_bytes> starting at <addr>
c256emu.peekbuf(<addr>, <num_bytes>)

-- Disassemble the 65816 program at <addr>, up to <count> lines, and return
-- a table of (line#, code). If <addr> is omitted, disassemble from the 
-- current PC on (if CPU is stopped.)
c256emu.disassemble(<addr>, <count>)

-- Load the binary <file> into <addr>
c256emu.load_bin(<file>, <addr>)

-- Load the Intel Hex, SRec, etc. file. The load address is assumed
-- to be part of the hex file. 
c256emu.load_hex(<file>)

-- Jump the program counter to <addr>
c256emu.sys(<addr>)

-- The following are self explanatory.
c256emu.cpu_state().pc
c256emu.cpu_state().a
c256emu.cpu_state().x
c256emu.cpu_state().y
c256emu.cpu_state().cycle_count
c256emu.cpu_state().status.carry_flag
c256emu.cpu_state().status.zero_flag
c256emu.cpu_state().status.interrupt_disable_flag
c256emu.cpu_state().status.decimal_flag
c256emu.cpu_state().status.break_flag
c256emu.cpu_state().status.accumulator_width_flag
c256emu.cpu_state().status.index_width_flag
c256emu.cpu_state().status.emulation_flag
c256emu.cpu_state().status.overflow_flag
c256emu.cpu_state().status.sign_flag
```

The `-script` argument can be used to read any Lua program, to set up functions, breakpoints, etc. to execute on boot.

### What missing from the debugger right now:

  * GUI
  * Raise / clear interrupts
  * Breakpoints on interrupts
  * Assembler
  * Conditional breakpoints

