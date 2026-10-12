# Styling and colors


Qamui provides its own QtGui-free `rgba` value type. Use the `Qamui` singleton
to construct a value from normalized red, green, blue, and alpha channels:

```qml
import QtQml
import Qamui

Terminal {
    property rgba accent: Qamui.rgba(0.2, 0.4, 0.6, 1.0)
    Text { text: "Custom color"; color: parent.accent }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) { Qt.quit(); event.accepted = true }
    }
}
```

This property contains a `qamui::Color`, not a `QColor`. The built-in QML
`color` type and `Qt.rgba()` belong to QtGui and are unavailable in a
QtCore/QtQml-only Qamui application. String literals remain supported for Qamui
properties whose declared C++ type is `qamui::Color`.

Two kinds of string are accepted, and they behave differently on screen:

- Hex values such as `"#2cde85"` are literal RGB. On truecolor terminals they
  render exactly; on 256- and 16-color terminals they are quantised.
- The sixteen ANSI palette names (`black`, `maroon`, `green`, `olive`, `navy`,
  `purple`, `teal`, `silver`, `gray`, `red`, `lime`, `yellow`, `blue`,
  `magenta`, `cyan`, `white`) and the explicit form `"ansi:N"` are palette
  indices, not RGB. They are emitted as SGR 30-37 and 90-97 at every color
  depth, so the user's terminal theme decides how they look. Other CSS names (`orange`, `steelblue`,
  ...) are literal RGB.
- `default` (also spelled `transparent`) means the terminal's default foreground or background.


## Component styling

Core items and Controls use the terminal's default foreground and background.
`"default"` delegates to the terminal; named ANSI colors follow its color table,
while hex colors request specific RGB values. `"transparent"` is an alias for
`"default"`; transparent item backgrounds leave the surface underneath visible.
On a `Canvas` this means `setPixelColor(x, y, "transparent")` paints a set pixel
in the terminal's default ink, not a hole; erase a pixel with
`setPixel(x, y, false)` or the whole surface with `clear()`.

There is no framework palette, inherited color scheme, or theme singleton.
Set ordinary properties to customize individual components, or define reusable
QML components for application-wide styling:

```qml
// AppButton.qml; import Qamui and Qamui.Controls in this component.
Button {
    background: Rectangle {
        color: "#202830"
        border.color: "cyan"
        border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
    }
    textColor: "#ffffff"
}
```

| Component | Color overrides |
| :--- | :--- |
| Text, Paragraph, TextInput, TextArea, Canvas, BusyIndicator, Sparkline | `color` |
| Rectangle and rectangle-based controls | `color`, `border.color` |
| Button, CheckBox, RadioButton, Switch, Slider | `textColor` |
| Panel | `titleColor` |
| Dialog | `textColor`, `titleColor`, `background.color`, `background.border.color` |
| ProgressBar | `color`, optional gradient/threshold colors |
| TabBar | `textColor`, `color`, `selectedTextColor`, `selectionColor` (background) |
| Toast | `textColor`, `color`, `infoColor`, `successColor`, `warningColor`, `errorColor` |
| ToolTip attached properties | `ToolTip.textColor`, `ToolTip.color` |

Focus uses borders or bold text. Disabled text is dimmed using SGR 2, preserving
its configured color. `Text.dim` also exposes this attribute directly. TextInput
hints use the configured text color with dimming unless `placeholderColor` is
set. Rich Paragraph links use underline and inline code uses bold.

TabBar and the built-in TableView/TreeView selection render with reverse video
until selection colors are explicitly supplied. TextInput also supports explicit
selection, selected-text, placeholder, and cursor colors. Custom delegates own
their appearance, including selection styling.

Core `Palette`, `TerminalPalette`, `ResolvedPalette`, `Item.palette`, and Controls
`DarkPalette` are no longer provided. Replace earlier palette bindings with
ordinary color bindings or application-defined components.


See the [ANSI styling example](../../examples/topics/ansi_styling/).
