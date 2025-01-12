#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QByteArray>
#include <QString>
#include <QMultiHash>


class HttpResponse
{
public:
    HttpResponse();

    enum STATUS
    {
        OK = 200,

        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,

        INTERNAL_SERVER_ERROR = 500,
    };

    static QString getStringFromStatus(STATUS status) { return _statusTexts.value(status, ""); }

    void setStatus(STATUS status) { _status = status; }
    STATUS getStatus() { return _status; }

    void setHeaders(const QMultiHash<QString, QString>& headers) { _headers = headers; }
    void setHeader(const QString& key, const QString& value) { _headers[key] = value; }
    void addHeader(const QString& key, const QString& value) { _headers.insert(key, value); }
    void removeHeader(const QString& key) { _headers.remove(key); }

    QMultiHash<QString, QString> getHeaders() { return _headers; }

    void setBody(const QByteArray& body);
    const QByteArray& getBody() { return _body; }

    QByteArray getRawData() const;

private:
    static const QHash<STATUS, QString> _statusTexts;
    static QHash<STATUS, QString> initStatusTexts();

    QString _protocol = "HTTP/1.1";

    STATUS _status = INTERNAL_SERVER_ERROR;

    QMultiHash<QString, QString> _headers;

    QByteArray _body = QByteArray();
};

#endif // HTTPRESPONSE_H
