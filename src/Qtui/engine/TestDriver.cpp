#include "TestDriver.h"
#include "TerminalEngine.h"
#include "FrameRenderer.h"
#include "items/Terminal.h"
#include "primitives/TextGridBuffer.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QDebug>
#include <QQmlContext>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <iostream>
#include <cstdlib>

namespace qtui {

TestDriver* TestDriver::s_instance = nullptr;

TestDriver::TestDriver(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
    const char* scriptEnv = std::getenv("QTUI_SCRIPT");
    if (scriptEnv && std::strlen(scriptEnv) > 0) {
        m_active = true;
    }
}

TestDriver::~TestDriver()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

TestDriver* TestDriver::instance()
{
    if (!s_instance) {
        s_instance = new TestDriver(QCoreApplication::instance());
    }
    return s_instance;
}

void TestDriver::attachTerminalEngine(TerminalEngine* engine)
{
    m_terminalEngine = engine;
}

void TestDriver::attachTerminal(Terminal* terminal)
{
    m_terminal = terminal;
    if (terminal) {
        m_terminalEngine = terminal->engine();

        const char* scriptEnv = std::getenv("QTUI_SCRIPT");
        if (scriptEnv && std::strlen(scriptEnv) > 0 && !m_scriptExecuted) {
            m_scriptExecuted = true;
            QString scriptPath = QString::fromUtf8(scriptEnv);
            QTimer::singleShot(0, [this, scriptPath]() {
                QQmlEngine* engine = ::qmlEngine(m_terminal);
                if (!engine) {
                    engine = new QQmlEngine(this);
                }
                executeScriptFile(engine, scriptPath);
            });
        }
    }
}

void TestDriver::sendKey(const QString& keyName, const QStringList& modifiers)
{
    if (!m_terminalEngine) {
        qWarning() << "[TestDriver] Cannot send key - TerminalEngine not attached!";
        return;
    }

    Qt::Key qtKey = Qt::Key_unknown;
    QString text = keyName;

    QString upperKey = keyName.trimmed().toUpper();
    if (upperKey == "ENTER" || upperKey == "RETURN") qtKey = Qt::Key_Return;
    else if (upperKey == "ESC" || upperKey == "ESCAPE") qtKey = Qt::Key_Escape;
    else if (upperKey == "TAB") qtKey = Qt::Key_Tab;
    else if (upperKey == "BACKSPACE" || upperKey == "BS") qtKey = Qt::Key_Backspace;
    else if (upperKey == "UP") qtKey = Qt::Key_Up;
    else if (upperKey == "DOWN") qtKey = Qt::Key_Down;
    else if (upperKey == "LEFT") qtKey = Qt::Key_Left;
    else if (upperKey == "RIGHT") qtKey = Qt::Key_Right;
    else if (upperKey == "SPACE") qtKey = Qt::Key_Space;
    else if (upperKey.startsWith("F") && upperKey.length() > 1) {
        bool ok = false;
        int num = upperKey.mid(1).toInt(&ok);
        if (ok && num >= 1 && num <= 35) {
            qtKey = static_cast<Qt::Key>(Qt::Key_F1 + (num - 1));
        }
    } else if (keyName.length() == 1) {
        QChar ch = keyName.at(0);
        if (ch.isLetter()) {
            qtKey = static_cast<Qt::Key>(Qt::Key_A + (ch.toUpper().toLatin1() - 'A'));
        } else if (ch.isDigit()) {
            qtKey = static_cast<Qt::Key>(Qt::Key_0 + (ch.toLatin1() - '0'));
        }
    }

    Qt::KeyboardModifiers mods = Qt::NoModifier;
    for (const QString& mod : modifiers) {
        QString m = mod.toUpper();
        if (m == "SHIFT") mods |= Qt::ShiftModifier;
        else if (m == "CTRL" || m == "CONTROL") mods |= Qt::ControlModifier;
        else if (m == "ALT") mods |= Qt::AltModifier;
    }

    m_terminalEngine->handleKeyInput(qtKey, text, mods);

    // Render frame after key event
    m_terminalEngine->requestRender();
    wait(20);
}

void TestDriver::typeText(const QString& text)
{
    for (int i = 0; i < text.length(); ++i) {
        sendKey(QString(text.at(i)));
    }
}

void TestDriver::wait(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

bool TestDriver::snapshot(const QString& imagePathArg, const QString& textPathArg)
{
    if (!m_terminalEngine) {
        qWarning() << "[TestDriver] Cannot snapshot - TerminalEngine not attached!";
        return false;
    }

    m_frameCount++;

    QString imgPath = imagePathArg;
    if (imgPath.isEmpty()) {
        imgPath = QString("/tmp/qtui_frame_%1.png").arg(m_frameCount);
    }

    QString txtPath = textPathArg;
    if (txtPath.isEmpty()) {
        txtPath = QString("/tmp/qtui_frame_%1.txt").arg(m_frameCount);
    }

    m_terminalEngine->requestRender();

    TextGridBuffer dummyGrid;
    const TextGridBuffer* gridPtr = m_terminalEngine->renderer() ? m_terminalEngine->renderer()->currentBuffer() : nullptr;
    const TextGridBuffer& grid = gridPtr ? *gridPtr : dummyGrid;
    int cols = grid.width();
    int rows = grid.height();

    // Save text grid snapshot
    QFile txtFile(txtPath);
    if (txtFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&txtFile);
        for (int r = 0; r < rows; ++r) {
            QString line;
            for (int c = 0; c < cols; ++c) {
                const Cell& cell = grid.cellAt(c, r);
                line.append(cell.character.isEmpty() ? " " : QString::fromUtf8(cell.character));
            }
            out << line << "\n";
        }
        txtFile.close();
    }

    // Render PNG screenshot if QGuiApplication is active
    bool saved = false;
    if (qobject_cast<QGuiApplication*>(qApp)) {
        int charWidth = 10;
        int charHeight = 20;
        int imgW = cols * charWidth;
        int imgH = rows * charHeight;

        QImage image(imgW, imgH, QImage::Format_RGB32);
        image.fill(QColor("#000000"));

        QPainter painter(&image);
        QFont font("Monospace", 12);
        font.setStyleHint(QFont::Monospace);
        painter.setFont(font);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                const Cell& cell = grid.cellAt(c, r);
                QRect rect(c * charWidth, r * charHeight, charWidth, charHeight);

                QColor bgColor = (cell.bgColor != 0) ? QColor::fromRgba(cell.bgColor) : QColor("#0d1c2d");
                QColor fgColor = (cell.fgColor != 0) ? QColor::fromRgba(cell.fgColor) : QColor("#D4E4FA");

                painter.fillRect(rect, bgColor);
                painter.setPen(fgColor);

                QString str = cell.character.isEmpty() ? " " : QString::fromUtf8(cell.character);
                painter.drawText(rect, Qt::AlignCenter, str);
            }
        }
        painter.end();

        saved = image.save(imgPath);
        std::cerr << "[TestDriver] Snapshot #" << m_frameCount << " saved to " << imgPath.toStdString() << std::endl;
    } else {
        std::cerr << "[TestDriver] Text snapshot #" << m_frameCount << " saved to " << txtPath.toStdString() << std::endl;
        saved = true;
    }
    return saved;
}

bool TestDriver::assertTextContains(const QString& expectedText)
{
    if (!m_terminalEngine) {
        std::cerr << "[TestDriver ASSERTION FAILED] TerminalEngine not attached!" << std::endl;
        std::exit(1);
    }

    m_terminalEngine->requestRender();

    TextGridBuffer dummyGrid;
    const TextGridBuffer* gridPtr = m_terminalEngine->renderer() ? m_terminalEngine->renderer()->currentBuffer() : nullptr;
    const TextGridBuffer& grid = gridPtr ? *gridPtr : dummyGrid;
    int cols = grid.width();
    int rows = grid.height();

    QString fullText;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const Cell& cell = grid.cellAt(c, r);
            fullText.append(cell.character.isEmpty() ? " " : QString::fromUtf8(cell.character));
        }
        fullText.append("\n");
    }

    if (!fullText.contains(expectedText)) {
        std::cerr << "========================================================================" << std::endl;
        std::cerr << "[TestDriver ASSERTION FAILED] Expected text pattern not found in UI!" << std::endl;
        std::cerr << "EXPECTED PATTERN: \"" << expectedText.toStdString() << "\"" << std::endl;
        std::cerr << "========================================================================" << std::endl;
        if (m_terminalEngine) {
            m_terminalEngine->stop();
        }
        QCoreApplication::exit(1);
        std::exit(1);
    }

    std::cerr << "[TestDriver ASSERTION PASSED] Found expected text pattern: \"" << expectedText.toStdString() << "\"" << std::endl;
    return true;
}

void TestDriver::quit(int exitCode)
{
    if (m_quitCalled) return;
    m_quitCalled = true;

    std::cerr << "[TestDriver] Test script execution completed cleanly with exit code " << exitCode << std::endl;
    if (m_terminalEngine) {
        m_terminalEngine->stop();
    }
    std::exit(exitCode);
}

void TestDriver::executeScriptFile(QQmlEngine* engine, const QString& scriptPath)
{
    m_qmlEngine = engine;
    if (!engine) {
        std::cerr << "[TestDriver ERROR] QQmlEngine is null!" << std::endl;
        return;
    }

    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "[TestDriver ERROR] Cannot open scenario script file: " << scriptPath.toStdString() << std::endl;
        if (m_terminalEngine) {
            m_terminalEngine->stop();
        }
        std::exit(1);
    }

    QString scriptCode = file.readAll();
    file.close();

    // Expose TestDriver to both QML root context AND QJS global object scope!
    engine->rootContext()->setContextProperty("TestDriver", this);
    QJSValue jsDriver = engine->newQObject(this);
    engine->globalObject().setProperty("TestDriver", jsDriver);

    std::cerr << "[TestDriver] Executing scenario script: " << scriptPath.toStdString() << std::endl;

    QJSValue jsRes = engine->evaluate(scriptCode, scriptPath);
    if (jsRes.isError()) {
        std::cerr << "[TestDriver JS ERROR] " << jsRes.toString().toStdString() << std::endl;
        if (m_terminalEngine) {
            m_terminalEngine->stop();
        }
        std::exit(1);
    }

    if (!m_quitCalled) {
        snapshot();
        quit(0);
    }
}

} // namespace qtui
