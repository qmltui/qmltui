# Qt Docs MCP capture: Qt Quick `Keys` and `KeyEvent`

Generated: 2026-07-14

This is a reproduction artifact for improving the Qt Docs MCP. Every documentation response below came directly from the Qt Docs MCP. No direct SQLite, QCH, Python, filesystem documentation extraction, or external web lookup was used to generate the captured responses.

## Observed issue

The MCP locates the correct Qt Quick pages and returns their descriptions and API member lists, but `qt_lookup` with `detail: "full"` omits detailed property, signal, method, example, and behavioral sections. Looking up a page anchor returns the same page-level summary rather than the anchored section. Human-readable title lookup also fails even though `qt_search` returns that exact title.

Expected missing material from the full `Keys` page includes the documented processing order, default priority, `forwardTo` behavior, automatic acceptance by specific key handlers, and detailed `shortcutOverride` behavior.

## qt_versions

Request:

````json
{}
````

Response:

````text
# Available Qt Documentation Versions

- `Qt-6.11.1` <- **active baseline**
````

## qt_diagnostics

Request:

````json
{}
````

Response:

````text
# Qt Documentation System Diagnostics

**Active Version:** Qt-6.11.1
**Active Baseline:** Qt-6.11.1
**Available Versions:** Qt-6.11.1
**Build-time Qt Prefix:** /home/yosri/Qt/6.11.1/gcc_64
**Build-time Qt Docs Path:** /home/yosri/Qt/6.11.1/gcc_64/doc
**Collection File:** /home/yosri/.cache/qt_mcp/qt_docs_Qt_6_11_1.qhc
**Read-only mode:** False
**Search Index:** Ready

**Registered Documentation:** 43 namespaces

````

## qt_search — Keys

Request:

````json
{
  "query": "Keys",
  "is_qml": true,
  "max_results": 50,
  "version": "Qt-6.11.1"
}
````

Response:

````text
# Search Results for 'Keys'

Showing 1-2 of 2 results (Version: Qt-6.11.1).

1. **Keys QML Type | Qt Quick | Qt 6.11.1**
   URL: `qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keys.html`
   Location:  > Qt Quick > QML Types > Keys Type Reference
2. **List of All Members for Keys | Qt Quick | Qt 6.11.1**
   URL: `qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keys-members.html`
   Location:  > Qt Quick > QML Types > Keys Type Reference > List of all members
````

## qt_lookup — human-readable page title

Request:

````json
{
  "symbol": "Keys QML Type",
  "detail": "full",
  "include_inherited": false,
  "version": "Qt-6.11.1"
}
````

Response:

````text
[isError: true]
No documentation found for 'Keys QML Type' in Qt Qt-6.11.1. The module might not be installed.
````

## qt_lookup — Keys qthelp URL

Request:

````json
{
  "symbol": "qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keys.html",
  "detail": "full",
  "include_inherited": false,
  "version": "Qt-6.11.1"
}
````

Response:

````text
**Location:**  > Qt Quick > QML Types > Keys Type Reference

### Metadata
- **Availability:** Available in Qt Qt-6.11.1
- **Status:** Active

# Keys QML Type

*Source: qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keys.html*

## Description

Provides key handling to Items. More...

All visual primitives support key handling via the Keys attaching type. Keys can be handled via the onPressed and onReleased signal properties.


The signal properties have a KeyEvent parameter, named *event* which contains details of the event. If a key is handled *event.accepted* should be set to true to prevent the event from propagating up the item hierarchy.

## Properties

- **enabled** : bool

- **forwardTo** : list<Item>

- **priority** : enumeration

## Signals

- **asteriskPressed**(KeyEvent *event*)

- **backPressed**(KeyEvent *event*)

- **backtabPressed**(KeyEvent *event*)

- **callPressed**(KeyEvent *event*)

- **cancelPressed**(KeyEvent *event*)

- **context1Pressed**(KeyEvent *event*)

- **context2Pressed**(KeyEvent *event*)

- **context3Pressed**(KeyEvent *event*)

- **context4Pressed**(KeyEvent *event*)

- **deletePressed**(KeyEvent *event*)

- **digit0Pressed**(KeyEvent *event*)

- **digit1Pressed**(KeyEvent *event*)

- **digit2Pressed**(KeyEvent *event*)

- **digit3Pressed**(KeyEvent *event*)

- **digit4Pressed**(KeyEvent *event*)

- **digit5Pressed**(KeyEvent *event*)

- **digit6Pressed**(KeyEvent *event*)

- **digit7Pressed**(KeyEvent *event*)

- **digit8Pressed**(KeyEvent *event*)

- **digit9Pressed**(KeyEvent *event*)

- **downPressed**(KeyEvent *event*)

- **enterPressed**(KeyEvent *event*)

- **escapePressed**(KeyEvent *event*)

- **flipPressed**(KeyEvent *event*)

- **hangupPressed**(KeyEvent *event*)

- **leftPressed**(KeyEvent *event*)

- **menuPressed**(KeyEvent *event*)

- **noPressed**(KeyEvent *event*)

- **pressed**(KeyEvent *event*)

- **released**(KeyEvent *event*)

- **returnPressed**(KeyEvent *event*)

- **rightPressed**(KeyEvent *event*)

- **selectPressed**(KeyEvent *event*)

- **shortcutOverride**(KeyEvent *event*)

- **spacePressed**(KeyEvent *event*)

- **tabPressed**(KeyEvent *event*)

- **upPressed**(KeyEvent *event*)

- **volumeDownPressed**(KeyEvent *event*)

- **volumeUpPressed**(KeyEvent *event*)

- **yesPressed**(KeyEvent *event*)

````

## qt_lookup — Keys priority anchor

Request:

````json
{
  "symbol": "qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keys.html#priority-prop",
  "detail": "full",
  "include_inherited": false,
  "version": "Qt-6.11.1"
}
````

Response:

````text
**Location:**  > Qt Quick > QML Types > Keys Type Reference

### Metadata
- **Availability:** Available in Qt Qt-6.11.1
- **Status:** Active

# Keys QML Type

*Source: qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keys.html#priority-prop*

## Description

Provides key handling to Items. More...

All visual primitives support key handling via the Keys attaching type. Keys can be handled via the onPressed and onReleased signal properties.


The signal properties have a KeyEvent parameter, named *event* which contains details of the event. If a key is handled *event.accepted* should be set to true to prevent the event from propagating up the item hierarchy.

## Properties

- **enabled** : bool

- **forwardTo** : list<Item>

- **priority** : enumeration

## Signals

- **asteriskPressed**(KeyEvent *event*)

- **backPressed**(KeyEvent *event*)

- **backtabPressed**(KeyEvent *event*)

- **callPressed**(KeyEvent *event*)

- **cancelPressed**(KeyEvent *event*)

- **context1Pressed**(KeyEvent *event*)

- **context2Pressed**(KeyEvent *event*)

- **context3Pressed**(KeyEvent *event*)

- **context4Pressed**(KeyEvent *event*)

- **deletePressed**(KeyEvent *event*)

- **digit0Pressed**(KeyEvent *event*)

- **digit1Pressed**(KeyEvent *event*)

- **digit2Pressed**(KeyEvent *event*)

- **digit3Pressed**(KeyEvent *event*)

- **digit4Pressed**(KeyEvent *event*)

- **digit5Pressed**(KeyEvent *event*)

- **digit6Pressed**(KeyEvent *event*)

- **digit7Pressed**(KeyEvent *event*)

- **digit8Pressed**(KeyEvent *event*)

- **digit9Pressed**(KeyEvent *event*)

- **downPressed**(KeyEvent *event*)

- **enterPressed**(KeyEvent *event*)

- **escapePressed**(KeyEvent *event*)

- **flipPressed**(KeyEvent *event*)

- **hangupPressed**(KeyEvent *event*)

- **leftPressed**(KeyEvent *event*)

- **menuPressed**(KeyEvent *event*)

- **noPressed**(KeyEvent *event*)

- **pressed**(KeyEvent *event*)

- **released**(KeyEvent *event*)

- **returnPressed**(KeyEvent *event*)

- **rightPressed**(KeyEvent *event*)

- **selectPressed**(KeyEvent *event*)

- **shortcutOverride**(KeyEvent *event*)

- **spacePressed**(KeyEvent *event*)

- **tabPressed**(KeyEvent *event*)

- **upPressed**(KeyEvent *event*)

- **volumeDownPressed**(KeyEvent *event*)

- **volumeUpPressed**(KeyEvent *event*)

- **yesPressed**(KeyEvent *event*)

````

## qt_lookup — Keys members page

Request:

````json
{
  "symbol": "qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keys-members.html",
  "detail": "full",
  "include_inherited": true,
  "version": "Qt-6.11.1"
}
````

Response:

````text
**Location:**  > Qt Quick > QML Types > Keys Type Reference > List of all members

### Metadata
- **Availability:** Available in Qt Qt-6.11.1
- **Status:** Active

# List of All Members for Keys

*Source: qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keys-members.html*

## Description

This is the complete list of members for Keys, including inherited members.

````

## qt_lookup — KeyEvent qthelp URL

Request:

````json
{
  "symbol": "qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keyevent.html",
  "detail": "full",
  "include_inherited": false,
  "version": "Qt-6.11.1"
}
````

Response:

````text
**Location:**  > Qt Quick > QML Types > KeyEvent Type Reference

### Metadata
- **Availability:** Available in Qt Qt-6.11.1
- **Status:** Active

# KeyEvent QML Type

*Source: qthelp://org.qt-project.qtquick.6111/qtquick/qml-qtquick-keyevent.html*

## Description

Provides information about a key event. More...

For example, the following changes the Item's state property when the Enter key is pressed:


```qml
Item {
     focus: true
     Keys.onPressed: (event)=> { if (event.key == Qt.Key_Enter) state = 'ShowDetails'; }
 }
```

## Methods

- bool **matches**(StandardKey *matchKey*)

## Properties

- **accepted** : bool

- **count** : int

- **isAutoRepeat** : bool

- **key** : int

- **modifiers** : int

- **nativeScanCode** : quint32

- **text** : string

````

## Suggested MCP checks

- Verify whether the HTML-to-Markdown extraction stops before detailed member documentation.
- Make `detail: "full"` include property, signal, method, example, and behavioral sections.
- Resolve exact search-result titles such as `Keys QML Type` in `qt_lookup`.
- When a qthelp URL contains an anchor, return or prioritize that anchored section.
- Populate the “List of All Members” page rather than returning only its heading.

