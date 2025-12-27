#include <QCoreApplication>
#include "TcpServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    TcpServer server;
    if (!server.startServer(8888)) {
        return 1;
    }

    return a.exec();
}
