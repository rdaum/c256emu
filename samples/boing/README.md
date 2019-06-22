# Run in emulator
c256emu.load_bin("samples/boing/tiles.bin",0xb00000);
c256emu.load_bin("samples/boing/boing.bin",0x10000);
c256emu.sys(0x10000)
