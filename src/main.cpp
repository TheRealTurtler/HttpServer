#include <QCoreApplication>
#include <QThread>

#include "httpapi.h"


int main(int argc, char *argv[])
{   
    QCoreApplication a(argc, argv);

    QThread* thr = new QThread();
    thr->setObjectName("HttpAPI");

    HttpAPI* api = new HttpAPI();
    api->moveToThread(thr);

    QObject::connect(thr, &QThread::started, api, &HttpAPI::start);

    thr->start();

    return a.exec();
}
