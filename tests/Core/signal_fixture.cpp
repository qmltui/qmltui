// A live Qtui process for the signal-safety test: a bare Terminal on the real
// platform backend, no Runner, no handlers of its own. The test drives it
// through a pseudo-terminal and job-control signals.
#include <QCoreApplication>
#include <Qtui/Core/items/Terminal.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    qtui::Terminal terminal;
    terminal.setTitle(QStringLiteral("signal fixture"));
    (void)terminal.engine();
    return app.exec();
}
