/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <eez/conf-internal.h>

#if EEZ_OPTION_GUI

#include <eez/gui/font.h>

namespace eez {
namespace gui {
namespace font {

Font::Font()
	: fontData(0)
{
}

Font::Font(const FontData *fontData_)
	: fontData(fontData_)
{
}

uint8_t Font::getAscent() {
    return fontData->ascent;
}

uint8_t Font::getDescent() {
    return fontData->descent;
}

uint8_t Font::getHeight() {
    return fontData->ascent + fontData->descent;
}

const GlyphData *Font::getGlyph(int32_t encoding) {
	auto start = fontData->encodingStart;
	auto end = fontData->encodingEnd;

    uint32_t glyphIndex = 0;
	if ((uint32_t)encoding < start || (uint32_t)encoding > end) {
        // TODO use binary search
        uint32_t i;
		for (i = 0; i < fontData->groups.count; i++) {
            auto group = fontData->groups[i];
            if ((uint32_t)encoding >= group->encoding && (uint32_t)encoding < group->encoding + group->length) {
                glyphIndex = group->glyphIndex + (encoding - group->encoding);
                break;
            }
        }
        if (i == fontData->groups.count) {
            return nullptr;
        }
	} else {
        glyphIndex = encoding - start;
    }

	auto glyphData = fontData->glyphs[glyphIndex];

	if (glyphData->dx == -128) {
		// empty glyph
		return nullptr;
	}

	return glyphData;
}

} // namespace font
} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
