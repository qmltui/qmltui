pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// One choice out of a group, drawn as `(*)` or `( )` followed by its label.
// The indicator is drawn in reverse video while the control has active focus;
// indicator and label are bold while focused, hovered or highlighted.
// autoExclusive: activating a checked radio button keeps it checked. Put
// the buttons of one choice in a ButtonGroup; checking one, by the user or
// from code, unchecks the others.
AbstractButton {
    id: control

    property ButtonGroup buttonGroup: null

    checkable: true
    autoExclusive: true
    contentItem: Row {
        spacing: 1
        Text {
            text: control.checked ? "(*)" : "( )"
            color: control.textColor
            bold: control.activeFocus || control.hovered || control.highlighted
            reversed: control.activeFocus
        }
        Text {
            visible: control.text !== ""
            text: control.text
            color: control.textColor
            bold: control.activeFocus || control.hovered || control.highlighted
        }
    }

    onCheckedChanged: {
        if (control.checked)
            d.uncheckGroup();
    }

    onButtonGroupChanged: {
        if (d.prevGroup && d.prevGroup !== control.buttonGroup)
            d.prevGroup.removeButton(control);
        d.prevGroup = control.buttonGroup;
        if (control.buttonGroup)
            control.buttonGroup.addButton(control);
        if (control.checked)
            d.uncheckGroup();
    }

    Component.onCompleted: {
        if (control.buttonGroup) {
            d.prevGroup = control.buttonGroup;
            control.buttonGroup.addButton(control);
            if (control.checked)
                d.uncheckGroup();
        }
    }

    Component.onDestruction: {
        if (d.prevGroup)
            d.prevGroup.removeButton(control);
    }

    QtObject {
        id: d

        property ButtonGroup prevGroup: null

        function uncheckGroup() {
            if (control.buttonGroup)
                control.buttonGroup.uncheckExcept(control);
        }
    }
}
