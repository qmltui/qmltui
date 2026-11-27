#pragma once

#include <QtCore/qglobal.h>

// Symbol visibility for the QmlTui libraries. Each library defines its own
// *_LIBRARY macro privately while it is being built, or *_STATIC publicly when
// built as a static archive; shared-library consumers see the import
// declaration. On non-Windows platforms these expand to default visibility.
#if defined(QMLTUI_CORE_STATIC)
#define QMLTUI_CORE_EXPORT
#elif defined(QMLTUI_CORE_LIBRARY)
#define QMLTUI_CORE_EXPORT Q_DECL_EXPORT
#else
#define QMLTUI_CORE_EXPORT Q_DECL_IMPORT
#endif

#if defined(QMLTUI_RUNNER_STATIC)
#define QMLTUI_RUNNER_EXPORT
#elif defined(QMLTUI_RUNNER_LIBRARY)
#define QMLTUI_RUNNER_EXPORT Q_DECL_EXPORT
#else
#define QMLTUI_RUNNER_EXPORT Q_DECL_IMPORT
#endif

#if defined(QMLTUI_CONTROLS_STATIC)
#define QMLTUI_CONTROLS_EXPORT
#elif defined(QMLTUI_CONTROLS_LIBRARY)
#define QMLTUI_CONTROLS_EXPORT Q_DECL_EXPORT
#else
#define QMLTUI_CONTROLS_EXPORT Q_DECL_IMPORT
#endif

#if defined(QMLTUI_IMAGE_STATIC)
#define QMLTUI_IMAGE_EXPORT
#elif defined(QMLTUI_IMAGE_LIBRARY)
#define QMLTUI_IMAGE_EXPORT Q_DECL_EXPORT
#else
#define QMLTUI_IMAGE_EXPORT Q_DECL_IMPORT
#endif
