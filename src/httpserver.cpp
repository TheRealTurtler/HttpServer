#include "httpserver.h"
#include <QFile>


HttpServer::HttpServer(QHostAddress address, quint16 port, QObject* parent) :
    QObject(parent),
    _listenAddress(address),
    _listenPort(port)
{

}

void HttpServer::setSslConfig(const QSslConfiguration& sslConf)
{
    _sslConfig = sslConf;
}

void HttpServer::setSslConfig(const QSslCertificate& sslCert, const QSslKey& sslKey, QSsl::SslProtocol sslProtocol)
{
    QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
    sslConf.setLocalCertificate(sslCert);
    sslConf.setPrivateKey(sslKey);
    sslConf.setProtocol(sslProtocol);

    setSslConfig(sslConf);
}

void HttpServer::setSslConfig(const QString& sslCertFile, const QString& sslKeyFile, QSsl::SslProtocol sslProtocol)
{
    QFile fileCert(sslCertFile);
    QFile fileKey(sslKeyFile);

    QSslCertificate sslCert = QSslCertificate(&fileCert, QSsl::Pem);
    QSslKey sslKey = QSslKey(&fileKey, QSsl::Rsa, QSsl::Pem);

    setSslConfig(sslCert, sslKey, sslProtocol);
}

void HttpServer::setCallback(HttpRequest::METHOD method, const QString& target, std::function<HttpResponse (const HttpRequest&)> function)
{
    _hashCallbacks.insert(qMakePair(method, target), function);
}

void HttpServer::removeCallback(HttpRequest::METHOD method, const QString &target)
{
    _hashCallbacks.remove(qMakePair(method, target));
}

void HttpServer::start()
{
    if (_sslConfig.isNull())
        _server = new QTcpServer(this);
    else
    {
        QSslServer* sslServer = new QSslServer(this);
        sslServer->setSslConfiguration(_sslConfig);

        _server = sslServer;
    }

    connect(_server, &QTcpServer::pendingConnectionAvailable, this, &HttpServer::connectionPending);

    _server->listen(_listenAddress, _listenPort);
}

void HttpServer::connectionPending()
{
    QTcpSocket* socket = _server->nextPendingConnection();

    if (!socket)
    {
        qDebug() << "Socket invalid!";
        return;
    }

    qDebug() << "New Connection from IP:" << socket->peerAddress() << "Port:" << socket->peerPort();

    connect(socket, &QTcpSocket::readyRead, this, &HttpServer::dataReceived);
}

void HttpServer::dataReceived()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

    if (!socket)
    {
        qDebug() << "Socket invalid!";
        return;
    }

    HttpRequest request(socket->readAll());
    HttpResponse response;

    qDebug() << socket->peerAddress() << socket->peerPort() << "Method:" << request.getMethod() << "Target:" << request.getTarget() << "Data:" << request.getBody();

    if (request.isValid())
    {
        const auto cbKey = qMakePair(request.getMethod(), request.getTarget());

        if (_hashCallbacks.contains(cbKey))
        {
            const auto& cb = _hashCallbacks[cbKey];
            response = cb(request);
        }
        else
            response.setStatus(HttpResponse::NOT_FOUND);
    }
    else
        response.setStatus(HttpResponse::BAD_REQUEST);

    if (response.getStatus() != HttpResponse::OK && response.getBody() == "")
    {
        QString strBody = QString::number(response.getStatus()) + " " + response.getStringFromStatus(response.getStatus());

        response.addHeader("Content-Type", "text/html; charset=utf-8");
        response.setBody(strBody.toUtf8());
    }

    socket->write(response.getRawData());
    socket->flush();

    qDebug() << "Closing Connection to IP:" << socket->peerAddress() << "Port:" << socket->peerPort();

    socket->close();
    socket->deleteLater();
}
