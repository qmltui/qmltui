#include "StreamAppender.h"
#include "Item.h"
#include "Terminal.h"
#include <QQmlContext>
#include <QQmlListReference>

namespace qtui {

StreamAppender::StreamAppender(QObject* parent)
    : QObject(parent) {
}

void StreamAppender::setModel(QAbstractItemModel* model) {
    if (m_model == model) return;

    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }

    m_model = model;
    m_consumedRowCount = 0;

    if (m_model) {
        connect(m_model, &QAbstractItemModel::rowsInserted, this,
                [this](const QModelIndex&, int first, int last) {
                    if (!m_delegate || !terminal()) return;
                    appendRows(first, last);
                    m_consumedRowCount = m_model ? m_model->rowCount() : 0;
                });
        connect(m_model, &QAbstractItemModel::rowsRemoved, this,
                [this]() {
                    m_consumedRowCount = m_model ? m_model->rowCount() : 0;
                });
        connect(m_model, &QAbstractItemModel::modelReset, this,
                [this]() {
                    m_consumedRowCount = 0;
                    appendExistingRows();
                });
    }

    emit modelChanged();
    appendExistingRows();
}

void StreamAppender::setDelegate(QQmlComponent* delegate) {
    if (m_delegate == delegate) return;
    m_delegate = delegate;
    emit delegateChanged();
    appendExistingRows();
}

void StreamAppender::appendExistingRows() {
    if (!m_model || !m_delegate || !terminal()) return;

    const int lastRow = m_model->rowCount() - 1;
    if (m_consumedRowCount <= lastRow) {
        appendRows(m_consumedRowCount, lastRow);
    }
    m_consumedRowCount = m_model->rowCount();
}

void StreamAppender::appendRows(int first, int last) {
    Terminal* output = terminal();
    if (!m_model || !m_delegate || !output) return;

    const QHash<int, QByteArray> roles = m_model->roleNames();
    for (int row = first; row <= last; ++row) {
        const QModelIndex index = m_model->index(row, 0);
        QQmlComponent* rowDelegate = delegateForRow(row);
        if (!rowDelegate) continue;

        QVariantMap properties;
        properties.insert(QStringLiteral("index"), row);
        for (auto role = roles.cbegin(); role != roles.cend(); ++role) {
            properties.insert(QString::fromUtf8(role.value()), m_model->data(index, role.key()));
        }

        QObject* object = rowDelegate->createWithInitialProperties(properties, qmlContext(this));
        Item* item = qobject_cast<Item*>(object);
        if (!item) {
            delete object;
            continue;
        }

        output->append(item);
        delete item;
    }
}

QQmlComponent* StreamAppender::delegateForRow(int row) const {
    QQmlListReference choices(m_delegate, "choices");
    if (!choices.isValid()) {
        return m_delegate;
    }

    const QModelIndex modelIndex = m_model->index(row, 0);
    const QString roleName = m_delegate->property("role").toString();
    QVariant roleValue;
    const QHash<int, QByteArray> roles = m_model->roleNames();
    for (auto role = roles.cbegin(); role != roles.cend(); ++role) {
        if (role.value() == roleName.toUtf8()) {
            roleValue = m_model->data(modelIndex, role.key());
            break;
        }
    }

    for (qsizetype choiceIndex = 0; choiceIndex < choices.count(); ++choiceIndex) {
        QObject* choice = choices.at(choiceIndex);
        if (!choice) continue;

        const int choiceRow = choice->property("row").toInt();
        const int choiceColumn = choice->property("column").toInt();
        const QVariant choiceValue = choice->property("roleValue");
        const bool rowMatches = choiceRow < 0 || choiceRow == row;
        const bool columnMatches = choiceColumn < 0 || choiceColumn == 0;
        const bool roleMatches = !choiceValue.isValid() || choiceValue == roleValue;
        if (!rowMatches || !columnMatches || !roleMatches) continue;

        return qobject_cast<QQmlComponent*>(
            choice->property("delegate").value<QObject*>());
    }

    return nullptr;
}

Terminal* StreamAppender::terminal() const {
    QObject* current = parent();
    while (current) {
        if (auto* result = qobject_cast<Terminal*>(current)) {
            return result;
        }
        current = current->parent();
    }
    return nullptr;
}

} // namespace qtui
