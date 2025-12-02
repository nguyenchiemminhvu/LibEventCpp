CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -Wunused-parameter -Wunused-function -g
TARGET = sample_event sample_sigslot sample_timer sample_once_event sample_fd_event
HEADERS = libevent.h

all: $(TARGET)

$(TARGET): sample_event.cpp sample_sigslot.cpp sample_timer.cpp sample_once_event.cpp sample_fd_event.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_event sample_event.cpp -pthread
	$(CXX) $(CXXFLAGS) -o sample_sigslot sample_sigslot.cpp -pthread
	$(CXX) $(CXXFLAGS) -o sample_timer sample_timer.cpp -lpthread -lrt
	$(CXX) $(CXXFLAGS) -o sample_once_event sample_once_event.cpp -pthread -lrt
	$(CXX) $(CXXFLAGS) -o sample_fd_event sample_fd_event.cpp -pthread -lrt

clean:
	rm -f $(TARGET)
