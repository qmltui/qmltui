#include "CastRecorder_p.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace qmltui {

bool CastRecorder::open(const QString& path, int columns, int rows, const QString& title, QString* error)
{
    close();
    m_file.setFileName(path);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = QStringLiteral("Cannot write cast %1: %2").arg(path, m_file.errorString());
        return false;
    }
    QJsonObject header{{QStringLiteral("version"), 2},
                       {QStringLiteral("width"), columns},
                       {QStringLiteral("height"), rows},
                       {QStringLiteral("title"), title},
                       {QStringLiteral("env"), QJsonObject{{QStringLiteral("TERM"), QStringLiteral("xterm-256color")},
                                                           {QStringLiteral("SHELL"), QStringLiteral("/bin/sh")}}},
                       {QStringLiteral("theme"), QJsonObject{{QStringLiteral("fg"), QStringLiteral("#c9d1d9")},
                                                             {QStringLiteral("bg"), QStringLiteral("#0d1117")},
                                                             {QStringLiteral("palette"), QStringLiteral("#000000:#cd0000:#00cd00:#cdcd00:#0000ee:#cd00cd:#00cdcd:#e5e5e5:#7f7f7f:#ff0000:#00ff00:#ffff00:#5c5cff:#ff00ff:#00ffff:#ffffff")}}}};
    const QByteArray headerBytes = QJsonDocument(header).toJson(QJsonDocument::Compact);
    if (m_file.write(headerBytes) != headerBytes.size() || m_file.write("\n") != 1) {
        if (error) *error = QStringLiteral("Cannot write cast header %1: %2").arg(path, m_file.errorString());
        m_file.close();
        return false;
    }
    return true;
}

void CastRecorder::output(double seconds, const std::string& bytes)
{
    event(seconds, "o", QString::fromUtf8(bytes.data(), static_cast<qsizetype>(bytes.size())));
}

void CastRecorder::marker(double seconds, const QString& label)
{
    event(seconds, "m", label);
}

void CastRecorder::resize(double seconds, int columns, int rows)
{
    event(seconds, "r", QStringLiteral("%1x%2").arg(columns).arg(rows));
}

void CastRecorder::event(double seconds, const char* code, const QString& data)
{
    if (!m_file.isOpen()) return;
    const QJsonArray line{seconds, QString::fromLatin1(code), data};
    const QByteArray lineBytes = QJsonDocument(line).toJson(QJsonDocument::Compact);
    if (m_file.write(lineBytes) != lineBytes.size() || m_file.write("\n") != 1) {
        m_file.close();
    }
}

void CastRecorder::close()
{
    if (m_file.isOpen()) m_file.close();
}

} // namespace qmltui
