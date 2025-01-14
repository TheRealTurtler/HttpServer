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


#define INHERIT_QTCPSERVER

#ifdef INHERIT_QTCPSERVER
class HttpServer : public QTcpServer
#else
class HttpServer : public QObject
#endif
{
    Q_OBJECT

public:
    explicit HttpServer(QHostAddress address, quint16 port, QObject* parent = nullptr);
    ~HttpServer();

    void setSslConfig(const QSslConfiguration& sslConf);
    void setSslConfig(const QSslCertificate& sslCert, const QSslKey& sslKey, QSsl::SslProtocol sslProtocol);

    void setCallback(HttpRequest::METHOD method, const QString& target, const std::function<HttpResponse(const HttpRequest&)>& function);
    void removeCallback(HttpRequest::METHOD method, const QString& target);

public slots:
    void start();

private slots:
    void clientConnected();
    void dataReceived();

#ifdef INHERIT_QTCPSERVER
protected:
    virtual void incomingConnection(qintptr handle);
#endif

private:
    QTcpServer* _server = nullptr;

    QHostAddress _listenAddress;
    quint16 _listenPort;

    QSslConfiguration _sslConfig = QSslConfiguration();

    QHash<QPair<HttpRequest::METHOD, QString>, std::function<HttpResponse(const HttpRequest&)> > _hashCallbacks;
};

#endif // HTTPSERVER_H
