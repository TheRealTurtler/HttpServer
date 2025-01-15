#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QByteArray>
#include <QMultiHash>


class HttpRequest
{
public:
    HttpRequest();
    HttpRequest(const QByteArray& data);

    void setData(const QByteArray& data);

    enum METHOD
    {
        UNKNOWN = -1,

        GET,
        POST,
        PUT,
        DELETE,
    };

    static QString getStringFromMethod(METHOD method) { return _methodTexts.value(method, ""); }
    static METHOD getMethodFromString(const QString& strMethod) { return _methodTexts.key(strMethod, UNKNOWN); }

    METHOD getMethod() const { return _method; }
    const QString& getTarget() const { return _target; }
    const QString& getTargetRaw() const { return _targetRaw; }
    const QString& getProtocol() const { return _protocol; }

    const QMultiHash<QString, QString>& getHeaders() const { return _headers; }
    QString getHeader(const QString& key) const { return _headers.value(key, ""); }

    const QMultiHash<QString, QString>& getTargetParameters() const { return _targetParameters; }
    QString getTargetParameter(const QString& key) const { return _targetParameters.value(key, ""); }

    const QByteArray& getBody() const { return _body; }

    bool isValid() const { return _valid; }

private:
    static const QHash<METHOD, QString> _methodTexts;
    static QHash<METHOD, QString> initMethodTexts();

    METHOD _method = UNKNOWN;
    QString _target;
    QString _targetRaw;
    QString _protocol;

    QMultiHash<QString, QString> _headers;
    QMultiHash<QString, QString> _targetParameters;

    QByteArray _body;

    bool _valid = false;
};

#endif // HTTPREQUEST_H
