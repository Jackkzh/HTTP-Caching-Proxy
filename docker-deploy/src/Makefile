all: main

main: main.cpp server.cpp server.h httpcommand.cpp httpcommand.h
	g++ -g  -o main main.cpp server.cpp httpcommand.cpp -lpthread

.PHONY:
	clean
clean:
	rm -rf *.o main