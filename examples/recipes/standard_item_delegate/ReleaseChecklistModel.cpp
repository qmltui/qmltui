#include "ReleaseChecklistModel.h"

#include <QColor>

ReleaseChecklistModel::ReleaseChecklistModel(QObject* parent) : QStandardItemModel(parent) {
    setHorizontalHeaderLabels(
        {QStringLiteral("Task"), QStringLiteral("Owner"), QStringLiteral("ETA")});

    struct Row {
        const char* task;
        const char* owner;
        const char* eta;
        Qt::CheckState state;
        QColor color;
    };
    const Row rows[] = {
        {"Freeze API", "core", "done", Qt::Checked, QColor(Qt::green)},
        {"Update docs", "docs", "2 d", Qt::Unchecked, QColor()},
        {"Run benchmarks", "perf", "1 d", Qt::PartiallyChecked, QColor(Qt::yellow)},
        {"Sign installers", "release", "4 h", Qt::Unchecked, QColor(Qt::red)},
        {"Announce", "marketing", "blocked", Qt::Unchecked, QColor()},
    };
    for (const Row& row : rows) {
        auto* task = new QStandardItem(QString::fromLatin1(row.task));
        task->setCheckable(true);
        task->setCheckState(row.state);
        if (row.color.isValid()) task->setForeground(row.color);
        auto* owner = new QStandardItem(QString::fromLatin1(row.owner));
        auto* eta = new QStandardItem(QString::fromLatin1(row.eta));
        eta->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        appendRow({task, owner, eta});
    }
    // Decoration glyphs stand in for icons; a QIcon would be dropped.
    item(0, 0)->setData(QStringLiteral("*"), Qt::DecorationRole);
    item(3, 0)->setData(QStringLiteral("!"), Qt::DecorationRole);
    // "Announce" cannot start yet: disabled rows paint dimmed and refuse toggles.
    for (int column = 0; column < 3; ++column)
        item(4, column)->setEnabled(false);
    item(3, 2)->setBackground(QColor(Qt::darkRed));

    connect(this, &QStandardItemModel::itemChanged, this, &ReleaseChecklistModel::doneCountChanged);
}

int ReleaseChecklistModel::doneCount() const {
    int count = 0;
    for (int row = 0; row < rowCount(); ++row)
        if (item(row, 0)->checkState() == Qt::Checked) ++count;
    return count;
}

void ReleaseChecklistModel::toggle(int row) {
    QStandardItem* task = item(row, 0);
    if (!task || !task->isEnabled() || !task->isCheckable()) return;
    task->setCheckState(task->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
}
