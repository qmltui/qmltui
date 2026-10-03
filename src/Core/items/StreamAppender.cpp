#include "StreamAppender.h"
#include "DelegateUtils.h"
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
        connect(m_model, &QObject::destroyed, this, [this]() {
            m_model.clear();
            m_consumedRowCount = 0;
            emit modelChanged();
        });
        connect(m_model, &QAbstractItemModel::rowsInserted, this,
                [this](const QModelIndex& parent, int first, int last) {
                    if (parent.isValid()) {
                        qWarning("StreamAppender ignores rows inserted below the model root.");
                        return;
                    }
            Q_UNUSED(first);
            Q_UNUSED(last);
            appendExistingRows();
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
    if (m_delegate) disconnect(m_delegate, nullptr, this, nullptr);
    m_delegate = delegate;
    if (m_delegate) {
        connect(m_delegate, &QObject::destroyed, this, [this]() {
            m_delegate.clear();
            emit delegateChanged();
        });
        connect(m_delegate, &QQmlComponent::statusChanged, this,
                [this](QQmlComponent::Status status) {
            if (status == QQmlComponent::Ready)
                appendExistingRows();
            else if (status == QQmlComponent::Error)
                DelegateUtils::isReady(m_delegate, this);
        });
    }
    emit delegateChanged();
    appendExistingRows();
}

void StreamAppender::appendExistingRows() {
    if (!m_model || !DelegateUtils::isReady(m_delegate, this) || !terminal()) return;

    const int lastRow = m_model->rowCount() - 1;
    if (m_consumedRowCount <= lastRow) {
        m_consumedRowCount = appendRows(m_consumedRowCount, lastRow);
    }
}

int StreamAppender::appendRows(int first, int last) {
    Terminal* output = terminal();
    if (!m_model || !m_delegate || !output) return first;

    const QHash<int, QByteArray> roles = m_model->roleNames();
    for (int row = first; row <= last; ++row) {
        const QModelIndex index = m_model->index(row, 0);
        QQmlComponent* rowDelegate = delegateForRow(row);
        if (!DelegateUtils::isReady(rowDelegate, this)) return row;

        QVariantMap properties;
        QVariantMap modelMap;
        properties.insert(QStringLiteral("index"), row);
        for (auto role = roles.cbegin(); role != roles.cend(); ++role) {
            const QString roleName = QString::fromUtf8(role.value());
            const QVariant value = m_model->data(index, role.key());
            properties.insert(roleName, value);
            modelMap.insert(roleName, value);
        }
        properties.insert(QStringLiteral("model"), modelMap);
        properties.insert(QStringLiteral("modelData"), modelMap);

        QQmlContext *context = DelegateUtils::createContext(rowDelegate, this);
        if (!context) return row;
        DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("index"), row);
        DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("model"), modelMap);
        DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("modelData"), modelMap);
        for (auto it = modelMap.cbegin(); it != modelMap.cend(); ++it)
            DelegateUtils::setContextProperty(rowDelegate, context, it.key(), it.value());

        Item *item = DelegateUtils::createItem(
            rowDelegate, properties, context, output, this);
        if (!item) return row;

        output->append(item);
        delete item;
    }
    return last + 1;
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
