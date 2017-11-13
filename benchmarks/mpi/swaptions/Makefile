
.PHONY: openmpi
openmpi:
	make -C src -f Makefile.freddo clean
	make -C src -f Makefile.freddo openmpi
	cp src/swaptions_freddo ./bin
	
.PHONY: mvapich	
mvapich:
	make -C src -f Makefile.freddo clean
	make -C src -f Makefile.freddo mvapich
	cp src/swaptions_freddo ./bin

.PHONY: mpich	
mpich:
	make -C src -f Makefile.freddo clean
	make -C src -f Makefile.freddo mpich
	cp src/swaptions_freddo ./bin

.PHONY: mpidefault	
mpidefault:
	make -C src -f Makefile.freddo clean
	make -C src -f Makefile.freddo mpidefault
	cp src/swaptions_freddo ./bin
	
serial:
	make -C src -f Makefile.serial clean
	make -C src -f Makefile.serial
	cp src/swaptions_serial ./bin

clean:
	make -C src -f Makefile.freddo clean
	make -C src -f Makefile.serial clean
	rm -f ./bin/swaptions_freddo
	rm -f ./bin/swaptions_serial