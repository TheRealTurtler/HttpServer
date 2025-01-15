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
        _targetRaw = list[1].trimmed();

        if (_method == GET && _targetRaw.contains('?'))
        {
            // Syntax: /target?par1=val1&par2=val2

            const auto idx = _targetRaw.indexOf('?');
            _target = _targetRaw.mid(0, idx);

            const QStringList listPar = _targetRaw.mid(idx + 1).split("&");

            for (const auto& parPair : listPar)
            {
                const QStringList par = parPair.split('=', Qt::KeepEmptyParts);

                if (par.size() == 2 && par.at(0) != "")
                    _targetParameters.insert(par.at(0), par.at(1));
            }
        }
        else
            _target = _targetRaw;

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
                const QString header = QString::fromLatin1(line.trimmed());
                const auto idx = header.indexOf(':');

                if (idx > 0)
                {
                    const QString key = header.mid(0, idx).trimmed();
                    const QString value = header.mid(idx + 1).trimmed();

                    if (key != "" && value != "")
                        _headers.insert(key, value);
                }
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
