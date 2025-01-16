#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QSslConfiguration>
#include <functional>

#include "httprequest.h"
#include "httpresponse.h"


class HttpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit HttpServer(const QHostAddress& address, quint16 port, QObject* parent = nullptr);
    ~HttpServer();

    static QString getServerName();
    static QString getServerVersion();

    void setSslConfig(const QSslConfiguration& sslConf);
    void setSslConfig(const QSslCertificate& sslCert, const QSslKey& sslKey, QSsl::SslProtocol sslProtocol);

    void setEnableHttp(bool enable);
    void setEnableHttpRedirection(bool enable);

    void setCallback(HttpRequest::METHOD method, const QString& target, const std::function<HttpResponse(const HttpRequest&, const QString&)>& function);
    void removeCallback(HttpRequest::METHOD method, const QString& target);

public slots:
    void start();
    void stop();

private slots:
    void clientConnected();
    void dataReceived();

protected:
    virtual void incomingConnection(qintptr handle);

private:
    QHostAddress m_listenAddress;
    quint16 m_listenPort;

    QSslConfiguration m_sslConfig = QSslConfiguration();

    bool m_enableHttp = true;
    bool m_enableHttpRedirection = false;

    QHash<QPair<HttpRequest::METHOD, QString>, std::function<HttpResponse(const HttpRequest&, const QString&)> > m_hashCallbacks;

    static QString getLogInfo(QTcpSocket* const socket);

    HttpResponse handleHttpRequest(const HttpRequest& request, const QString& logInfo);
};

#endif // HTTPSERVER_H
