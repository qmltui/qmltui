#pragma once

// Terminal graphics protocols for the Image item.
//
// This is the extension seam for pixel-accurate image output. Kitty and Sixel
// are implemented here; additional protocols (such as iTerm2 inline images) can
// slot in without touching Image's public API. To add one:
//   1. detect it into TerminalCapabilities,
//   2. add its case to resolve() in preference order,
//   3. add its encoder(s) beside the kitty and sixel ones,
//   4. teach Image::paint() to call them for that Protocol.
// Everything else, the Auto render mode and the capability plumbing, already
// accounts for multiple protocols.

#include <QmlTui/Core/Items/Item.h>                 // ClipRect
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QmlTui/Core/Primitives/TerminalCapabilities.h>
#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QByteArray>
#include <QImage>

namespace qmltui::graphics {

enum class Protocol : uint8_t {
    None,  // no pixel protocol; the caller falls back to half-block, braille, or ASCII
    Kitty, // kitty graphics with Unicode virtual placements
    Sixel, // sixel graphics (DCS ... ST) painted at the cursor after cell diffing
};

// The best pixel protocol the terminal supports, in preference order. Kitty
// needs true colour because the Unicode placeholder cells carry the image id
// in a 24-bit foreground colour that must not be quantised.
QMLTUI_IMAGE_EXPORT Protocol resolve(const TerminalCapabilities& capabilities);

// A process-wide image id in [1, 0xFFFFFF]. Stays in 24 bits so it fits a
// single true-colour foreground with no high-byte diacritic.
QMLTUI_IMAGE_EXPORT quint32 nextImageId();

// --- kitty graphics encoders. Each returns a complete APC byte sequence. ---

// Uploads `image` as zlib-compressed RGBA under `imageId`, chunked at the
// protocol's 4096-byte base64 limit, with responses suppressed. Creates the image only; placement
// is separate so a resize re-places without re-uploading.
QMLTUI_IMAGE_EXPORT QByteArray kittyTransmit(quint32 imageId, const QImage& image);

// The pixel size to upload for a source shown in a placement of
// `placementPixels` (cols x rows times the cell size, invalid when the cell
// size is unknown). The terminal scales the stored image into the placement
// with a filter of its own choosing, often nearest-neighbour, so a small
// picture magnified onto a large placement comes out blocky. With `smooth`
// the source is pre-scaled here to the placement size, never beyond
// `maxEdge` per side; without it, or when shrinking or the cell size is
// unknown, the source goes as is (capped at maxEdge) and the terminal scales.
QMLTUI_IMAGE_EXPORT QSize kittyUploadSize(const QSize& source, const QSize& placementPixels, bool smooth, int maxEdge = 2048);

// A Unicode virtual placement of `imageId` sized to cols x rows cells. The
// terminal scales the image into that rectangle; the placeholder cells select
// which part shows where. Reuses placement id 1 so resizing replaces the
// existing geometry even on terminals that retain placements across uploads.
QMLTUI_IMAGE_EXPORT QByteArray kittyPlacement(quint32 imageId, int cols, int rows);

// --- sixel encoder. Paints at the cursor, so the caller positions it. ---

// Encodes `image` as a sixel data string (DCS ... ST). Alpha below 128 is
// left unpainted so the cell background shows through. The image is emitted at
// its own pixel size; the caller scales it to the target first.
QMLTUI_IMAGE_EXPORT QByteArray sixelEncode(const QImage& image);

// Frees `imageId` and every placement of it.
QMLTUI_IMAGE_EXPORT QByteArray kittyDelete(quint32 imageId);

// Fills the cols x rows rectangle at (x0, y0) with Unicode placeholder cells
// that reference `imageId`, clipped to `clip`. Each cell is U+10EEEE plus a row
// and a column combining mark, with the id in its foreground colour.
QMLTUI_IMAGE_EXPORT void writeKittyPlaceholders(TextGridBuffer& buffer, int x0, int y0,
                                               int cols, int rows, quint32 imageId, const ClipRect& clip);

} // namespace qmltui::graphics
