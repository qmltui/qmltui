#pragma once

#include <QFile>
#include <QString>

#include <string>

namespace qmltui {

// Writes an asciicast v2 file: one JSON header line followed by one JSON event
// per line. Times are seconds since the recording started; the header carries
// no wall-clock timestamp so a scenario replayed on the virtual clock produces
// a byte-identical cast.
class CastRecorder {
public:
    bool open(const QString& path, int columns, int rows, const QString& title, QString* error);
    bool isOpen() const { return m_file.isOpen(); }
    QString path() const { return m_file.fileName(); }

    void output(double seconds, const std::string& bytes);
    void marker(double seconds, const QString& label);
    void resize(double seconds, int columns, int rows);
    void close();

private:
    void event(double seconds, const char* code, const QString& data);

    QFile m_file;
};

} // namespace qmltui
