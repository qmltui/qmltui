#pragma once

#include <QtCore/qglobal.h>

// Symbol visibility for the Qtui libraries. Each library defines its own
// *_LIBRARY macro privately while it is being built; consumers see the import
// declaration. On non-Windows platforms these expand to default visibility.
#if defined(QTUI_CORE_LIBRARY)
#  define QTUI_CORE_EXPORT Q_DECL_EXPORT
#else
#  define QTUI_CORE_EXPORT Q_DECL_IMPORT
#endif

#if defined(QTUI_RUNNER_LIBRARY)
#  define QTUI_RUNNER_EXPORT Q_DECL_EXPORT
#else
#  define QTUI_RUNNER_EXPORT Q_DECL_IMPORT
#endif

#if defined(QTUI_CONTROLS_LIBRARY)
#  define QTUI_CONTROLS_EXPORT Q_DECL_EXPORT
#else
#  define QTUI_CONTROLS_EXPORT Q_DECL_IMPORT
#endif

#if defined(QTUI_IMAGE_LIBRARY)
#  define QTUI_IMAGE_EXPORT Q_DECL_EXPORT
#else
#  define QTUI_IMAGE_EXPORT Q_DECL_IMPORT
#endif

#if defined(QTUI_TERMINALHOST_LIBRARY)
#  define QTUI_TERMINALHOST_EXPORT Q_DECL_EXPORT
#else
#  define QTUI_TERMINALHOST_EXPORT Q_DECL_IMPORT
#endif
