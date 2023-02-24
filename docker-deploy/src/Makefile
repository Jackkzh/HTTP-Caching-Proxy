all: main

main: main.cpp client.h client.cpp httpcommand.cpp httpcommand.h myexception.h helper.cpp helper.h proxy.h proxy.cpp ResponseInfo.h ResponseInfo.cpp
	g++ -g -std=c++11 -o main main.cpp client.cpp httpcommand.cpp helper.cpp proxy.cpp ResponseInfo.cpp -lpthread -lboost_regex

.PHONY:
	clean
clean:
	rm -rf *.o main