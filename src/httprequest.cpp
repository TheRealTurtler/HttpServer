#include "httprequest.h"

#include <QUrl>


const QHash<HttpRequest::METHOD, QString> HttpRequest::m_methodTexts = initMethodTexts();


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

    m_method = UNKNOWN;
    m_target = "";
    m_protocol = "";
    m_headers.clear();
    m_targetParameters.clear();
    m_body = "";
    m_valid = false;

    QByteArrayList lines = data.split('\n');

    {
        const QByteArrayList list = lines[0].split(' ');

        if (list.size() < 3)
            return;

        m_method = getMethodFromString(QString::fromLatin1(list[0]).trimmed());
        m_targetRaw = QUrl::fromPercentEncoding(list[1]).trimmed();

        if (m_method == GET && m_targetRaw.contains('?'))
        {
            // Syntax: /target?par1=val1&par2=val2

            const auto idx = m_targetRaw.indexOf('?');
            m_target = m_targetRaw.mid(0, idx);

            const QStringList listPar = m_targetRaw.mid(idx + 1).split('&');

            for (const auto& parPair : listPar)
            {
                const QStringList par = parPair.split('=', Qt::KeepEmptyParts);

                if (par.size() == 2 && par.at(0) != "")
                    m_targetParameters.insert(par.at(0), par.at(1));
            }
        }
        else
            m_target = m_targetRaw;

        m_protocol = QString::fromLatin1(list[2]).trimmed();
    }

    if (m_method != UNKNOWN)
        m_valid = true;

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
                        m_headers.insert(key, value);
                }
            }
        }
        else
        {
            if (!m_body.isEmpty())
                m_body.append("\r\n");

            m_body.append(line.trimmed());
        }
    }
}

QString HttpRequest::getHeader(const QString& key, Qt::CaseSensitivity cs) const
{
    QString result = "";

    if (cs == Qt::CaseInsensitive)
        result = m_headers.value(key, "");
    else
    {
        const QStringList listKeys = m_headers.keys();
        const auto idx = listKeys.indexOf(key, 0, cs);

        if (idx >= 0)
            result = m_headers[listKeys[idx]];
    }

    return result;
}

QHash<HttpRequest::METHOD, QString> HttpRequest::initMethodTexts()
{
    QHash<METHOD, QString> result;

    result.insert(GET,      "GET");
    result.insert(HEAD,     "HEAD");
    result.insert(POST,     "POST");
    result.insert(PUT,      "PUT");
    result.insert(DELETE,   "DELETE");
    result.insert(CONNECT,  "CONNECT");
    result.insert(OPTIONS,  "OPTIONS");
    result.insert(TRACE,    "TRACE");
    result.insert(PATCH,    "PATCH");

    return result;
}
