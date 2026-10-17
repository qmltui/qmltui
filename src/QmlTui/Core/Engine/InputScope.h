#pragma once

namespace qmltui {

class Item;

// Interface implemented by overlay containers (such as Popup) that participate
// in modal input routing, Escape dismissal, and outside-click dismissal.
class InputScope {
public:
    virtual ~InputScope() = default;

    virtual bool isScopeVisible() const = 0;
    // When true, blocks keyboard focus cycling and mouse delivery outside this scope.
    virtual bool isScopeModal() const = 0;
    virtual bool closeOnOutsideClick() const = 0;
    virtual bool closeOnEscape() const = 0;
    // Closes or rejects the scope; returns true when the scope consumed the dismissal.
    virtual bool rejectScope() = 0;
    virtual Item* asItem() = 0;
    virtual const Item* asItem() const = 0;
    virtual bool isDescendantOfScope(const InputScope* other) const = 0;
};

} // namespace qmltui
