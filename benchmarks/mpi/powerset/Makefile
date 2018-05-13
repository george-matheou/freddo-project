all: bin/powerset

EXECS=bin/powerset

bin/powerset: powerset.cpp
	$(CXX_MPI) $< $(CXXFLAGS) -o bin/powerset
	
clean:
	rm -f bin/powerset
	
	