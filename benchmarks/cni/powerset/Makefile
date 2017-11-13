include ../Makefile.in

all: bin/powerset

bin/powerset: powerset.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/powerset

clean:
	rm -f bin/powerset Timer.o
	
	