#pragma once

// Terminal graphics protocols for the Image item.
//
// This is the extension seam for pixel-accurate image output. Today only the
// kitty graphics protocol is implemented; sixel and the iTerm2 inline-image
// protocol are meant to slot in here without touching Image. To add one:
//   1. detect it into TerminalCapabilities (sixel is already detected),
//   2. add its case to resolve() in preference order,
//   3. add its encoder(s) beside the kitty ones,
//   4. teach Image::paint() to call them for that Protocol.
// Everything else, the Auto render mode and the capability plumbing, already
// accounts for more than one protocol.

#include <Qamui/Core/items/Item.h>                 // ClipRect
#include <Qamui/Core/primitives/TextGridBuffer.h>
#include <Qamui/Core/primitives/TerminalCapabilities.h>

#include <QByteArray>
#include <QImage>

namespace qamui::graphics {

enum class Protocol {
    None,    // no pixel protocol; the caller falls back to half-block or braille
    Kitty,   // kitty graphics with Unicode virtual placements (implemented)
    Sixel,   // detected, not yet rendered; resolve() will return it once encoded
};

// The best pixel protocol the terminal supports, in preference order. Kitty
// needs true colour because the Unicode placeholder cells carry the image id
// in a 24-bit foreground colour that must not be quantised.
Protocol resolve(const TerminalCapabilities& capabilities);

// A process-wide image id in [1, 0xFFFFFF]. Stays in 24 bits so it fits a
// single true-colour foreground with no high-byte diacritic.
quint32 nextImageId();

// --- kitty graphics encoders. Each returns a complete APC byte sequence. ---

// Uploads `image` as zlib-compressed RGBA under `imageId`, chunked at the
// protocol's 4096-byte base64 limit, with responses suppressed. Creates the image only; placement
// is separate so a resize re-places without re-uploading.
QByteArray kittyTransmit(quint32 imageId, const QImage& image);

// The pixel size to upload for a source shown in a placement of
// `placementPixels` (cols x rows times the cell size, invalid when the cell
// size is unknown). The terminal scales the stored image into the placement
// with a filter of its own choosing, often nearest-neighbour, so a small
// picture magnified onto a large placement comes out blocky. With `smooth`
// the source is pre-scaled here to the placement size, never beyond
// `maxEdge` per side; without it, or when shrinking or the cell size is
// unknown, the source goes as is (capped at maxEdge) and the terminal scales.
QSize kittyUploadSize(const QSize& source, const QSize& placementPixels, bool smooth, int maxEdge = 2048);

// A Unicode virtual placement of `imageId` sized to cols x rows cells. The
// terminal scales the image into that rectangle; the placeholder cells select
// which part shows where. Reuses placement id 1 so resizing replaces the
// existing geometry even on terminals that retain placements across uploads.
QByteArray kittyPlacement(quint32 imageId, int cols, int rows);

// --- sixel encoder. Paints at the cursor, so the caller positions it. ---

// Encodes `image` as a sixel data string (DCS ... ST). Alpha below 128 is
// left unpainted so the cell background shows through. The image is emitted at
// its own pixel size; the caller scales it to the target first.
QByteArray sixelEncode(const QImage& image);

// Frees `imageId` and every placement of it.
QByteArray kittyDelete(quint32 imageId);

// Fills the cols x rows rectangle at (x0, y0) with Unicode placeholder cells
// that reference `imageId`, clipped to `clip`. Each cell is U+10EEEE plus a row
// and a column combining mark, with the id in its foreground colour.
void writeKittyPlaceholders(TextGridBuffer& buffer, int x0, int y0,
                            int cols, int rows, quint32 imageId, const ClipRect& clip);

} // namespace qamui::graphics
