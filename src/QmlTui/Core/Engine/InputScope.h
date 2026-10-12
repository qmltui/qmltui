#pragma once

namespace qmltui {

class Item;

class InputScope {
public:
    virtual ~InputScope() = default;

    virtual bool isScopeVisible() const = 0;
    virtual bool isScopeModal() const = 0;
    virtual bool closeOnOutsideClick() const = 0;
    virtual bool closeOnEscape() const = 0;
    virtual bool rejectScope() = 0;
    virtual Item* asItem() = 0;
    virtual const Item* asItem() const = 0;
    virtual bool isDescendantOfScope(const InputScope* other) const = 0;
};

} // namespace qmltui
