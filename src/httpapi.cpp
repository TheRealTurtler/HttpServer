#include "httpapi.h"

#include <QJsonDocument>
#include <QFile>


HttpAPI::HttpAPI(QObject* parent) :
    QObject(parent)
{
    _server = new HttpServer(QHostAddress::Any, 8080, this);

    QFile fileCert("certs/certificate.crt");
    QFile fileKey("certs/privatekey.key");

    QSslCertificate sslCert;
    QSslKey sslKey;

    if (fileCert.open(QFile::ReadOnly))
        sslCert = QSslCertificate(fileCert.readAll(), QSsl::Pem);
    else
        qDebug() << "Cannot open Certificate!";

    if (fileKey.open(QFile::ReadOnly))
        sslKey = QSslKey(fileKey.readAll(), QSsl::Rsa, QSsl::Pem);
    else
        qDebug() << "Cannot open PrivateKey!";

    _server->setSslConfig(sslCert, sslKey, QSsl::TlsV1_2OrLater);
    _server->setEnableHttpRedirection(true);

    auto cbGet = [this](const HttpRequest& request, const QString& logInfo) { return cbGET(request, logInfo); };
    auto cbPost = [this](const HttpRequest& request, const QString& logInfo) { return cbPOST(request, logInfo); };

    QStringList targetsGET;
    targetsGET << "/";
    targetsGET << "/ping";
    targetsGET << "/test";

    QStringList targetsPOST;
    targetsPOST << "/";
    targetsPOST << "/echo";
    targetsPOST << "/test";

    for (const auto& itTarget : targetsGET)
    {
        _server->setCallback(HttpRequest::GET, itTarget, cbGet);
    }

    for (const auto& itTarget : targetsPOST)
    {
        _server->setCallback(HttpRequest::POST, itTarget, cbPost);
    }
}

void HttpAPI::start()
{
    _server->start();
}

HttpResponse HttpAPI::cbGET(const HttpRequest& request, const QString& logInfo)
{
    HttpResponse response;

    const QString& target = request.getTarget();

    if (target == "/")
        response = cbHome(request, logInfo);
    else if (target == "/ping")
        response = cbPing(request, logInfo);
    else if (target == "/test")
        response = cbTestGET(request, logInfo);
    else
        response.setStatus(HttpResponse::NOT_FOUND);

    return response;
}

HttpResponse HttpAPI::cbPOST(const HttpRequest &request, const QString& logInfo)
{
    HttpResponse response;

    const QString& target = request.getTarget();

    if (target == "/")
        response = cbHome(request, logInfo);
    else if (target == "/echo")
        response = cbEcho(request, logInfo);
    else if (target == "/test")
        response = cbTestPOST(request, logInfo);
    else
        response.setStatus(HttpResponse::NOT_FOUND);

    return response;
}

HttpResponse HttpAPI::cbHome(const HttpRequest &request, const QString& logInfo)
{
    QString strBody = "Home";

    HttpResponse response;
    response.setStatus(HttpResponse::OK);
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody(strBody.toUtf8());

    return response;
}

HttpResponse HttpAPI::cbPing(const HttpRequest &request, const QString& logInfo)
{
    QString strBody = "Pong";

    HttpResponse response;
    response.setStatus(HttpResponse::OK);
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody(strBody.toUtf8());

    return response;
}

HttpResponse HttpAPI::cbEcho(const HttpRequest& request, const QString& logInfo)
{
    HttpResponse response;
    response.setStatus(HttpResponse::OK);
    response.setHeader("Contet-Type", request.getHeader("Content-Type"));
    response.setBody(request.getBody());

    return response;
}

HttpResponse HttpAPI::cbTestGET(const HttpRequest& request, const QString& logInfo)
{
    QString strBody = "GET Test OK";

    HttpResponse response;
    response.setStatus(HttpResponse::OK);
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody(strBody.toUtf8());

    return response;
}

HttpResponse HttpAPI::cbTestPOST(const HttpRequest &request, const QString& logInfo)
{
    HttpResponse response;

    const QJsonDocument json = QJsonDocument::fromJson(request.getBody());

    if (json.isNull())
    {
        qDebug() << logInfo << "JSON Request is invalid!";
        response.setStatus(HttpResponse::BAD_REQUEST);
        return response;
    }

    const QJsonValue valFunction = json["function"];

    if (valFunction == QJsonValue::Undefined || !valFunction.isString())
    {
        qDebug() << logInfo << "JSON Function is invalid!";
        response.setStatus(HttpResponse::BAD_REQUEST);
        return response;
    }

    const QString strFunction = valFunction.toString();

    if (strFunction == "hello")
    {
        QString strBody = "Hello World!";

        response.setStatus(HttpResponse::OK);
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody(strBody.toUtf8());
    }
    else if (strFunction == "echo")
    {
        response.setStatus(HttpResponse::OK);
        response.setHeader("Content-Type", request.getHeader("Content-Type"));
        response.setBody(request.getBody());
    }
    else if (strFunction == "secret")
    {
        const QJsonValue valPassword = json["password"];

        if (valPassword == QJsonValue::Undefined || !valPassword.isString())
        {
            response.setStatus(HttpResponse::UNAUTHORIZED);
            return response;
        }

        if (valPassword.toString() == "asdf1234")
        {
            QString strBody = "Password correct!";

            response.setStatus(HttpResponse::OK);
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody(strBody.toUtf8());
        }
        else
            response.setStatus(HttpResponse::UNAUTHORIZED);
    }
    else
        response.setStatus(HttpResponse::BAD_REQUEST);

    return response;
}
