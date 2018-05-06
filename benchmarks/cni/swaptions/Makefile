
all:
	make -C src -f Makefile.freddo clean
	make -C src -f Makefile.freddo
	cp src/swaptions_freddo ./bin
	
	make -C src -f Makefile.serial clean
	make -C src -f Makefile.serial
	cp src/swaptions_serial ./bin
	
clean:
	make -C src -f Makefile.freddo clean
	make -C src -f Makefile.serial clean
	rm -f ./bin/swaptions_freddo
	rm -f ./bin/swaptions_serial