#include <QCoreApplication>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QVariant>

#include <Terminal.h>
#include <Testing/TestRunner.h>

namespace {
constexpr int operationTimeoutMs = 1500;

// test_plc.js deliberately holds five states for two seconds each so a person
// can follow the scenario in the alternate screen. Keep the end-to-end guard
// comfortably above that presentation time and the bounded condition waits.
constexpr int scenarioTimeoutMs = 30000;
} // namespace

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  QQmlApplicationEngine engine;
  engine.loadFromModule("IndustrialPlcHmi", "Main");
  if (engine.rootObjects().isEmpty())
    return -1;

  auto *terminal =
      qobject_cast<qtui::Terminal *>(engine.rootObjects().constFirst());
  if (!terminal)
    return -1;

  terminal->setProperty("deterministicSimulation", true);

  QString artifactDirectory =
      QString::fromUtf8(qgetenv("QTUI_TEST_ARTIFACT_DIR")).trimmed();
  if (artifactDirectory.isEmpty()) {
    artifactDirectory = QDir::temp().filePath(
        QStringLiteral("qtui-industrial-plc-test-artifacts"));
  }

  auto *runner = new qtui::TestRunner(
      &engine, terminal,
      QStringLiteral(":/qt/qml/IndustrialPlcHmi/test_plc.js"),
      artifactDirectory, QSize(120, 80), operationTimeoutMs, scenarioTimeoutMs,
      QString());

  QTimer::singleShot(0, runner, &qtui::TestRunner::start);
  return app.exec();
}
