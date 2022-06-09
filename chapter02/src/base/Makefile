CXX= g++ -std=c++17
CXXFLAGS= -g -O2 -Wall -Wextra
LDFLAGS=
LIBS= -pthread

CORE_O= main.o count_down_latch.o thread.o current_thread.o

ALL_T= main
ALL_O= $(CORE_O)

# Targets start here.

t: $(ALL_T)

o: $(ALL_O)

$(ALL_T): $(ALL_O)
	$(CXX) -o $@ $(LDFLAGS) $(ALL_O) $(LIBS)

clean:
	rm -rf $(ALL_O) $(ALL_T)

.PHONY: clean o t

main.o: main.cc condition.h mutex.h thread_local.h current_thread.h
count_down_latch.o: count_down_latch.cc count_down_latch.h
thread.o: thread.cc thread.h current_thread.h
current_thread.o: current_thread.cc current_thread.h common.h

# mutex.h
# condition.h
# thread_local.h
