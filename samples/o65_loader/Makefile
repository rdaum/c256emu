%.o: %.a65
	ca65 --cpu 65816 $< -l $(basename $<).lst

%.bin: %.o
	ld65 -C o65_loader.cfg -S 0x10000 -m o65_loader.map $< -o $@

o65_loader.bin: o65_loader.o
o65_loader.o: o65_loader.a65

clean:
	rm -f *.o o65_loader.bin *.lst

