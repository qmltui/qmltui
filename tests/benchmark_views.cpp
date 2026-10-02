#include <QAbstractListModel>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <memory>
#include <TreeView.h>

class CountModel final : public QAbstractListModel {
public:
    explicit CountModel(int count, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_count(count) {}

    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : m_count;
    }

    QVariant data(const QModelIndex &index, int role) const override {
        return index.isValid() && role == Qt::DisplayRole
            ? QVariant(index.row()) : QVariant();
    }

    QHash<int, QByteArray> roleNames() const override {
        return {{Qt::DisplayRole, "display"}};
    }

private:
    int m_count;
};

static qint64 measureCreation(QQmlEngine &engine, int rowCount)
{
    CountModel model(rowCount);
    engine.rootContext()->setContextProperty("benchmarkModel", &model);
    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        import Qtui.Widgets
        TreeView {
            width: 80
            height: 24
            model: benchmarkModel
            delegate: Item {
                required property int depth
                required property bool hasChildren
                required property bool expanded
                required property bool isCurrent
                required property string display
                required property var modelData
            }
        }
    )", QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << component.errorString();
        return -1;
    }

    QElapsedTimer timer;
    timer.start();
    std::unique_ptr<QObject> object(component.create());
    auto *tree = qobject_cast<qtui::TreeView*>(object.get());
    if (!tree) return -1;
    qtui::TextGridBuffer buffer;
    if (!buffer.resize(80, 24)) return -1;
    tree->render(buffer, 0, 0);
    return timer.elapsed();
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    qtui::TreeView ensureModuleIsLinked;
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath()
                         + QStringLiteral("/../Qtui"));

    const qint64 oneThousandMs = measureCreation(engine, 1'000);
    const qint64 tenThousandMs = measureCreation(engine, 10'000);
    qInfo() << "TreeView creation: 1k rows" << oneThousandMs
            << "ms; 10k rows" << tenThousandMs << "ms";
    qInfo() << "Virtualization review threshold: 10k creation > 500 ms";
    return oneThousandMs < 0 || tenThousandMs < 0 ? 1 : 0;
}
