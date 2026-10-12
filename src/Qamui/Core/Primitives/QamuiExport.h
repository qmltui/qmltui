#pragma once

#include <QtCore/qglobal.h>

// Symbol visibility for the Qamui libraries. Each library defines its own
// *_LIBRARY macro privately while it is being built; consumers see the import
// declaration. On non-Windows platforms these expand to default visibility.
#if defined(QAMUI_CORE_LIBRARY)
#  define QAMUI_CORE_EXPORT Q_DECL_EXPORT
#else
#  define QAMUI_CORE_EXPORT Q_DECL_IMPORT
#endif

#if defined(QAMUI_RUNNER_LIBRARY)
#  define QAMUI_RUNNER_EXPORT Q_DECL_EXPORT
#else
#  define QAMUI_RUNNER_EXPORT Q_DECL_IMPORT
#endif

#if defined(QAMUI_CONTROLS_LIBRARY)
#  define QAMUI_CONTROLS_EXPORT Q_DECL_EXPORT
#else
#  define QAMUI_CONTROLS_EXPORT Q_DECL_IMPORT
#endif

#if defined(QAMUI_IMAGE_LIBRARY)
#  define QAMUI_IMAGE_EXPORT Q_DECL_EXPORT
#else
#  define QAMUI_IMAGE_EXPORT Q_DECL_IMPORT
#endif

#if defined(QAMUI_TERMINALHOST_LIBRARY)
#  define QAMUI_TERMINALHOST_EXPORT Q_DECL_EXPORT
#else
#  define QAMUI_TERMINALHOST_EXPORT Q_DECL_IMPORT
#endif
