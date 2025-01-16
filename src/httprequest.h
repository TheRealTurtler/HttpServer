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

    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
    enum METHOD
    {
        UNKNOWN = -1,

        GET,
        HEAD,
        POST,
        PUT,
        DELETE,
        CONNECT,
        OPTIONS,
        TRACE,
        PATCH,
    };

    static QString getStringFromMethod(METHOD method) { return m_methodTexts.value(method, ""); }
    static METHOD getMethodFromString(const QString& strMethod) { return m_methodTexts.key(strMethod, UNKNOWN); }

    METHOD getMethod() const { return m_method; }
    const QString& getTarget() const { return m_target; }
    const QString& getTargetRaw() const { return m_targetRaw; }
    const QString& getProtocol() const { return m_protocol; }

    // https://developer.mozilla.org/en-US/docs/Glossary/Request_header
    const QMultiHash<QString, QString>& getHeaders() const { return m_headers; }
    QString getHeader(const QString& key, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

    const QMultiHash<QString, QString>& getTargetParameters() const { return m_targetParameters; }
    QString getTargetParameter(const QString& key) const { return m_targetParameters.value(key, ""); }

    const QByteArray& getBody() const { return m_body; }

    bool isValid() const { return m_valid; }

private:
    static const QHash<METHOD, QString> m_methodTexts;
    static QHash<METHOD, QString> initMethodTexts();

    METHOD m_method = UNKNOWN;
    QString m_target;
    QString m_targetRaw;
    QString m_protocol;

    QMultiHash<QString, QString> m_headers;
    QMultiHash<QString, QString> m_targetParameters;

    QByteArray m_body;

    bool m_valid = false;
};

#endif // HTTPREQUEST_H
