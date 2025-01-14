#include "httpserver.h"
#include <QFile>


HttpServer::HttpServer(QHostAddress address, quint16 port, QObject* parent) :
#ifdef INHERIT_QTCPSERVER
    QTcpServer(parent),
#else
    QObject(parent),
#endif
    _listenAddress(address),
    _listenPort(port)
{

}

HttpServer::~HttpServer()
{
    if (_server)
    {
        _server->close();
        _server->deleteLater();
        _server = nullptr;
    }
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

void HttpServer::setCallback(HttpRequest::METHOD method, const QString& target, const std::function<HttpResponse (const HttpRequest &)> &function)
{
    _hashCallbacks.insert(qMakePair(method, target), function);
}

void HttpServer::removeCallback(HttpRequest::METHOD method, const QString &target)
{
    _hashCallbacks.remove(qMakePair(method, target));
}

void HttpServer::start()
{
#ifdef INHERIT_QTCPSERVER
    this->listen(_listenAddress, _listenPort);
#else
    if (_sslConfig.isNull())
        _server = new QTcpServer(this);
    else
    {
        QSslServer* sslServer = new QSslServer(this);
        sslServer->setSslConfiguration(_sslConfig);

        _server = sslServer;

        connect(sslServer, &QSslServer::errorOccurred, [](QSslSocket* socket, QAbstractSocket::SocketError error) {
            const QString logName = socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());
            qDebug() << logName << "Server Error:" << error << socket->errorString();
        });
    }

    connect(_server, &QTcpServer::newConnection, this, &HttpServer::clientConnected);

    _server->listen(_listenAddress, _listenPort);
#endif
}

void HttpServer::clientConnected()
{
    while (_server->hasPendingConnections())
    {
        QTcpSocket* socket = _server->nextPendingConnection();

        if (!socket)
        {
            qDebug() << "Socket invalid!";
            continue;
        }

        const QString logName = socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());

        qDebug() << logName << "Connection opened";

        connect(socket, &QTcpSocket::readyRead, this, &HttpServer::dataReceived);
        connect(socket, &QTcpSocket::disconnected, socket, &QSslSocket::deleteLater);
        connect(socket, &QTcpSocket::disconnected, [logName]() { qDebug() << logName << "Disconnected"; });
        connect(socket, &QTcpSocket::destroyed, [logName]() { qDebug() << logName << "Destroyed"; });
        connect(socket, &QSslSocket::errorOccurred, [logName, socket](QAbstractSocket::SocketError error) { qDebug() << logName << "Socket Error:" << error << socket->errorString(); });

        QSslSocket* sslSocket = qobject_cast<QSslSocket*>(socket);

        if (sslSocket)
        {
            qDebug() << logName << "Connection is SSL";

            connect(sslSocket, &QSslSocket::encrypted, [logName]() { qDebug() << logName << "Encrypted"; });
            connect(sslSocket, &QSslSocket::handshakeInterruptedOnError, [logName](const QSslError& error) { qDebug() << logName << "Handhshake Error:" << error; });
            connect(sslSocket, &QSslSocket::peerVerifyError, [logName](const QSslError& error) { qDebug() << logName << "Peer Verify Error:" << error; });
            connect(sslSocket, &QSslSocket::sslErrors, [logName](const QList<QSslError>& errors) { qDebug() << logName << "SSL Errors:" << errors; });
        }
    }
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

#ifdef INHERIT_QTCPSERVER
void HttpServer::incomingConnection(qintptr handle)
{
    QSslSocket* socket = new QSslSocket(this);
    socket->setSocketDescriptor(handle);
    socket->setSslConfiguration(_sslConfig);

    const QString logName = socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());

    qDebug() << logName << "Connection opened";

    connect(socket, &QSslSocket::readyRead, this, &HttpServer::dataReceived);
    connect(socket, &QSslSocket::disconnected, socket, &QSslSocket::deleteLater);
    connect(socket, &QSslSocket::disconnected, [logName]() { qDebug() << logName << "Disconnected"; });
    connect(socket, &QSslSocket::destroyed, [logName]() { qDebug() << logName << "Destroyed"; });
    connect(socket, &QSslSocket::errorOccurred, [logName, socket](QAbstractSocket::SocketError error) { qDebug() << logName << "Socket Error:" << error << socket->errorString(); });
    connect(socket, &QSslSocket::encrypted, [logName]() { qDebug() << logName << "Encrypted"; });
    connect(socket, &QSslSocket::handshakeInterruptedOnError, [logName](const QSslError& error) { qDebug() << logName << "Handhshake Error:" << error; });
    connect(socket, &QSslSocket::peerVerifyError, [logName](const QSslError& error) { qDebug() << logName << "Peer Verify Error:" << error; });
    connect(socket, &QSslSocket::sslErrors, [logName](const QList<QSslError>& errors) { qDebug() << logName << "SSL Errors:" << errors; });

    socket->startServerEncryption();
    socket->waitForEncrypted();

    // TODO: addPendingConnection?
}
#endif
