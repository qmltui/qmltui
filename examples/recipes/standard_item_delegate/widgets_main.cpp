// Reference viewer: the same ReleaseChecklistModel shown by Qt Widgets'
// QListView, QTableView, and QTreeView with their default QStyledItemDelegate.
// Compare it with the terminal rendering in Main.qml: colors, check boxes,
// alignment, decorations, and the disabled row come from the same roles.
// Built only when Qt6::Widgets is available. Pass --smoke to exit after the
// first event loop iteration (used by the CTest smoke test).
#include "ReleaseChecklistModel.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QTableView>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include <QHeaderView>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    ReleaseChecklistModel model;

    QWidget window;
    window.setWindowTitle(QStringLiteral("Standard item roles: Qt Widgets reference"));
    auto* layout = new QVBoxLayout(&window);

    auto* title =
        new QLabel(QStringLiteral("<b>One QStandardItemModel, three Qt Widgets views</b><br>"
                                  "Click a check box; the terminal example shows the same roles."));
    layout->addWidget(title);

    auto* row = new QHBoxLayout;
    auto* list = new QListView;
    list->setModel(&model);
    auto* table = new QTableView;
    table->setModel(&model);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->hide();
    row->addWidget(list, 1);
    row->addWidget(table, 2);
    layout->addLayout(row);

    auto* tree = new QTreeView;
    tree->setModel(&model);
    tree->header()->setStretchLastSection(true);
    layout->addWidget(tree);

    // One selection model, as the terminal example binds its three views to
    // the ListView's currentIndex.
    table->setSelectionModel(list->selectionModel());
    tree->setSelectionModel(list->selectionModel());
    list->setCurrentIndex(model.index(0, 0));

    auto* status = new QLabel;
    const auto updateStatus = [&] {
        status->setText(
            QStringLiteral("%1 of %2 tasks done").arg(model.doneCount()).arg(model.rowCount()));
    };
    QObject::connect(&model, &ReleaseChecklistModel::doneCountChanged, status, updateStatus);
    updateStatus();
    layout->addWidget(status);

    window.resize(900, 520);
    window.show();

    if (app.arguments().contains(QStringLiteral("--smoke"))) {
        // Exercise the same interaction the terminal scenario performs.
        QTimer::singleShot(0, &app, [&] {
            model.toggle(1);
            const bool ok = model.doneCount() == 2 && !model.item(4, 0)->isEnabled();
            app.exit(ok ? 0 : 1);
        });
    }
    return app.exec();
}
