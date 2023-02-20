all: main

main: main.cpp server.cpp server.h client.h client.cpp httpcommand.cpp httpcommand.h myexception.h
	g++ -g  -o main main.cpp server.cpp client.cpp httpcommand.cpp -lpthread

.PHONY:
	clean
clean:
	rm -rf *.o main