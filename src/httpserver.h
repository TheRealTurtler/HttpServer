#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QSslServer>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>
#include <functional>

#include "httprequest.h"
#include "httpresponse.h"


class HttpServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(QHostAddress address, quint16 port, QObject* parent = nullptr);

    void setSslConfig(const QSslConfiguration& sslConf);
    void setSslConfig(const QSslCertificate& sslCert, const QSslKey& sslKey, QSsl::SslProtocol sslProtocol);
    void setSslConfig(const QString& sslCertFile, const QString& sslKeyFile, QSsl::SslProtocol sslProtocol);

    void setCallback(HttpRequest::METHOD method, const QString& target, std::function<HttpResponse(const HttpRequest&)> function);
    void removeCallback(HttpRequest::METHOD method, const QString& target);

public slots:
    void start();

private slots:
    void connectionPending();
    void dataReceived();

private:
    QTcpServer* _server = nullptr;

    QHostAddress _listenAddress;
    quint16 _listenPort;

    QSslConfiguration _sslConfig = QSslConfiguration();

    QHash<QPair<HttpRequest::METHOD, QString>, std::function<HttpResponse(const HttpRequest&)> > _hashCallbacks;
};

#endif // HTTPSERVER_H
