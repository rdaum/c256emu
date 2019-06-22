%.o: %.asm
	ca65 --cpu 65816 $< -l $(basename $<).lst

%.bin: %.o
	ld65 -C boing.cfg $< -o $@

boing.bin: boing.o

boing.o: boing.asm place_tiles.asm clear_tiles.asm set_lut.asm \
         ball.asm macros.inc

clean:
	rm -f *.o boing.bin *.lst

