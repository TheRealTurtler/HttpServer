#include "httpresponse.h"


const QHash<HttpResponse::STATUS, QString> HttpResponse::_statusTexts = initStatusTexts();


HttpResponse::HttpResponse()
{

}

void HttpResponse::setBody(const QByteArray &body)
{
    _body = body;

    if (_body.size() > 0)
        setHeader("Content-Length", QString::number(_body.size()));
    else
        removeHeader("Content-Length");
}

QByteArray HttpResponse::getRawData() const
{
    // <protocol> <status-code> <status-text>
    QByteArray result = _protocol.toLatin1() + " " + QByteArray::number(_status) + " " + getStringFromStatus(_status).toLatin1();
    result += "\r\n";

    // Headers
    for (auto it = _headers.cbegin(); it != _headers.cend(); ++it)
    {
        result += it.key().toLatin1() + ":" + it.value().toLatin1();
        result += "\r\n";
    }

    result += "\r\n";
    result += _body;

    return result;
}

QHash<HttpResponse::STATUS, QString> HttpResponse::initStatusTexts()
{
    QHash<STATUS, QString> result;

    result.insert(OK,                       "OK");

    result.insert(BAD_REQUEST,              "Bad Request");
    result.insert(UNAUTHORIZED,             "Unauthorized");
    result.insert(FORBIDDEN,                "Forbidden");
    result.insert(NOT_FOUND,                "Not Found");

    result.insert(INTERNAL_SERVER_ERROR,    "Internal Server Error");

    return result;
}
