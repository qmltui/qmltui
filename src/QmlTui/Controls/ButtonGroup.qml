pragma ComponentBehavior: Bound
import QtQml

// Non-visual group that links mutually exclusive checkable controls (such as
// RadioButton) by assigning itself to each member's `buttonGroup` property or
// accepting self-registration from controls with `buttonGroup: group`.
QtObject {
    id: group

    property list<QtObject> buttons
    property var registeredButtons: []

    function addButton(btn) {
        if (!btn)
            return;
        for (let i = 0; i < group.registeredButtons.length; ++i) {
            if (group.registeredButtons[i] === btn)
                return;
        }
        const next = group.registeredButtons.slice();
        next.push(btn);
        group.registeredButtons = next;
    }

    function removeButton(btn) {
        if (!btn)
            return;
        const next = [];
        let changed = false;
        for (let i = 0; i < group.registeredButtons.length; ++i) {
            const cur = group.registeredButtons[i];
            if (cur && cur !== btn)
                next.push(cur);
            else
                changed = true;
        }
        if (changed)
            group.registeredButtons = next;
    }

    function uncheckExcept(activeButton) {
        for (let i = 0; i < group.registeredButtons.length; ++i) {
            const b = group.registeredButtons[i];
            if (b && b !== activeButton && (b.buttonGroup === undefined || b.buttonGroup === group) && b.checked !== undefined)
                b.checked = false;
        }
    }

    function syncDeclarativeButton(b) {
        if (!b)
            return;
        group.addButton(b);
        if (b.buttonGroup !== undefined && b.buttonGroup !== group)
            b.buttonGroup = group;
    }

    onButtonsChanged: {
        for (let i = 0; i < group.buttons.length; ++i) {
            group.syncDeclarativeButton(group.buttons[i]);
        }
    }
}
