#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QByteArray>
#include <QString>
#include <QMultiHash>


class HttpResponse
{
public:
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
    enum STATUS
    {
        OK = 200,

        MOVED_PERMANENTLY = 301,

        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,

        INTERNAL_SERVER_ERROR = 500,
    };

    HttpResponse();
    HttpResponse(STATUS status);

    static QString getStringFromStatus(STATUS status) { return m_statusTexts.value(status, ""); }

    void setStatus(STATUS status) { m_status = status; }
    STATUS getStatus() { return m_status; }

    // https://developer.mozilla.org/en-US/docs/Glossary/Response_header
    void setHeaders(const QMultiHash<QString, QString>& headers) { m_headers = headers; }
    void setHeader(const QString& key, const QString& value) { m_headers[key] = value; }
    void addHeader(const QString& key, const QString& value) { m_headers.insert(key, value); }
    void removeHeader(const QString& key) { m_headers.remove(key); }

    QMultiHash<QString, QString> getHeaders() { return m_headers; }

    void setBody(const QByteArray& body);
    const QByteArray& getBody() { return m_body; }

    QByteArray getRawData() const;

    void checkHeaders();

private:
    static const QHash<STATUS, QString> m_statusTexts;
    static QHash<STATUS, QString> initStatusTexts();

    QString m_protocol = "HTTP/1.0";     // TODO: 1.1 -> Some Headers are mandatory!
    STATUS m_status;

    QMultiHash<QString, QString> m_headers;

    QByteArray m_body;
};

#endif // HTTPRESPONSE_H
