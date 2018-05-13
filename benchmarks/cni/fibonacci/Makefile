SOURCES=$(wildcard *.cpp)
EXECS=$(SOURCES:.cpp=)
BIN_DIR=./bin
Binaries := $(addprefix $(BIN_DIR)/,$(EXECS))

all:$(EXECS)

%:%.cpp
	$(CXX) $< $(CXXFLAGS) -o $(BIN_DIR)/$@

Timer.o: Timer.h Timer.c
	gcc -c Timer.c -o Timer.o

clean:
	rm -f $(Binaries) Timer.o
	
	