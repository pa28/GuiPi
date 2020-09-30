//
// Created by richard on 2020-09-12.
//

#include <iostream>
#include <SDL2/SDL.h>
#include <sdlgui/button.h>
#include <sdlgui/entypo.h>
#include "ImageDisplay.h"

namespace sdlgui {
    bool ImageDisplay::mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button,
                                        int modifiers) {
        if (button) {
            if (!mMotion) {
                mMotionStart = p;
                mMotionEnd = p;
                mButton = mMotion = true;
            } else {
                mMotionEnd = p;
            }
        }
        return Widget::mouseMotionEvent(p, rel, button, modifiers);
    }

    bool ImageDisplay::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
        if (button) {
            if (down) {
                if (!mButton) {
                    mMotion = false;
                }
                mButton = true;
            } else {
                if (mCallback) {
                    auto d = mMotionEnd - mMotionStart;
                    if (mMotion && d.x * d.x + d.y * d.y > 25) {
                        if (abs(d.y) >= abs(d.x)) {
                            if (d.y > 0)
                                mCallback(*this, DOWN_EVENT);
                            else
                                mCallback(*this, UP_EVENT);
                        } else {
                            if (d.x > 0)
                                mCallback(*this, RIGHT_EVENT);
                            else
                                mCallback(*this, LEFT_EVENT);
                        }
                        mMotionStart = mMotionEnd = Vector2i(0, 0);
                        mMotion = mButton = false;
                    } else {
                        mCallback(*this, CLICK_EVENT);
                    }
                }
            }
        }
        return Widget::mouseButtonEvent(p, button, down, modifiers);
    }

    void ImageDisplay::draw(SDL_Renderer *renderer) {
        int ax = getAbsoluteLeft();
        int ay = getAbsoluteTop();;

        PntRect clip = getAbsoluteCliprect();
        SDL_Rect clipRect = pntrect2srect(clip);

        if (!mImages.empty()) {
            mImageIndex = mImageIndex % (long) mImages.size();
            Vector2i p = Vector2i(mMargin, mMargin);
            p += Vector2i(ax, ay);
            int imgw = mImages[mImageIndex].w;
            int imgh = mImages[mImageIndex].h;

            float iw, ih, ix, iy;
            int thumbSize;
            if (imgw < imgh) {
                thumbSize = fixedWidth() != 0 ? fixedWidth() : width();
                iw = (float) thumbSize;
                ih = iw * (float) imgh / (float) imgw;
                ix = 0;
                iy = 0;
            } else {
                thumbSize = fixedHeight() != 0 ? fixedHeight() : height();
                ih = (float) thumbSize;
                iw = ih * (float) imgw / (float) imgh;
                ix = 0;
                iy = 0;
            }

            SDL_Rect imgPaintRect{p.x + (int) ix, p.y + (int) iy, (int) iw, (int) ih};
            SDL_Rect imgSrcRect{0, 0, imgw, imgh};
            PntRect imgrect = clip_rects(srect2pntrect(imgPaintRect), clip);
            imgPaintRect.w = imgrect.x2 - imgrect.x1;
            imgPaintRect.h = imgrect.y2 - imgrect.y1;
            if (imgPaintRect.y < clip.y1) {
                imgPaintRect.y = clip.y1;
                imgSrcRect.h = (int) (((float) imgPaintRect.h / (float) ih) * (float) imgh);
                imgSrcRect.y = (int) ((1 - ((float) imgPaintRect.h / (float) ih)) * (float) imgh);
            } else if (imgPaintRect.h < (int) ih) {
                imgSrcRect.h = (int) (((float) imgPaintRect.h / ih) * (float) imgh);
            }

            SDL_RenderCopy(renderer, mImages[mImageIndex].get(), &imgSrcRect, &imgPaintRect);
        } else if (mRepeatedTexture) {
            Vector2i p = Vector2i(mMargin, mMargin);
            p += Vector2i(ax, ay);
            int imgw;
            int imgh;
            SDL_QueryTexture(mRepeatedTexture, nullptr, nullptr, &imgw, &imgh);

            float iw, ih, ix, iy;
            int thumbSize;
            if (imgw < imgh) {
                thumbSize = fixedWidth() != 0 ? fixedWidth() : width();
                iw = (float) thumbSize;
                ih = iw * (float) imgh / (float) imgw;
                ix = 0;
                iy = 0;
            } else {
                thumbSize = fixedHeight() != 0 ? fixedHeight() : height();
                ih = (float) thumbSize;
                iw = ih * (float) imgw / (float) imgh;
                ix = 0;
                iy = 0;
            }

            SDL_Rect imgPaintRect{p.x + (int) ix, p.y + (int) iy, (int) iw, (int) ih};
            SDL_Rect imgSrcRect{0, 0, imgw, imgh};
            PntRect imgrect = clip_rects(srect2pntrect(imgPaintRect), clip);
            imgPaintRect.w = imgrect.x2 - imgrect.x1;
            imgPaintRect.h = imgrect.y2 - imgrect.y1;
            if (imgPaintRect.y < clip.y1) {
                imgPaintRect.y = clip.y1;
                imgSrcRect.h = (int) (((float) imgPaintRect.h / (float) ih) * (float) imgh);
                imgSrcRect.y = (int) ((1 - ((float) imgPaintRect.h / (float) ih)) * (float) imgh);
            } else if (imgPaintRect.h < (int) ih) {
                imgSrcRect.h = (int) (((float) imgPaintRect.h / ih) * (float) imgh);
            }

            SDL_RenderCopy(renderer, mRepeatedTexture, &imgSrcRect, &imgPaintRect);
        }

        Widget::draw(renderer);
    }

    Vector2i ImageDisplay::preferredSize(SDL_Renderer *ctx) const {
        if (mImages.empty()) {
            int w;
            int h;
            SDL_QueryTexture(mRepeatedTexture, nullptr, nullptr, &w, &h);
            return Vector2i(w, h);
        }
        return Vector2i(mImages[mImageIndex].w, mImages[mImageIndex].h);
    }

    ImageRepeater::ImageRepeater(Widget *parent,
                                 const Vector2i &position, const Vector2i &fixedSize)
                                 : Window(parent, "", position) {
        withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0);
        buttonPanel()->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0)
                ->withFixedHeight(35)
                ->add<Button>("", ENTYPO_ICON_SQUARED_CROSS)
                        ->withCallback([=]() {
                            window()->setVisible(false);
                        })
                        ->withFixedSize(Vector2i(30,30));
        mImageDisplay = add<ImageDisplay>()->withFixedSize(fixedSize);
        setVisible(false);
    }

    void ImageRepeater::setTexture(SDL_Texture *texture, const string &caption) {
        mImageDisplay->setTexture(texture);
        window()->setTitle(caption);
    }
}
