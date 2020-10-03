//
// Created by richard on 2020-09-12.
//

#pragma onece

#include <SDL.h>
#include <sdlgui/common.h>
#include <sdlgui/widget.h>
#include <sdlgui/Image.h>
#include <sdlgui/ImageRepository.h>
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

        void repeateFromRepository(const ref<ImageDisplay>& imageDisplay, ref<ImageRepository> imageRepository, ImageRepository::ImageStoreIndex imageStoreIndex);
    };

    /**
     * @class ImageDisplay
     * A minimalist Image display widget capable of resizing and displaying one image on an ImageList
     */
    class ImageDisplay : public Widget {
        friend class ImageRepeater;

    private:
        ImageDataList mImages;      //< This list of images
        ref<ImageRepository> mImageRepository;
        ImageRepository::ImageStoreIndex mImageStoreIndex;
//        long mImageIndex{0};        //< The index to the image displayed
        int mMargin{0};             //< The margin around the image
        bool mTextureDirty{true};   //< True when the image needs to be re-drawn

        bool mButton{false};        //< True when button 1 has been pressed
        bool mMotion{false};        //< True when the mouse has been in motion with button 1 pressed
        Vector2i mMotionStart{};    //< The starting point of the motion;
        Vector2i mMotionEnd{};      //< The ending point of the motion;

        SDL_Texture *mRepeatedTexture{nullptr};
        ref<ImageRepeater> mImageRepeater;

        std::function<void(ImageDisplay &, ImageRepository::EventType)> mCallback;

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
         * Get the ImageList
         * @return a const reference to the ListImages.
         */
        [[nodiscard]] const ImageDataList &images() const { return mImages; }

        /**
         * Get the current image index
         * @return the image index
         */
        [[nodiscard]] auto getImageIndex() const { return mImageStoreIndex; }

        auto imageRepository() const { return mImageRepository; }

        /**
         * Set the image index. The result set is modulo the size of the ListImages so -1 is allowed.
         * @param index the index to set
         */
        void setImageIndex(ImageRepository::ImageStoreIndex index) {
            if (mImageStoreIndex != index) {
                mImageStoreIndex = index;
                mTextureDirty = true;
            }
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

//        /**
//         * Building help, set the index to the desired image.
//         * @param idx the requested index
//         * @return a reference to this ImageDisplay
//         */
//        ref<ImageDisplay> withImageIndex(const long idx) {
//            mImageIndex = idx % (long)mImages.size();
//            return this;
//        }

        /**
         * Add an ImageRepater to this display
         * @param repeater the repeater
         * @return a reference to this ImageDisplay
         */
        ref<ImageDisplay> withRepeater(ref<ImageRepeater> repeater) {
            mImageRepeater = std::move(repeater);
            mImageRepeater->mParentImageDisplay = ref<ImageDisplay>{this};
            return ref<ImageDisplay>{this};
        }

        ref<ImageDisplay> withImageRepository(ref<ImageRepository> repository) {
            mImageRepository = std::move(repository);
            return ref<ImageDisplay>{this};
        }

        /**
         * Repeate the currently selected image on an ImageRepeater if one has been configured.
         */
        void repeatImage() {
            if (mImageRepeater && mImageRepository) {
                mImageRepeater->repeateFromRepository(ref<ImageDisplay>{this}, mImageRepository, mImageStoreIndex);
            }
        }

        /**
         * Get the currently set callback function.
         * @return the callback function.
         */
        [[nodiscard]] std::function<void(ImageDisplay &, ImageRepository::EventType)> callback() const { return mCallback; }

        /**
         * Set the call back function
         * @param callback the desired callback function, a simple but usefule lambda is:
         * [](ImageDisplay &w, ImageRepository::::EventType e) {
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
        ref<ImageDisplay> setCallback(const std::function<void(ImageDisplay &, ImageRepository::EventType)> &callback) {
            mCallback = callback;
            return ref<ImageDisplay>{this};
        }

    };

}
