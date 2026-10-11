# Animation and states

Use `State` and `PropertyChanges` to describe alternate property values, then
`Transition` and property animations to interpolate changes. `NumberAnimation`
and `ColorAnimation` animate integer geometry and colors. `Behavior on property`
is optional: configure `QAMUI_ENABLE_BEHAVIOR=ON` with Qt QmlPrivate available.
Applications that do not import Behavior work in either configuration.

Register time-dependent Runner scenarios with `CLOCK VIRTUAL` and advance Qamui
time using `advanceTime()`. Assert a committed frame at each meaningful state;
wall-clock waits do not advance the virtual clock.

See [animation](../../examples/topics/animation/),
[states and transitions](../../examples/topics/state_transitions/),
[State](../qml-types/state.md), and [Runner](../cpp-classes/runner.md).
