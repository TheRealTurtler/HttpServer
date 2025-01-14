#include "httprequest.h"


const QHash<HttpRequest::METHOD, QString> HttpRequest::_methodTexts = initMethodTexts();


HttpRequest::HttpRequest()
{

}

HttpRequest::HttpRequest(const QByteArray& data) :
    HttpRequest()
{
    setData(data);
}

void HttpRequest::setData(const QByteArray& data)
{
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages#http_requests

    _method = UNKNOWN;
    _target = "";
    _protocol = "";
    _headers.clear();
    _targetParameters.clear();
    _body = "";
    _valid = false;

    QByteArrayList lines = data.split('\n');

    {
        const QString strLine = QString::fromLatin1(lines[0]);
        const QStringList list = strLine.split(' ');

        if (list.size() < 3)
            return;

        _method = getMethodFromString(list[0]);

        QString path = list[1].trimmed();

        if (_method == GET && path.contains('?'))
        {
            // Syntax: /target?par1=val1&par2=val2

            const auto idx = path.indexOf('?');
            _target = path.mid(0, idx);

            const QStringList listPar = path.mid(idx + 1).split("&");

            for (const auto& parPair : listPar)
            {
                const QStringList par = parPair.split('=', Qt::KeepEmptyParts);

                if (par.size() == 2 && par.at(0) != "")
                    _targetParameters.insert(par.at(0), par.at(1));
            }
        }
        else
            _target = path;

        _protocol = list[2].trimmed();
    }

    if (_method != UNKNOWN)
        _valid = true;

    lines.pop_front();

    bool isHeader = true;

    for (const auto& line : lines)
    {
        if (isHeader)
        {
            if (line.trimmed() == "")
                isHeader = false;
            else
            {
                const QStringList list = QString::fromLatin1(line).split(':');

                if (list.size() < 2)
                {
                    _valid = false;
                    return;
                }

                const QString key = list [0].trimmed();
                const QString value = list[1].trimmed();

                if (key != "" && value != "")
                    _headers.insert(key, value);
            }
        }
        else
        {
            if (!_body.isEmpty())
                _body.append("\r\n");

            _body.append(line.trimmed());
        }
    }
}

QHash<HttpRequest::METHOD, QString> HttpRequest::initMethodTexts()
{
    QHash<METHOD, QString> result;

    result.insert(GET,      "GET");
    result.insert(POST,     "POST");
    result.insert(PUT,      "PUT");
    result.insert(DELETE,   "DELETE");

    return result;
}
