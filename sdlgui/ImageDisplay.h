//
// Created by richard on 2020-09-12.
//

#pragma onece

#include <SDL.h>
#include <sdlgui/common.h>
#include <sdlgui/widget.h>
#include <sdlgui/Image.h>
#include <sdlgui/window.h>
#include <functional>
#include <utility>
#include "screen.h"

namespace sdlgui {

    class ImageDisplay;

    /**
     * A class to repeat display an image, usually in a larger size.
     */
    class ImageRepeater : public Window {
        friend class ImageDisplay;

    private:

        sdlgui::ref<ImageDisplay> mImageDisplay;        //< Local image display, the repeater
        sdlgui::ref<ImageDisplay> mParentImageDisplay;  //< Parent image display, source of the image

        /**
         * Set the texture to be repeated
         * @param texture
         */
        void setTexture(SDL_Texture *texture, const string &caption);

    public:
        ImageRepeater() = delete;

        ~ImageRepeater() override = default;

        ImageRepeater(Widget *parent, const Vector2i &position, const Vector2i &fixedSize);
    };

    /**
     * @class ImageDisplay
     * A minimalist Image display widget capable of resizing and displaying one image on an ImageList
     */
    class ImageDisplay : public Widget {
        friend class ImageRepeater;

    public:
        enum EventType {
            UP_EVENT, LEFT_EVENT, DOWN_EVENT, RIGHT_EVENT, CLICK_EVENT
        };

    private:
        ImageDataList mImages;      //< This list of images
        long mImageIndex{0};        //< The index to the image displayed
        int mMargin{0};             //< The margin around the image
        bool mTextureDirty{true};   //< True when the image needs to be re-drawn

        bool mButton{false};        //< True when button 1 has been pressed
        bool mMotion{false};        //< True when the mouse has been in motion with button 1 pressed
        Vector2i mMotionStart{};    //< The starting point of the motion;
        Vector2i mMotionEnd{};      //< The ending point of the motion;

        SDL_Texture *mRepeatedTexture{nullptr};
        ref<ImageRepeater> mImageRepeater;

        std::function<void(ImageDisplay &, EventType)> mCallback;

        void setTexture(SDL_Texture *texture) {
            mRepeatedTexture = texture;
            mTextureDirty = true;
            dynamic_cast<Screen*>(window()->parent())->moveWindowToFront(window());
            window()->setVisible(true);
        }

    public:
        ImageDisplay() = delete;

        /**
         * (Constructor)
         * Construct an empty ImageDisplay
         * @param parent the parent widget
         */
        explicit ImageDisplay(Widget *parent) : Widget(parent) {}

        /**
         * (Constructor)
         * Construct an ImageDisplay with an ListImages
         * @param parent the parent widget
         * @param data the ListImages
         */
        explicit ImageDisplay(Widget *parent, ImageDataList data)
                : Widget(parent) { setImages(data); }

        /**
         * Set the ListImages
         * @param listImages
         */
        void setImages(ImageDataList &listImages) {
            mImages.clear();
            std::move(listImages.begin(), listImages.end(), std::back_inserter(mImages));
            setImageIndex(0);
        }

        /**
         * Get the ImageList
         * @return a const reference to the ListImages.
         */
        [[nodiscard]] const ImageDataList &images() const { return mImages; }

        /**
         * Get the current image index
         * @return the image index
         */
        [[nodiscard]] auto getImageIndex() const { return mImageIndex; }

        /**
         * Set the image index. The result set is modulo the size of the ListImages so -1 is allowed.
         * @param index the index to set
         */
        void setImageIndex(long index) {
            if (mImages.empty())
                mImageIndex = 0;
            else
                mImageIndex = ((long) mImages.size() + index) % (long) mImages.size();
        }

        bool mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;

        bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) override;

        /**
         * Override the Widget draw method.
         * @param renderer the renderer to draw to.
         */
        void draw(SDL_Renderer *renderer) override;

        /// Compute the preferred size of the widget
        Vector2i preferredSize(SDL_Renderer *ctx) const override;

        /**
         * Building help, add a List of Images
         * @param listImages
         * @return a reference to this ImageDisplay
         */
        ref<ImageDisplay> withImages(ImageDataList &listImages) {
            mImages.clear();
            std::move(listImages.begin(), listImages.end(), std::back_inserter(mImages));
            setImageIndex(0);
            return this;
        }

        /**
         * Building help, set the index to the desired image.
         * @param idx the requested index
         * @return a reference to this ImageDisplay
         */
        ref<ImageDisplay> withImageIndex(const long idx) {
            mImageIndex = idx % (long)mImages.size();
            return this;
        }

        /**
         * Add an ImageRepater to this display
         * @param repeater the repeater
         * @return a reference to this ImageDisplay
         */
        ref<ImageDisplay> withRepeater(ref<ImageRepeater> repeater) {
            mImageRepeater = std::move(repeater);
            mImageRepeater->mParentImageDisplay = ref<ImageDisplay>{this};
            return this;
        }

        /**
         * Repeate the currently selected image on an ImageRepeater if one has been configured.
         */
        void repeatImage() {
            if (mImageRepeater && !mImages.empty()) {
                mImageIndex %= mImages.size();
                auto p = mImages[mImageIndex].name.find_last_of(".");
                auto caption = p ? mImages[mImageIndex].name.substr(0, p) : mImages[mImageIndex].name;

                mImageRepeater->setTexture(mImages[mImageIndex].get(), caption);
            }
        }

        /**
         * Get the currently set callback function.
         * @return the callback function.
         */
        [[nodiscard]] std::function<void(ImageDisplay &, EventType)> callback() const { return mCallback; }

        /**
         * Set the call back function
         * @param callback the desired callback function, a simple but usefule lambda is:
         * [](ImageDisplay &w, ImageDisplay::EventType e) {
                            switch(e) {
                                case ImageDisplay::RIGHT_EVENT:
                                case ImageDisplay::DOWN_EVENT:
                                    w.setImageIndex(w.getImageIndex()+1);
                                    break;
                                case ImageDisplay::LEFT_EVENT:
                                case ImageDisplay::UP_EVENT:
                                    w.setImageIndex(w.getImageIndex()-1);
                                    break;
                            }
                        }
         * @return a reference to this ImageDisplay
         */
        ref<ImageDisplay> setCallback(const std::function<void(ImageDisplay &, EventType)> &callback) {
            mCallback = callback;
            return this;
        }

        const string &getImageTitle() const {
            return mImages[mImageIndex].name;
        }

    };

}
