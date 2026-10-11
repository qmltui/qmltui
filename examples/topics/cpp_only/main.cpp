// The interface is built entirely from Qamui's C++ items.
#include "ServiceDashboard.h"

#include <QCoreApplication>

#ifdef QAMUI_CPP_ONLY_RUNNER
#include <Qamui/Runner/Runner.h>
#include <QQmlEngine>
#include <memory>
#endif

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ServiceDashboard dashboard;
    bool scenarioRequested = false;
#ifdef QAMUI_CPP_ONLY_RUNNER
    // Only automation needs a script engine; ordinary startup stays C++ only.
    std::unique_ptr<QQmlEngine> scriptEngine;
    for (const char *name : {"QAMUI_RUNNER_SCRIPT", "QAMUI_RUNNER_EVAL",
                             "QAMUI_RUNNER_CAPTURE_AFTER_MS"}) {
        scenarioRequested |= !qgetenv(name).trimmed().isEmpty();
    }
    if (scenarioRequested) {
        scriptEngine = std::make_unique<QQmlEngine>();
        QJSEngine::setObjectOwnership(&dashboard, QJSEngine::CppOwnership);
        qamui::Runner::startFromEnvironment(scriptEngine.get(), &dashboard);
    }
#endif
    // Scenarios advance telemetry explicitly through rootObject().tick().
    dashboard.setLiveTelemetry(!scenarioRequested);
    return app.exec();
}
