#include "httpserver.h"
#include <QFile>


const quint8 VERSION_MAJOR = 1;
const quint8 VERSION_MINOR = 0;
const quint8 VERSION_FIX = 0;


HttpServer::HttpServer(const QHostAddress &address, quint16 port, QObject* parent) :
    QTcpServer(parent),
    m_listenAddress(address),
    m_listenPort(port)
{
    connect(this, &QTcpServer::newConnection, this, &HttpServer::clientConnected);
}

HttpServer::~HttpServer()
{
    stop();
}

QString HttpServer::getServerName()
{
    return "TRT";
}

QString HttpServer::getServerVersion()
{
    return QString::number(VERSION_MAJOR) + "." + QString::number(VERSION_MINOR) + "." + QString::number(VERSION_FIX);
}

void HttpServer::setSslConfig(const QSslConfiguration& sslConf)
{
    m_sslConfig = sslConf;
}

void HttpServer::setSslConfig(const QSslCertificate& sslCert, const QSslKey& sslKey, QSsl::SslProtocol sslProtocol)
{
    QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
    sslConf.setLocalCertificate(sslCert);
    sslConf.setPrivateKey(sslKey);
    sslConf.setProtocol(sslProtocol);

    setSslConfig(sslConf);
}

void HttpServer::setEnableHttp(bool enable)
{
    m_enableHttp = enable;

    if (enable)
        m_enableHttpRedirection = false;
}

void HttpServer::setEnableHttpRedirection(bool enable)
{
    m_enableHttpRedirection = enable;

    if (enable)
        m_enableHttp = false;
}

void HttpServer::setCallback(HttpRequest::METHOD method, const QString& target, const std::function<HttpResponse (const HttpRequest &, const QString&)> &function)
{    
    m_hashCallbacks.insert(qMakePair(method, target), function);
}

void HttpServer::removeCallback(HttpRequest::METHOD method, const QString &target)
{
    m_hashCallbacks.remove(qMakePair(method, target));
}

void HttpServer::start()
{
    if (!this->isListening())
        this->listen(m_listenAddress, m_listenPort);
}

void HttpServer::stop()
{
    if (this->isListening())
        this->close();
}

void HttpServer::clientConnected()
{
    while (this->hasPendingConnections())
    {
        QTcpSocket* const socket = this->nextPendingConnection();

        if (!socket)
        {
            qDebug() << "Socket invalid!";
            continue;
        }

        const QString logInfo = getLogInfo(socket);

        qDebug() << logInfo << "Connected";

        connect(socket, &QTcpSocket::readyRead, this, &HttpServer::dataReceived);
        connect(socket, &QTcpSocket::disconnected, socket, &QSslSocket::deleteLater);

        connect(socket, &QTcpSocket::disconnected, [logInfo]() { qDebug() << logInfo << "Disconnected"; });
        connect(socket, &QTcpSocket::destroyed, [logInfo]() { qDebug() << logInfo << "Destroyed"; });
        connect(socket, &QSslSocket::errorOccurred, [logInfo, socket](QAbstractSocket::SocketError error) { qDebug() << logInfo << "Socket Error:" << error << socket->errorString(); });

        QSslSocket* const sslSocket = qobject_cast<QSslSocket*>(socket);

        if (sslSocket)
        {
            connect(sslSocket, &QSslSocket::encrypted, [logInfo]() { qDebug() << logInfo << "Encrypted"; });
            connect(sslSocket, &QSslSocket::handshakeInterruptedOnError, [logInfo](const QSslError& error) { qDebug() << logInfo << "Handhshake Error:" << error; });
            connect(sslSocket, &QSslSocket::peerVerifyError, [logInfo](const QSslError& error) { qDebug() << logInfo << "Peer Verify Error:" << error; });
            connect(sslSocket, &QSslSocket::sslErrors, [logInfo](const QList<QSslError>& errors) { qDebug() << logInfo << "SSL Errors:" << errors; });
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

    const QString logInfo = getLogInfo(socket);

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

        response.checkHeaders();
        socket->write(response.getRawData());
        socket->close();
    }

    // TLS Handshakes always starts with 22
    // https://datatracker.ietf.org/doc/html/rfc5246
    // -> enum ContentType is 22 for Handshake
    else if (socket->peek(1).startsWith(22))
    {
        qDebug() << logInfo << "TLS Handshake";

        if (m_sslConfig.isNull())
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

        bool writeResponse = false;
        HttpResponse response;

        // Redirect HTTP to HTTPS
        if (m_enableHttpRedirection)
        {
            qDebug() << logInfo << "Redirecting HTTP to HTTPS";

            if (m_sslConfig.isNull())
                qWarning() << logInfo << "SSL Config is invalid!";
            else
            {
                const QString host = request.getHeader("Host");

                if (host != "")
                {
                    response.setStatus(HttpResponse::MOVED_PERMANENTLY);
                    response.setHeader("Location", "https://" + host + request.getTargetRaw());
                    response.setHeader("Content-Type", "text/plain; charset=utf-8");

                    QString strBody = "Permanently Moved to https://" + host + request.getTarget();
                    response.setBody(strBody.toUtf8());
                }
                else
                    response.setStatus(HttpResponse::BAD_REQUEST);

                writeResponse = true;
            }
        }

        // Handle HTTP Request
        else if (m_enableHttp)
        {
            qDebug() << logInfo << "Handling HTTP Request";

            response = handleHttpRequest(request, logInfo);
            writeResponse = true;
        }

        if (writeResponse)
        {
            if (response.getStatus() != HttpResponse::OK && response.getBody() == "")
            {
                const QString strBody = QString::number(response.getStatus()) + " " + response.getStringFromStatus(response.getStatus());

                response.setHeader("Content-Type", "text/plain; charset=utf-8");
                response.setBody(strBody.toUtf8());
            }

            response.checkHeaders();
            socket->write(response.getRawData());
        }

        socket->close();
    }
}

QString HttpServer::getLogInfo(QTcpSocket* const socket)
{
    QString result = "";

    if (socket)
        result = QString::number(socket->localPort()) + ":" + socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());

    return result;
}

void HttpServer::incomingConnection(qintptr handle)
{
    QSslSocket* const socket = new QSslSocket(this);

    if (!socket->setSocketDescriptor(handle))
    {
        qDebug() << "SocketDescriptor invalid!";
        socket->deleteLater();
        return;
    }

    const QString logInfo = getLogInfo(socket);

    qDebug() << logInfo << "Connection opened";

    if (!m_sslConfig.isNull())
        socket->setSslConfiguration(m_sslConfig);

    this->addPendingConnection(socket);
}

HttpResponse HttpServer::handleHttpRequest(const HttpRequest& request, const QString &logInfo)
{
    HttpResponse response;

    qDebug() << logInfo << "Method:" << request.getMethod() << HttpRequest::getStringFromMethod(request.getMethod()) << "Target:" << request.getTarget();
    qDebug() << logInfo << "Parameters:" << request.getTargetParameters();
    qDebug() << logInfo << "Data:" << request.getBody();

    if (request.isValid())
    {
        const auto cbKey = qMakePair(request.getMethod(), request.getTarget());

        if (m_hashCallbacks.contains(cbKey))
        {
            qDebug() << logInfo << "Callback found";

            const auto& cb = m_hashCallbacks[cbKey];
            response = cb(request, logInfo);
        }
        else
        {
            qDebug() << logInfo << "Request valid, but no Callback set";
            response.setStatus(HttpResponse::NOT_FOUND);
        }
    }
    else
    {
        qDebug() << logInfo << "Request invalid!";
        response.setStatus(HttpResponse::BAD_REQUEST);
    }

    return response;
}
