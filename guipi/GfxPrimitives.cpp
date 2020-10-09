//
// Created by richard on 2020-10-09.
//

#include "GfxPrimitives.h"

namespace guipi {
    /* ---- Pixel */

    /*!
    \brief Draw pixel  in currently set color.

    \param drawable The renderer to draw on.
    \param x X (horizontal) coordinate of the pixel.
    \param y Y (vertical) coordinate of the pixel.

    \returns Returns 0 on success, -1 on failure.
    */
    int pixel(Drawable &drawable, Sint16 x, Sint16 y) {
        if (holds_alternative<Surface>(drawable)) {
            // TODO: Code surface alternative.
            return 1;
        } else if (holds_alternative<Renderer>(drawable)) {
            return SDL_RenderDrawPoint(get<Renderer>(drawable).get(), x, y);
        } else
            throw std::logic_error("Alternative not allowed for in variant Drawable.");
    }

    /*!
    \brief Draw pixel with blending enabled if a<255.

    \param drawable The renderer to draw on.
    \param x X (horizontal) coordinate of the pixel.
    \param y Y (vertical) coordinate of the pixel.
    \param color The color value of the pixel to draw (0xRRGGBBAA).

    \returns Returns 0 on success, -1 on failure.
    */
    int pixelColor(Drawable &drawable, Sint16 x, Sint16 y, Uint32 color) {
        auto *c = (Uint8 *) &color;
        return pixelRGBA(drawable, x, y, c[0], c[1], c[2], c[3]);
    }

    /*!
    \brief Draw pixel with blending enabled if a<255.

    \param drawable The renderer to draw on.
    \param x X (horizontal) coordinate of the pixel.
    \param y Y (vertical) coordinate of the pixel.
    \param r The red color value of the pixel to draw.
    \param g The green color value of the pixel to draw.
    \param b The blue color value of the pixel to draw.
    \param a The alpha value of the pixel to draw.

    \returns Returns 0 on success, -1 on failure.
    */
    int pixelRGBA(Drawable &drawable, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        int result = 0;
        if (holds_alternative<Surface>(drawable)) {
            // TODO: Code surface alternative.
            result = 1;
        } else if (holds_alternative<Renderer>(drawable)) {
            result |= SDL_SetRenderDrawBlendMode(get<Renderer>(drawable).get(), (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
            result |= SDL_SetRenderDrawColor(get<Renderer>(drawable).get(), r, g, b, a);
            result |= SDL_RenderDrawPoint(get<Renderer>(drawable).get(), x, y);
        } else
            throw std::logic_error("Alternative not allowed for in variant Drawable.");

        return result;
    }
}