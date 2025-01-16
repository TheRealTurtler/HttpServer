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
    ~HttpAPI();

public slots:
    void start();
    void stop();

private:
    HttpServer* m_server = nullptr;

    HttpResponse cbGET(const HttpRequest& request, const QString& logInfo);
    HttpResponse cbPOST(const HttpRequest& request, const QString& logInfo);

    HttpResponse cbHome(const HttpRequest& request, const QString& logInfo);
    HttpResponse cbPing(const HttpRequest& request, const QString& logInfo);
    HttpResponse cbEcho(const HttpRequest& request, const QString& logInfo);
    HttpResponse cbTestGET(const HttpRequest& request, const QString& logInfo);
    HttpResponse cbTestPOST(const HttpRequest& request, const QString& logInfo);
};

#endif // HTTPAPI_H
