// The TypeScript declarations (qtui-runner.d.ts) drive editor completion for
// scenario authors; this test fails when a facade method exists that the
// declarations do not mention, so the two cannot drift apart silently.
#include <QFile>
#include <QRegularExpression>
#include <QtTest/QtTest>

class TestRunnerDeclarations : public QObject {
    Q_OBJECT

private slots:
    void everyFacadeMethodIsDeclared();
};

static QString readAll(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

void TestRunnerDeclarations::everyFacadeMethodIsDeclared()
{
    const QString sourceDir = QStringLiteral(QTUI_SOURCE_DIR);
    const QString runnerSource = readAll(sourceDir + QStringLiteral("/src/Qtui/Runner/Runner.cpp"));
    QVERIFY(!runnerSource.isEmpty());
    const QString declarations = readAll(sourceDir + QStringLiteral("/src/Qtui/Runner/qtui-runner.d.ts"));
    QVERIFY(!declarations.isEmpty());

    // The facade's methodNames array is the authoritative list of what a
    // scenario can call on the terminal argument.
    const QRegularExpression listPattern(
        QStringLiteral("const methodNames = \\[(.*?)\\];"),
        QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch listMatch = listPattern.match(runnerSource);
    QVERIFY2(listMatch.hasMatch(), "methodNames array not found in Runner.cpp");

    QStringList names;
    const QRegularExpression namePattern(QStringLiteral("\"(\\w+)\""));
    QRegularExpressionMatchIterator nameMatches = namePattern.globalMatch(listMatch.captured(1));
    while (nameMatches.hasNext())
        names.append(nameMatches.next().captured(1));
    QVERIFY(names.size() > 30);
    // The two facade-built locator constructors are part of the API too.
    names << QStringLiteral("region") << QStringLiteral("item");

    QStringList missing;
    for (const QString& name : std::as_const(names)) {
        const QRegularExpression declared(
            QStringLiteral("\\b%1\\s*\\(").arg(QRegularExpression::escape(name)));
        if (!declarations.contains(declared))
            missing.append(name);
    }
    QVERIFY2(missing.isEmpty(),
             qPrintable(QStringLiteral("Facade methods missing from qtui-runner.d.ts: %1")
                            .arg(missing.join(QStringLiteral(", ")))));
}

QTEST_GUILESS_MAIN(TestRunnerDeclarations)

#include "test_runner_declarations.moc"
