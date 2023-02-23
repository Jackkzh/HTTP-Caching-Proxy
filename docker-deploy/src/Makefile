all: main

main: main.cpp httpcommand.cpp httpcommand.h myexception.h helper.cpp helper.h proxy.h proxy.cpp cache.h cache.cpp
	g++ -g -o main main.cpp httpcommand.cpp helper.cpp proxy.cpp cache.cpp -lpthread -lboost_regex

.PHONY:
	clean
clean:
	rm -rf *.o main