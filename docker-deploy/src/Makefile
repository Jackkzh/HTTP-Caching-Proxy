all: main

main: main.cpp server.cpp server.h httpcommand.cpp httpcommand.h client.h client.cpp
	g++ -g  -o main main.cpp server.cpp httpcommand.cpp client.cpp -lpthread

.PHONY:
	clean
clean:
	rm -rf *.o main