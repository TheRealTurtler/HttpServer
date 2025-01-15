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

void HttpServer::setCallback(HttpRequest::METHOD method, const QString& target, const std::function<HttpResponse (const HttpRequest &, const QString&)> &function)
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
    QSslSocket* const socket = qobject_cast<QSslSocket*>(sender());

    if (!socket)
    {
        qDebug() << "Socket invalid!";
        return;
    }

    const QString logInfo = socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());

    // Socket is encrypted -> Handle HTTPS request
    if (socket->isEncrypted())
    {
        qDebug() << logInfo << "Encrypted -> HTTPS";

        const HttpRequest request(socket->readAll());
        HttpResponse response = handleHttpRequest(request, logInfo);

        if (response.getStatus() != HttpResponse::OK && response.getBody() == "")
        {
            QString strBody = QString::number(response.getStatus()) + " " + response.getStringFromStatus(response.getStatus());

            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody(strBody.toUtf8());
        }

        socket->write(response.getRawData());
        socket->close();
    }

    // TLS Handshakes always starts with 22
    // https://datatracker.ietf.org/doc/html/rfc5246
    // -> enum ContentType is 22 for Handshake
    else if (socket->peek(1).startsWith(22))
    {
        qDebug() << logInfo << "TLS Handshake";

        if (_sslConfig.isNull())
        {
            qWarning() << logInfo << "SSL Config is invalid!";
            socket->close();
        }
        else
            socket->startServerEncryption();
    }

    // Socket is not encrypted and no TLS Handshake -> Handle HTTP request
    else
    {
        qDebug() << logInfo << "Unencrypted -> HTTP";

        const HttpRequest request(socket->readAll());
        HttpResponse response;

        if (_enableHttpRedirection)
        {
            qDebug() << logInfo << "Redirecting HTTP to HTTPS";

            if (_sslConfig.isNull())
                qWarning() << logInfo << "SSL Config is invalid!";      // TODO: Close socket instead of sending a 500 Error
            else
            {
                const QString host = request.getHeader("Host");

                if (host != "")
                {
                    response.setStatus(HttpResponse::MOVED_PERMANENTLY);
                    response.setHeader("Location", "https://" + host + request.getTargetRaw());
                    response.setHeader("Content-Type", "text/plain; charset=utf-8");

                    QString strBody = QString::number(response.getStatus()) + " " + HttpResponse::getStringFromStatus(response.getStatus()) + " to " + "https://" + host + request.getTarget();
                    response.setBody(strBody.toUtf8());
                }
                else
                    response.setStatus(HttpResponse::BAD_REQUEST);
            }
        }

        // TODO: HTTPS-only-Mode
        // -> _enableHttp
        else
            response = handleHttpRequest(request, logInfo);

        if (response.getStatus() != HttpResponse::OK && response.getBody() == "")
        {
            QString strBody = QString::number(response.getStatus()) + " " + response.getStringFromStatus(response.getStatus());

            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody(strBody.toUtf8());
        }

        socket->write(response.getRawData());
        socket->close();
    }
}

#ifdef INHERIT_QTCPSERVER
void HttpServer::incomingConnection(qintptr handle)
{
    QSslSocket* socket = new QSslSocket(this);

    if (!socket->setSocketDescriptor(handle))
    {
        qDebug() << "SocketDescriptor invalid!";
        socket->deleteLater();
        return;
    }

    const QString logInfo = socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());

    qDebug() << logInfo << "Connection opened";

    connect(socket, &QSslSocket::readyRead, this, &HttpServer::dataReceived);
    connect(socket, &QSslSocket::disconnected, socket, &QSslSocket::deleteLater);
    connect(socket, &QSslSocket::disconnected, [logInfo]() { qDebug() << logInfo << "Disconnected"; });
    connect(socket, &QSslSocket::destroyed, [logInfo]() { qDebug() << logInfo << "Destroyed"; });

    connect(socket, &QSslSocket::errorOccurred, [logInfo, socket](QAbstractSocket::SocketError error) { qDebug() << logInfo << "Socket Error:" << error << socket->errorString(); });
    connect(socket, &QSslSocket::encrypted, [logInfo]() { qDebug() << logInfo << "Encrypted"; });
    connect(socket, &QSslSocket::handshakeInterruptedOnError, [logInfo](const QSslError& error) { qDebug() << logInfo << "Handhshake Error:" << error; });
    connect(socket, &QSslSocket::peerVerifyError, [logInfo](const QSslError& error) { qDebug() << logInfo << "Peer Verify Error:" << error; });
    connect(socket, &QSslSocket::sslErrors, [logInfo](const QList<QSslError>& errors) { qDebug() << logInfo << "SSL Errors:" << errors; });

    if (!_sslConfig.isNull())
        socket->setSslConfiguration(_sslConfig);

    // TODO: addPendingConnection?
}

HttpResponse HttpServer::handleHttpRequest(const HttpRequest& request, const QString &logInfo)
{
    HttpResponse response;

    qDebug() << logInfo << "Method:" << request.getMethod() << HttpRequest::getStringFromMethod(request.getMethod()) << "Target:" << request.getTarget() << "Parameters:" << request.getTargetParameters() << "Data:" << request.getBody();

    if (request.isValid())
    {
        for (auto it = request.getHeaders().cbegin(); it != request.getHeaders().cend(); ++it)
        {
            qDebug() << logInfo << "Header:" << it.key() << it.value();
        }

        const auto cbKey = qMakePair(request.getMethod(), request.getTarget());

        if (_hashCallbacks.contains(cbKey))
        {
            const auto& cb = _hashCallbacks[cbKey];
            response = cb(request, logInfo);
        }
        else
            response.setStatus(HttpResponse::NOT_FOUND);
    }
    else
        response.setStatus(HttpResponse::BAD_REQUEST);

    return response;
}
#endif
