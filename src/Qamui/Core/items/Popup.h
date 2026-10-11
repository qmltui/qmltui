#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>

#include <Qamui/Core/items/Rectangle.h>
#include <Qamui/Core/engine/InputScope.h>
#include <QPointer>

namespace qamui {

class QAMUI_CORE_EXPORT Popup : public Rectangle, public InputScope {
    Q_OBJECT
    // Registered as a QML type by Qamui.Controls (see Controls/ForeignTypes.h);
    // the class stays in Core because Terminal manages popups.

    Q_PROPERTY(bool modal READ modal WRITE setModal NOTIFY modalChanged)
    Q_PROPERTY(bool dimBackground READ dimBackground WRITE setDimBackground NOTIFY dimBackgroundChanged)
    Q_PROPERTY(int closePolicy READ closePolicy WRITE setClosePolicy NOTIFY closePolicyChanged)
    Q_PROPERTY(qamui::Item* initialFocusItem READ initialFocusItem WRITE setInitialFocusItem NOTIFY initialFocusItemChanged)
    Q_PROPERTY(qamui::Item* owner READ owner WRITE setOwner NOTIFY ownerChanged)
    Q_PROPERTY(qamui::Popup* parentPopup READ parentPopup NOTIFY ownerChanged)

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
    Item *owner() const { return m_owner; }
    void setOwner(Item *owner);
    Popup *parentPopup() const;
    quint64 openSerial() const { return m_openSerial; }
    void setOpenSerial(quint64 serial) { m_openSerial = serial; }
    bool isDescendantOfPopup(const Popup *popup) const;

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();

    void setVisible(bool visible) override;
    void setParentItem(Item *parentItem) override;

    // InputScope interface
    bool isScopeVisible() const override { return visible(); }
    bool isScopeModal() const override { return modal(); }
    bool closeOnOutsideClick() const override { return (closePolicy() & CloseOnOutsideClick) != 0; }
    bool closeOnEscape() const override { return (closePolicy() & CloseOnEscape) != 0; }
    bool rejectScope() override { reject(); return true; }
    Item* asItem() override { return this; }
    const Item* asItem() const override { return this; }
    bool isDescendantOfScope(const InputScope* other) const override;

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
    void ownerChanged();

private:
    bool m_modal = false;
    bool m_dimBackground = true;
    int m_closePolicy = CloseOnEscape | CloseOnOutsideClick;
    QPointer<Item> m_initialFocusItem;
    QPointer<Item> m_previousFocusItem;
    bool m_tookFocus = false;
    QPointer<Item> m_owner;
    quint64 m_openSerial = 0;
};

} // namespace qamui
