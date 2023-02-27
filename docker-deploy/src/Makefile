all: main

main: main.cpp httpcommand.cpp httpcommand.h myexception.h helper.cpp helper.h proxy.h proxy.cpp cache.h cache.cpp ResponseInfo.h ResponseInfo.cpp
	g++ -g -o main main.cpp httpcommand.cpp helper.cpp proxy.cpp cache.cpp ResponseInfo.cpp -lpthread -lboost_system -lboost_regex -lboost_date_time

.PHONY:
	clean
clean:
	rm -rf *.o main