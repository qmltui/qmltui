#pragma once

#include "Item.h"

namespace qtui {

class Popup : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool modal READ modal WRITE setModal NOTIFY modalChanged)
    Q_PROPERTY(bool dimBackground READ dimBackground WRITE setDimBackground NOTIFY dimBackgroundChanged)
    Q_PROPERTY(int closePolicy READ closePolicy WRITE setClosePolicy NOTIFY closePolicyChanged)

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

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();

    void setVisible(bool visible);
    void setParentItem(Item *parentItem);

signals:
    void opened();
    void closed();
    void modalChanged();
    void dimBackgroundChanged();
    void closePolicyChanged();

private:
    bool m_modal = false;
    bool m_dimBackground = true;
    int m_closePolicy = CloseOnEscape | CloseOnOutsideClick;
};

} // namespace qtui
