%.o: %.a65
	ca65 -g --cpu 65816 $< -l $(basename $<).lst

%.bin: %.o
	ld65 -C o65_loader.cfg -Ln o65_loader.lbl -m o65_loader.map $< -o $@

o65_loader.bin: o65_loader.o
o65_loader.o: o65_loader.a65

clean:
	rm -f *.o o65_loader.bin *.lst *.lbl

