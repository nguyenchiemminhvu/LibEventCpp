CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -Wunused-parameter -Wunused-function -g
TARGET = sample_event_handler sample_sigslot sample_timer sample_once_event sample_fd_event sample_signal_event sample_toggle_event sample_fs_event
HEADERS = libevent.h

all: $(TARGET)

sample_event_handler: sample_event_handler.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_event_handler sample_event_handler.cpp -pthread

sample_sigslot: sample_sigslot.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_sigslot sample_sigslot.cpp -pthread

sample_timer: sample_timer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_timer sample_timer.cpp -lpthread -lrt

sample_once_event: sample_once_event.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_once_event sample_once_event.cpp -pthread -lrt

sample_fd_event: sample_fd_event.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_fd_event sample_fd_event.cpp -pthread -lrt

sample_signal_event: sample_signal_event.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_signal_event sample_signal_event.cpp -pthread -lrt

sample_toggle_event: sample_toggle_event.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_toggle_event sample_toggle_event.cpp -pthread -lrt

sample_fs_event: sample_fs_event.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o sample_fs_event sample_fs_event.cpp -pthread -lrt

clean:
	rm -f $(TARGET)
