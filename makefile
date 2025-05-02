

server: tool.cpp main.cpp WebServer.cpp    timer/timer.cpp  http/http_connect.cpp locker/locker.h mmysql/Mysql.cpp threadpool/threadpool.hpp
	g++ -o server $^ -I. -lmysqlclient -lpthread
	./server
	rm server


gdbserver: tool.cpp main.cpp WebServer.cpp    timer/timer.cpp  http/http_connect.cpp locker/locker.h mmysql/Mysql.cpp threadpool/threadpool.hpp
	g++ -o  server $^ -I. -g -lmysqlclient -lpthread
	
	