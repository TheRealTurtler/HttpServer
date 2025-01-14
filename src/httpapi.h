#ifndef HTTPAPI_H
#define HTTPAPI_H

#include <QObject>

#include "httpserver.h"
#include "httpresponse.h"


class HttpAPI : public QObject
{
    Q_OBJECT

public:
    explicit HttpAPI(QObject* parent = nullptr);

public slots:
    void start();

private:
    HttpServer* _server = nullptr;

    HttpResponse cbGET(const HttpRequest& request);
    HttpResponse cbPOST(const HttpRequest& request);

    HttpResponse cbHome(const HttpRequest& request);
    HttpResponse cbEcho(const HttpRequest& request);
    HttpResponse cbTestGET(const HttpRequest& request);
    HttpResponse cbTestPOST(const HttpRequest& request);
};

#endif // HTTPAPI_H
