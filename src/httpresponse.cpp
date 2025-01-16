#include "httpresponse.h"

#include <QDateTime>

#include "httpserver.h"


const QHash<HttpResponse::STATUS, QString> HttpResponse::m_statusTexts = initStatusTexts();


HttpResponse::HttpResponse() :
    HttpResponse(INTERNAL_SERVER_ERROR)
{

}

HttpResponse::HttpResponse(STATUS status) :
    m_status(status)
{

}

void HttpResponse::setBody(const QByteArray &body)
{
    m_body = body;

    if (m_body.size() > 0)
        setHeader("Content-Length", QString::number(m_body.size()));
    else
        removeHeader("Content-Length");
}

QByteArray HttpResponse::getRawData() const
{
    // <protocol> <status-code> <status-text>
    QByteArray result = m_protocol.toLatin1() + " " + QByteArray::number(m_status) + " " + getStringFromStatus(m_status).toLatin1();
    result += "\r\n";

    // Headers
    for (auto it = m_headers.cbegin(); it != m_headers.cend(); ++it)
    {
        result += it.key().toLatin1() + ": " + it.value().toLatin1();
        result += "\r\n";
    }

    result += "\r\n";
    result += m_body;

    return result;
}

void HttpResponse::checkHeaders()
{
    const QStringList listHeaders = m_headers.keys();

    if (!listHeaders.contains("Date", Qt::CaseInsensitive))
        setHeader("Date", QDateTime::currentDateTimeUtc().toString("ddd, dd MMM yyyy hh:mm:ss") + " GMT");

    if (!listHeaders.contains("Server", Qt::CaseInsensitive))
        setHeader("Server", HttpServer::getServerName() + "/" + HttpServer::getServerVersion());

    if (!listHeaders.contains("Connection", Qt::CaseInsensitive))
        setHeader("Connection", "close");       // TODO: Persistent Connections for HTTP/1.1 with Connection: keep-alive

    if (m_status == METHOD_NOT_ALLOWED)
    {
        if (!listHeaders.contains("Allow", Qt::CaseInsensitive))
            setHeader("Allow", "GET");
    }
    else if (m_status == UNAUTHORIZED)
    {
        // TODO
        // if (!listHeaders.contains("WWW-Authenticate"))
            // setHeader("WWW-Authenticate", "");
    }

    // TODO: Other Headers?
}

QHash<HttpResponse::STATUS, QString> HttpResponse::initStatusTexts()
{
    QHash<STATUS, QString> result;

    result.insert(OK,                       "OK");

    result.insert(MOVED_PERMANENTLY,        "Moved Permanently");

    result.insert(BAD_REQUEST,              "Bad Request");
    result.insert(UNAUTHORIZED,             "Unauthorized");
    result.insert(FORBIDDEN,                "Forbidden");
    result.insert(NOT_FOUND,                "Not Found");
    result.insert(METHOD_NOT_ALLOWED,       "Method Not Allowed");

    result.insert(INTERNAL_SERVER_ERROR,    "Internal Server Error");

    return result;
}
