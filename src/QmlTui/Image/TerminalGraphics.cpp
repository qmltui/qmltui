#include <QmlTui/Image/TerminalGraphics.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>

#include <atomic>
#include <vector>

namespace qmltui::graphics {

namespace {

// The kitty row/column diacritics table (Unicode combining marks), in the
// order the protocol assigns to indices 0, 1, 2, ... A placeholder cell at
// image row r, column c is U+10EEEE followed by diacritic[r] then diacritic[c].
// 297 entries; images are capped well below that.
constexpr char32_t kDiacritics[] = {
    0x0305,  0x030D,  0x030E,  0x0310,  0x0312,  0x033D,  0x033E,  0x033F,  0x0346,  0x034A,
    0x034B,  0x034C,  0x0350,  0x0351,  0x0352,  0x0357,  0x035B,  0x0363,  0x0364,  0x0365,
    0x0366,  0x0367,  0x0368,  0x0369,  0x036A,  0x036B,  0x036C,  0x036D,  0x036E,  0x036F,
    0x0483,  0x0484,  0x0485,  0x0486,  0x0487,  0x0592,  0x0593,  0x0594,  0x0595,  0x0597,
    0x0598,  0x0599,  0x059C,  0x059D,  0x059E,  0x059F,  0x05A0,  0x05A1,  0x05A8,  0x05A9,
    0x05AB,  0x05AC,  0x05AF,  0x05C4,  0x0610,  0x0611,  0x0612,  0x0613,  0x0614,  0x0615,
    0x0616,  0x0617,  0x0657,  0x0658,  0x0659,  0x065A,  0x065B,  0x065D,  0x065E,  0x06D6,
    0x06D7,  0x06D8,  0x06D9,  0x06DA,  0x06DB,  0x06DC,  0x06DF,  0x06E0,  0x06E1,  0x06E2,
    0x06E4,  0x06E7,  0x06E8,  0x06EB,  0x06EC,  0x0730,  0x0732,  0x0733,  0x0735,  0x0736,
    0x073A,  0x073D,  0x073F,  0x0740,  0x0741,  0x0743,  0x0745,  0x0747,  0x0749,  0x074A,
    0x07EB,  0x07EC,  0x07ED,  0x07EE,  0x07EF,  0x07F0,  0x07F1,  0x07F3,  0x0816,  0x0817,
    0x0818,  0x0819,  0x081B,  0x081C,  0x081D,  0x081E,  0x081F,  0x0820,  0x0821,  0x0822,
    0x0823,  0x0825,  0x0826,  0x0827,  0x0829,  0x082A,  0x082B,  0x082C,  0x082D,  0x0951,
    0x0953,  0x0954,  0x0F82,  0x0F83,  0x0F86,  0x0F87,  0x135D,  0x135E,  0x135F,  0x17DD,
    0x193A,  0x1A17,  0x1A75,  0x1A76,  0x1A77,  0x1A78,  0x1A79,  0x1A7A,  0x1A7B,  0x1A7C,
    0x1B6B,  0x1B6D,  0x1B6E,  0x1B6F,  0x1B70,  0x1B71,  0x1B72,  0x1B73,  0x1CD0,  0x1CD1,
    0x1CD2,  0x1CDA,  0x1CDB,  0x1CE0,  0x1DC0,  0x1DC1,  0x1DC3,  0x1DC4,  0x1DC5,  0x1DC6,
    0x1DC7,  0x1DC8,  0x1DC9,  0x1DCB,  0x1DCC,  0x1DD1,  0x1DD2,  0x1DD3,  0x1DD4,  0x1DD5,
    0x1DD6,  0x1DD7,  0x1DD8,  0x1DD9,  0x1DDA,  0x1DDB,  0x1DDC,  0x1DDD,  0x1DDE,  0x1DDF,
    0x1DE0,  0x1DE1,  0x1DE2,  0x1DE3,  0x1DE4,  0x1DE5,  0x1DE6,  0x1DFE,  0x20D0,  0x20D1,
    0x20D4,  0x20D5,  0x20D6,  0x20D7,  0x20DB,  0x20DC,  0x20E1,  0x20E7,  0x20E9,  0x20F0,
    0x2CEF,  0x2CF0,  0x2CF1,  0x2DE0,  0x2DE1,  0x2DE2,  0x2DE3,  0x2DE4,  0x2DE5,  0x2DE6,
    0x2DE7,  0x2DE8,  0x2DE9,  0x2DEA,  0x2DEB,  0x2DEC,  0x2DED,  0x2DEE,  0x2DEF,  0x2DF0,
    0x2DF1,  0x2DF2,  0x2DF3,  0x2DF4,  0x2DF5,  0x2DF6,  0x2DF7,  0x2DF8,  0x2DF9,  0x2DFA,
    0x2DFB,  0x2DFC,  0x2DFD,  0x2DFE,  0x2DFF,  0xA66F,  0xA67C,  0xA67D,  0xA6F0,  0xA6F1,
    0xA8E0,  0xA8E1,  0xA8E2,  0xA8E3,  0xA8E4,  0xA8E5,  0xA8E6,  0xA8E7,  0xA8E8,  0xA8E9,
    0xA8EA,  0xA8EB,  0xA8EC,  0xA8ED,  0xA8EE,  0xA8EF,  0xA8F0,  0xA8F1,  0xAAB0,  0xAAB2,
    0xAAB3,  0xAAB7,  0xAAB8,  0xAABE,  0xAABF,  0xAAC1,  0xFE20,  0xFE21,  0xFE22,  0xFE23,
    0xFE24,  0xFE25,  0xFE26,  0x10A0F, 0x10A38, 0x1D185, 0x1D186, 0x1D187, 0x1D188, 0x1D189,
    0x1D1AA, 0x1D1AB, 0x1D1AC, 0x1D1AD, 0x1D242, 0x1D243, 0x1D244,
};
constexpr int kDiacriticCount = static_cast<int>(std::size(kDiacritics));

constexpr char32_t kPlaceholder = 0x10EEEE;

} // namespace

Protocol resolve(const TerminalCapabilities& capabilities) {
    // Kitty first: it composites per cell, so it survives clipping, scrolling
    // and overlap. It needs true colour for the id-in-foreground scheme.
    if (capabilities.kittyGraphics && capabilities.colorDepth == ColorDepth::TrueColor)
        return Protocol::Kitty;
    if (capabilities.sixelGraphics) return Protocol::Sixel;
    return Protocol::None;
}

quint32 nextImageId() {
    static std::atomic<quint32> counter{1};
    quint32 id = counter.fetch_add(1, std::memory_order_relaxed);
    id &= 0xFFFFFFu;
    if (id == 0) id = counter.fetch_add(1, std::memory_order_relaxed) & 0xFFFFFFu;
    return id == 0 ? 1u : id;
}

QSize kittyUploadSize(const QSize& source, const QSize& placementPixels, bool smooth, int maxEdge) {
    if (source.isEmpty()) return source;
    QSize size = source;
    const bool magnifies =
        placementPixels.isValid() &&
        (placementPixels.width() > source.width() || placementPixels.height() > source.height());
    if (smooth && magnifies) size = placementPixels;
    if (size.width() > maxEdge || size.height() > maxEdge)
        size = size.scaled(maxEdge, maxEdge, Qt::KeepAspectRatio);
    return size;
}

QByteArray kittyTransmit(quint32 imageId, const QImage& image) {
    if (image.isNull()) return {};
    // PNG's per-row filter selection is expensive for live, full-resolution
    // frames. Kitty also accepts lossless zlib-compressed RGBA directly.
    // RGBA8888 fixes the byte order independently of the host's endianness.
    const QImage rgba = image.convertToFormat(QImage::Format_RGBA8888);
    const qsizetype rowBytes = qsizetype(rgba.width()) * 4;
    QByteArray pixels;
    if (rgba.bytesPerLine() == rowBytes) {
        pixels = QByteArray::fromRawData(reinterpret_cast<const char*>(rgba.constBits()),
                                         rgba.sizeInBytes());
    } else {
        // Externally backed QImages can have padding even in RGBA8888.
        pixels.reserve(rowBytes * rgba.height());
        for (int y = 0; y < rgba.height(); ++y)
            pixels.append(reinterpret_cast<const char*>(rgba.constScanLine(y)), rowBytes);
    }
    // qCompress prefixes the zlib stream with a Qt-specific four-byte length.
    // Only the zlib stream belongs on the wire. Level 2 stays on the fast
    // compressor while emitting Dynamic Huffman blocks; zlib-ng's level 1
    // (deflate_quick) emits a single Fixed Huffman block that triggers a
    // 64 KiB window-boundary crash in Ghostty 1.3.1's flate decompressor.
    const QByteArray compressed = qCompress(pixels, 2);
    if (compressed.size() < 4) return {};
    const QByteArray payload =
        QByteArray::fromRawData(compressed.constData() + 4, compressed.size() - 4).toBase64();

    QByteArray out;
    constexpr int chunkSize = 4096;
    const int chunks = static_cast<int>((payload.size() + chunkSize - 1) / chunkSize);
    for (int i = 0; i < chunks; ++i) {
        const int offset = i * chunkSize;
        const int length =
            static_cast<int>((std::min)(qsizetype(chunkSize), payload.size() - offset));
        const bool last = (i == chunks - 1);
        out += "\033_G";
        if (i == 0) {
            // f=32 RGBA, o=z zlib, t=d direct, a=t transmit only, q=2 no reply.
            out += "q=2,a=t,f=32,o=z,t=d,i=" + QByteArray::number(imageId);
            out += ",s=" + QByteArray::number(rgba.width()) +
                   ",v=" + QByteArray::number(rgba.height());
            out += ",m=";
            out += last ? '0' : '1';
        } else {
            out += "m=";
            out += last ? '0' : '1';
        }
        out += ';';
        out += QByteArray::fromRawData(payload.constData() + offset, length);
        out += "\033\\";
    }
    return out;
}

QByteArray kittyPlacement(quint32 imageId, int cols, int rows) {
    if (imageId == 0 || cols <= 0 || rows <= 0) return {};
    // A nonzero placement id replaces the same placement on every update.
    // Ghostty 1.3.1 retains placements when image data is replaced; leaving
    // p unspecified accumulates unnamed placements with stale resize geometry.
    // Each image owns just one virtual placement, so placeholders can select
    // it without an explicit placement id in their underline colour.
    return "\033_Ga=p,U=1,q=2,i=" + QByteArray::number(imageId) +
           ",p=1,c=" + QByteArray::number(cols) + ",r=" + QByteArray::number(rows) + "\033\\";
}

QByteArray kittyDelete(quint32 imageId) {
    // d=I frees the stored image data as well as its placements.
    return "\033_Ga=d,d=I,q=2,i=" + QByteArray::number(imageId) + "\033\\";
}

QByteArray sixelEncode(const QImage& source) {
    if (source.isNull()) return {};
    // Quantise to at most 256 palette entries; sixel addresses colours by index.
    QImage image = source.convertToFormat(QImage::Format_Indexed8, Qt::ThresholdDither);
    const int w = image.width();
    const int h = image.height();
    if (w <= 0 || h <= 0) return {};
    const QList<QRgb> palette = image.colorTable();

    QByteArray out;
    out.reserve(w * h / 4 + 1024);
    out += "\033P0;1q"; // DCS, P2=1: unset pixels stay transparent
    out += "\"1;1;" + QByteArray::number(w) + ';' + QByteArray::number(h); // raster attributes

    // Palette definitions, colour components scaled to 0..100.
    for (int i = 0; i < palette.size(); ++i) {
        const QRgb c = palette[i];
        if (qAlpha(c) < 128) continue; // transparent index: never emitted
        out += '#' + QByteArray::number(i) + ";2;" +
               QByteArray::number((qRed(c) * 100 + 127) / 255) + ';' +
               QByteArray::number((qGreen(c) * 100 + 127) / 255) + ';' +
               QByteArray::number((qBlue(c) * 100 + 127) / 255);
    }

    // Which palette indices are opaque, so transparent pixels are skipped.
    std::vector<bool> opaque(static_cast<size_t>(palette.size()), false);
    for (int i = 0; i < palette.size(); ++i)
        opaque[static_cast<size_t>(i)] = qAlpha(palette[i]) >= 128;

    std::vector<char> sixels(static_cast<size_t>(w));
    std::vector<int> seenStamp(static_cast<size_t>(palette.size()), 0);
    std::vector<int> bandColors;
    bandColors.reserve(static_cast<size_t>(palette.size()));
    int stamp = 0;

    for (int band = 0; band < h; band += 6) {
        const int rowsInBand = (std::min)(6, h - band);
        const uchar* scanLines[6];
        for (int r = 0; r < rowsInBand; ++r)
            scanLines[r] = image.constScanLine(band + r);
        // Collect only the opaque palette indices actually present in this band.
        ++stamp;
        bandColors.clear();
        for (int r = 0; r < rowsInBand; ++r) {
            const uchar* line = scanLines[r];
            for (int x = 0; x < w; ++x) {
                const int idx = line[x];
                if (idx < palette.size() && opaque[static_cast<size_t>(idx)] &&
                    seenStamp[static_cast<size_t>(idx)] != stamp) {
                    seenStamp[static_cast<size_t>(idx)] = stamp;
                    bandColors.push_back(idx);
                }
            }
        }
        std::sort(bandColors.begin(), bandColors.end());

        bool firstColorInBand = true;
        for (const int i : bandColors) {
            if (!firstColorInBand) out += '$'; // overstrike: back to column 0
            firstColorInBand = false;
            out += '#' + QByteArray::number(i);
            for (int x = 0; x < w; ++x) {
                unsigned char bits = 0;
                for (int r = 0; r < rowsInBand; ++r)
                    if (scanLines[r][x] == i) bits |= (1u << r);
                sixels[static_cast<size_t>(x)] = static_cast<char>(63 + bits);
            }
            // Run-length encode the band row for this colour.
            int x = 0;
            while (x < w) {
                int run = 1;
                while (x + run < w &&
                       sixels[static_cast<size_t>(x + run)] == sixels[static_cast<size_t>(x)])
                    ++run;
                if (run >= 4) {
                    out += '!' + QByteArray::number(run);
                    out += sixels[static_cast<size_t>(x)];
                } else {
                    for (int k = 0; k < run; ++k)
                        out += sixels[static_cast<size_t>(x)];
                }
                x += run;
            }
        }
        out += '-'; // next band
    }
    out += "\033\\"; // ST
    return out;
}

void writeKittyPlaceholders(TextGridBuffer& buffer, int x0, int y0, int cols, int rows,
                            quint32 imageId, const ClipRect& clip) {
    const uint32_t idColor = 0xFF000000u | (imageId & 0xFFFFFFu); // true-colour, opaque
    const int usableRows = (std::min)(rows, kDiacriticCount);
    const int usableCols = (std::min)(cols, kDiacriticCount);
    for (int r = 0; r < usableRows; ++r) {
        for (int c = 0; c < usableCols; ++c) {
            const int x = x0 + c;
            const int y = y0 + r;
            if (!clip.contains(x, y)) continue;
            Cell* cell = buffer.tryCellAt(x, y);
            if (!cell) continue;
            buffer.splitWideGlyphSpan(x, y, 1);
            char buf[16];
            int len = UnicodeText::encodeCodePointUtf8(kPlaceholder, buf);
            len += UnicodeText::encodeCodePointUtf8(kDiacritics[r], buf + len);
            len += UnicodeText::encodeCodePointUtf8(kDiacritics[c], buf + len);
            cell->setCharacter(QByteArrayView(buf, len));
            cell->displayWidth = 1;
            cell->continuation = false;
            cell->fgColor = idColor;
            cell->bgColor = 0x00000000u; // let the image show through
            cell->attributes = TextAttribute::None;
            cell->linkId = 0;
        }
    }
}

} // namespace qmltui::graphics
