//
// Created by richard on 2020-09-12.
//

#include <iostream>
#include <utility>
#include <SDL2/SDL.h>
#include <sdlgui/toolbutton.h>
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
                                mCallback(*this, ImageRepository::DOWN_EVENT);
                            else
                                mCallback(*this, ImageRepository::UP_EVENT);
                        } else {
                            if (d.x > 0)
                                mCallback(*this, ImageRepository::RIGHT_EVENT);
                            else
                                mCallback(*this, ImageRepository::LEFT_EVENT);
                        }
                        mMotionStart = mMotionEnd = Vector2i(0, 0);
                        mMotion = mButton = false;
                    } else {
                        mCallback(*this, ImageRepository::CLICK_EVENT);
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

        if (mImageRepository) {
//            mImageIndex = mImageIndex % (long) mImages.size();
            Vector2i p = Vector2i(mMargin, mMargin);
            p += Vector2i(ax, ay);
            auto imageSize = mImageRepository->imageSize(mImageStoreIndex);
            int imgw = imageSize.x;
            int imgh = imageSize.y;

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

            mImageRepository->renderCopy(renderer, mImageStoreIndex, imgSrcRect, imgPaintRect);
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

        return Vector2i(mImageRepository->imageSize(mImageStoreIndex));
    }

    ImageRepeater::ImageRepeater(Widget *parent,
                                 const Vector2i &position, const Vector2i &fixedSize)
                                 : Window(parent, "Repeater", position) {
        withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0);
        if (buttonPanel())
            buttonPanel()->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0)
                    ->withFixedHeight(35)
                    ->add<ToolButton>(ENTYPO_ICON_SQUARED_CROSS, Button::Flags::NormalButton)
                            ->withCallback([&]() {
                                window()->setVisible(false);
                            });
        mImageDisplay = add<ImageDisplay>()->withFixedSize(fixedSize);
        setVisible(false);
    }

    void ImageRepeater::setTexture(SDL_Texture *texture, const string &caption) {
        mImageDisplay->setTexture(texture);
        window()->setTitle(caption);
    }

    void ImageRepeater::repeateFromRepository(const ref<ImageDisplay>& imageDisplay, ref<ImageRepository> imageRepository,
                                              ImageRepository::ImageStoreIndex imageStoreIndex) {
        auto name = imageRepository->imageName(imageStoreIndex);
        auto idx = name.find_last_of('.');
        mImageDisplay->mImageRepository = std::move(imageRepository);
        mImageDisplay->mImageStoreIndex = imageStoreIndex;
        window()->setTitle(name.substr(0, idx));
        window()->setVisible(true);
        dynamic_cast<Screen*>(window()->parent())->moveWindowToFront(window());
        mImageDisplay->mTextureDirty = true;
    }

    ref<ImageDisplay>
    ImageRepeater::setCallback(const function<void(ImageDisplay &, ImageRepository::EventType)> &callback) {
            mImageDisplay->setCallback(callback);
            return mImageDisplay;
    }
}
