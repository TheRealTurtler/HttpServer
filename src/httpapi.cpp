#include "httpapi.h"

#include <QJsonDocument>


HttpAPI::HttpAPI(QObject* parent) :
    QObject(parent)
{
    _server = new HttpServer(QHostAddress::Any, 8080, this);

    {
        auto cb = [this](const HttpRequest& request){ return replyHome(); };
        _server->setCallback(HttpRequest::GET, "/", cb);
    }

    {
        auto cb = [this](const HttpRequest& request){ return replyGetTest(); };
        _server->setCallback(HttpRequest::GET, "/test", cb);
    }

    {
        auto cb = [this](const HttpRequest& request){ return replyPostTest(request); };
        _server->setCallback(HttpRequest::POST, "/test", cb);
    }
}

void HttpAPI::start()
{
    _server->start();
}

HttpResponse HttpAPI::replyHome()
{
    HttpResponse result;

    QString strBody = "Home";

    result.setStatus(HttpResponse::OK);
    result.addHeader("Content-Type", "text/html; charset=utf-8");
    result.setBody(strBody.toUtf8());
    return result;
}

HttpResponse HttpAPI::replyGetTest()
{
    HttpResponse result;

    QString strBody = "GET Test OK";

    result.setStatus(HttpResponse::OK);
    result.addHeader("Content-Type", "text/html; charset=utf-8");
    result.setBody(strBody.toUtf8());
    return result;
}

HttpResponse HttpAPI::replyPostTest(const HttpRequest& request)
{
    HttpResponse result;

    const QJsonDocument json = QJsonDocument::fromJson(request.getBody());

    if (json.isNull())
    {
        result.setStatus(HttpResponse::BAD_REQUEST);
        return result;
    }

    const QJsonValue valFunction = json["function"];

    if (valFunction == QJsonValue::Undefined || !valFunction.isString())
    {
        result.setStatus(HttpResponse::BAD_REQUEST);
        return result;
    }

    const QString strFunction = valFunction.toString();

    if (strFunction == "hello")
    {
        QString strBody = "Hello World!";

        result.setStatus(HttpResponse::OK);
        result.addHeader("Content-Type", "text/html; charset=utf-8");
        result.setBody(strBody.toUtf8());
    }
    else if (strFunction == "echo")
    {
        result.setStatus(HttpResponse::OK);
        result.addHeader("Content-Type", "application/json");
        result.setBody(request.getBody());
    }
    else if (strFunction == "secret")
    {
        const QJsonValue valPassword = json["password"];

        if (valPassword == QJsonValue::Undefined || !valPassword.isString())
        {
            result.setStatus(HttpResponse::BAD_REQUEST);
            return result;
        }

        if (valPassword.toString() == "asdf1234")
        {
            QString strBody = "Password correct!";

            result.setStatus(HttpResponse::OK);
            result.addHeader("Content-Type", "text/html; charset=utf-8");
            result.setBody(strBody.toUtf8());
        }
        else
            result.setStatus(HttpResponse::UNAUTHORIZED);
    }
    else
        result.setStatus(HttpResponse::BAD_REQUEST);

    return result;
}
