all: main

main: main.cpp server.cpp server.h client.h client.cpp httpcommand.cpp httpcommand.h myexception.h helper.cpp helper.h proxy.h
	g++ -g  -o main main.cpp server.cpp client.cpp httpcommand.cpp helper.cpp -lpthread

.PHONY:
	clean
clean:
	rm -rf *.o main