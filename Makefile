CXX = g++
CXXFLAGS = -std=c++20 -Wall

all: server

compile: server run

server: server.cpp p1_helper.cpp
	$(CXX) $(CXXFLAGS) -o server server.cpp p1_helper.cpp
client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp
	./client 192.168.0.10
run:
	./server server.conf
clean:
	rm -f server
	rm -f client