#include "WebServer.h"

int main()
{
    webserver server;
    server.threadpoolInit();
    server.mysqlInit();
    server.eventListen();
    server.eventLoop();
    return 0;
}