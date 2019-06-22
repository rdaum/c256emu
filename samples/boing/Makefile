%.o: %.asm
	ca65 --cpu 65816 $< -l $(basename $<).lst

%.o65: %.o
	ld65 -C boing.cfg -m boing.map $< -o $@

boing.o65: boing.o

boing.o: boing.asm place_tiles.asm clear_tiles.asm set_lut.asm \
         ball.asm macros.inc

clean:
	rm -f *.o boing.o65 *.lst

