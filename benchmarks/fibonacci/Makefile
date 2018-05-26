SOURCES=$(wildcard *.cpp)
EXECS=$(SOURCES:.cpp=)
BIN_DIR=./bin
Binaries := $(addprefix $(BIN_DIR)/,$(EXECS))

# Set the default goal of this makefile
.DEFAULT_GOAL := all

.PHONY: all
all:$(EXECS)

%:%.cpp
	$(CXX_MPI) $< $(CXXFLAGS) -o $(BIN_DIR)/$@

Timer.o: Timer.h Timer.c
	gcc -c Timer.c -o Timer.o

clean:
	rm -f $(Binaries) Timer.o
	
	