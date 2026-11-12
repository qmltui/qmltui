#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Engine/InputScope.h>
#include <QtQml/qqmlregistration.h>
#include <cstdint>
#include <memory>

namespace qmltui {

struct PopupPrivate;

// Floating overlay container painted in the Terminal's overlay pass above
// normal scene items.
//
// Implements InputScope for modal focus trapping, background dimming, and
// Escape / outside-click dismissal, and restores previous keyboard focus when
// closed.
class QMLTUI_CORE_EXPORT Popup : public Item, public InputScope {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Popup)
    QML_ELEMENT

    Q_PROPERTY(bool modal READ modal WRITE setModal NOTIFY modalChanged)
    Q_PROPERTY(
        bool dimBackground READ dimBackground WRITE setDimBackground NOTIFY dimBackgroundChanged)
    Q_PROPERTY(
        ClosePolicies closePolicy READ closePolicy WRITE setClosePolicy NOTIFY closePolicyChanged)
    Q_PROPERTY(qmltui::Item* initialFocusItem READ initialFocusItem WRITE setInitialFocusItem NOTIFY
                   initialFocusItemChanged)
    // Logical opener Item used to nest popups (parentPopup) when the popup is
    // parented directly to the root Terminal.
    Q_PROPERTY(qmltui::Item* owner READ owner WRITE setOwner NOTIFY ownerChanged)
    Q_PROPERTY(qmltui::Popup* parentPopup READ parentPopup NOTIFY ownerChanged)

public:
    enum ClosePolicy : uint8_t {
        NoAutoClose = 0,
        CloseOnEscape = 1 << 0,
        CloseOnOutsideClick = 1 << 1,
    };
    Q_DECLARE_FLAGS(ClosePolicies, ClosePolicy)
    Q_FLAG(ClosePolicies)

    explicit Popup(QObject* parent = nullptr);
    ~Popup() override;

    bool modal() const;
    void setModal(bool modal);

    bool dimBackground() const;
    void setDimBackground(bool dim);

    ClosePolicies closePolicy() const;
    void setClosePolicy(ClosePolicies policy);
    Item* initialFocusItem() const;
    void setInitialFocusItem(Item* item);
    Item* owner() const;
    void setOwner(Item* owner);
    Popup* parentPopup() const;
    quint64 openSerial() const;
    void setOpenSerial(quint64 serial);
    bool isDescendantOfPopup(const Popup* popup) const;

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();

    void setVisible(bool visible) override;
    void setParentItem(Item* parentItem) override;

    // InputScope interface
    bool isScopeVisible() const override { return visible(); }
    bool isScopeModal() const override { return modal(); }
    bool closeOnOutsideClick() const override { return (closePolicy() & CloseOnOutsideClick) != 0; }
    bool closeOnEscape() const override { return (closePolicy() & CloseOnEscape) != 0; }
    bool rejectScope() override {
        reject();
        return true;
    }
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
    std::unique_ptr<PopupPrivate> m_d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Popup::ClosePolicies)

} // namespace qmltui
