CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra
TARGET = sample_event sample_sigslot
HEADERS = libevent.h

all: $(TARGET)

$(TARGET): sample_event.cpp sample_sigslot.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_event sample_event.cpp -lpthread
	$(CXX) $(CXXFLAGS) -o sample_sigslot sample_sigslot.cpp -lpthread

clean:
	rm -f $(TARGET)
