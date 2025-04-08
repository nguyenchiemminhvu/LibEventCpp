CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra
TARGET = sample_event sample_sigslot sample_timer
HEADERS = libevent.h

all: $(TARGET)

$(TARGET): sample_event.cpp sample_sigslot.cpp sample_timer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_event sample_event.cpp -lpthread
	$(CXX) $(CXXFLAGS) -o sample_sigslot sample_sigslot.cpp -lpthread
	$(CXX) $(CXXFLAGS) -o sample_timer sample_timer.cpp -lpthread -lrt

clean:
	rm -f $(TARGET)
