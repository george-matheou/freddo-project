SOURCES=$(wildcard *.cpp)
EXECS=$(SOURCES:.cpp=)
BIN_DIR=./bin
Binaries := $(addprefix $(BIN_DIR)/,$(EXECS))

all:$(EXECS)

%:%.cpp
	$(CXX) $< $(CXXFLAGS) -o $(BIN_DIR)/$@

clean:
	rm -f $(Binaries)
	