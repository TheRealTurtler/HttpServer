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

    if (!fileCert.open(QFile::ReadOnly) || !fileKey.open(QFile::ReadOnly))
    {
        qDebug() << "Unable to open SSL certificate!";
        return;
    }

    QSslCertificate sslCert = QSslCertificate(fileCert.readAll(), QSsl::Pem);
    QSslKey sslKey = QSslKey(fileKey.readAll(), QSsl::Rsa, QSsl::Pem);

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

        connect(_server, &QSslServer::acceptError, this, &HttpServer::acceptError);
    }

    connect(_server, &QTcpServer::newConnection, this, &HttpServer::connectionPending);

    _server->listen(_listenAddress, _listenPort);
}

void HttpServer::connectionPending()
{
    while (_server->hasPendingConnections())
    {
        QTcpSocket* socket = _server->nextPendingConnection();

        if (!socket)
        {
            qDebug() << "Socket invalid!";
            continue;
        }

        qDebug() << socket->peerAddress().toString() << socket->peerPort() << "Connection opened";

        connect(socket, &QTcpSocket::readyRead, this, &HttpServer::dataReceived);
        connect(socket, &QTcpSocket::aboutToClose, this, &HttpServer::connectionClosed);
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

        connect(socket, &QTcpSocket::destroyed, this, &HttpServer::destroyed);

        QSslSocket* sslSocket = qobject_cast<QSslSocket*>(socket);

        if (sslSocket)
        {
            qDebug() << socket->peerAddress().toString() << socket->peerPort() << "Connection is SSL";

            connect(sslSocket, &QSslSocket::handshakeInterruptedOnError, this, &HttpServer::handshakeInterruptedOnError);
            connect(sslSocket, &QSslSocket::peerVerifyError, this, &HttpServer::peerVerifyError);
            connect(sslSocket, &QSslSocket::sslErrors, this, &HttpServer::sslErrors);
        }
    }
}

void HttpServer::connectionClosed()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

    if (!socket)
        return;

    qDebug() << socket->peerAddress().toString() << socket->peerPort() << "Connection closed";
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

    qDebug() << socket->peerAddress().toString() << socket->peerPort() << "Method:" << request.getMethod() << "Target:" << request.getTarget() << "Data:" << request.getBody();

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
    socket->close();
}

void HttpServer::errorOccurred(QSslSocket *socket, QAbstractSocket::SocketError socketError)
{
    qDebug() << socket->peerAddress() << socket->peerPort() << socketError;
}

void HttpServer::acceptError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "Socket Error:" << socketError;
}

void HttpServer::handshakeInterruptedOnError(const QSslError& error)
{
    qDebug() << "Handshake interrupted:" << error;
}

void HttpServer::peerVerifyError(const QSslError& error)
{
    qDebug() << "Peer verify:" << error;
}

void HttpServer::sslErrors(const QList<QSslError>& errors)
{
    qDebug() << "SSL errors:" << errors;
}

void HttpServer::destroyed()
{
    QObject* socket = qobject_cast<QObject*>(sender());

    if (!socket)
        return;

    qDebug() << "Destroyed";
}
