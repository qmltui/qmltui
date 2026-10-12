#include <Qamui/Runner/MemoryTerminalBackend.h>

namespace qamui {

MemoryTerminalBackend::MemoryTerminalBackend(QSize size, QObject* parent)
    : TerminalBackend(parent)
    , m_size(size)
{
}

bool MemoryTerminalBackend::enterRawMode()
{
    m_rawMode = true;
    return true;
}

bool MemoryTerminalBackend::leaveRawMode()
{
    m_rawMode = false;
    return true;
}

TerminalBackend::WriteStatus MemoryTerminalBackend::writeSome(const char* data, size_t size, size_t& offset)
{
    if (offset >= size)
        return WriteStatus::Done;
    m_output.append(data + offset, size - offset);
    offset = size;
    return WriteStatus::Done;
}

bool MemoryTerminalBackend::writeAll(std::string& pending, int timeoutMs)
{
    Q_UNUSED(timeoutMs);
    m_output += pending;
    pending.clear();
    return true;
}

void MemoryTerminalBackend::feedInput(const QByteArray& bytes)
{
    if (!bytes.isEmpty())
        emit inputReceived(bytes);
}

void MemoryTerminalBackend::setSize(const QSize& size)
{
    if (m_size == size)
        return;
    m_size = size;
    emit resized();
}

} // namespace qamui
