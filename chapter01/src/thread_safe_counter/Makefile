CXX= g++ -std=c++17
CXXFLAGS= -g -O2 -Wall -Wextra
LDFLAGS=
LIBS= -pthread 

ALL_T= main
ALL_O= main.o

# Targets start here.
$(ALL_T): $(ALL_O)
	$(CXX) -o $@ $(LDFLAGS) $(ALL_O) $(LIBS)

clean:
	rm -rf $(ALL_O) $(ALL_T)

.PHONY: clean o t

main.o: main.cc 
