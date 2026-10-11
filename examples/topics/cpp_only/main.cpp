// Entry point of the C++-only Qamui example. There is no QML engine: the whole
// interface is built in ServiceDashboard.cpp from Qamui's C++ items.

#include "ServiceDashboard.h"

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // A root Terminal takes over the screen when the event loop starts and
    // restores it when the object is destroyed.
    ServiceDashboard dashboard;
    dashboard.setLiveTelemetry(true);

    return app.exec();
}
