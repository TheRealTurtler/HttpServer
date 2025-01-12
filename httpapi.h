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

    HttpResponse replyHome();

    HttpResponse replyGetTest();
    HttpResponse replyPostTest(const HttpRequest& request);

    static QMultiHash<QString, QString> getDefaultHeaders();
};

#endif // HTTPAPI_H
