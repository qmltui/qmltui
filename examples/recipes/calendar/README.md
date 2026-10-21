# Calendar

A pure-QML month-grid date picker (`Calendar.qml`) paired with a day-notes panel and `StatusBadge.qml`.

- **`Calendar.qml`** builds a 6×7 day grid with nested `Column`, `Row`, and `Repeater` items, highlights `todayDate` and `selectedDate`, and emits `dateSelected(date)`.
- **Keyboard navigation**: Arrow keys (or `H`/`J`/`K`/`L`) move by day or week, `PageUp`/`PageDown` step months, `[`/`]` step years, and `T`/`Home` jumps to today.
- **Contextual focus**: `Escape` leaves the date-jump or note `TextInput` when focused, and quits the application when neither input has active focus.

```bash
cmake --build build --target qmltui_calendar_demo
./build/examples/recipes/calendar/qmltui_calendar_demo
ctest --test-dir build -R '^calendar_smoke$' --output-on-failure
```

Copy `Calendar.qml` into your application module to use the date picker standalone.
