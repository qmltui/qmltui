#pragma once

#include "Rectangle.h"
#include <QPointer>

namespace qtui {

class Popup : public Rectangle {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool modal READ modal WRITE setModal NOTIFY modalChanged)
    Q_PROPERTY(bool dimBackground READ dimBackground WRITE setDimBackground NOTIFY dimBackgroundChanged)
    Q_PROPERTY(int closePolicy READ closePolicy WRITE setClosePolicy NOTIFY closePolicyChanged)
    Q_PROPERTY(qtui::Item* initialFocusItem READ initialFocusItem WRITE setInitialFocusItem NOTIFY initialFocusItemChanged)

public:
    enum ClosePolicy {
        NoAutoClose = 0,
        CloseOnEscape = 1 << 0,
        CloseOnOutsideClick = 1 << 1
    };
    Q_DECLARE_FLAGS(ClosePolicies, ClosePolicy)
    Q_FLAG(ClosePolicies)

    explicit Popup(QObject *parent = nullptr);
    ~Popup() override;

    bool modal() const { return m_modal; }
    void setModal(bool modal);

    bool dimBackground() const { return m_dimBackground; }
    void setDimBackground(bool dim);

    int closePolicy() const { return m_closePolicy; }
    void setClosePolicy(int policy);
    Item *initialFocusItem() const { return m_initialFocusItem; }
    void setInitialFocusItem(Item *item);

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();

    void setVisible(bool visible) override;
    void setParentItem(Item *parentItem) override;

signals:
    void opened();
    void closed();
    void accepted();
    void rejected();
    void finished(bool accepted);
    void modalChanged();
    void dimBackgroundChanged();
    void closePolicyChanged();
    void initialFocusItemChanged();

private:
    bool m_modal = false;
    bool m_dimBackground = true;
    int m_closePolicy = CloseOnEscape | CloseOnOutsideClick;
    QPointer<Item> m_initialFocusItem;
    QPointer<Item> m_previousFocusItem;
};

} // namespace qtui
