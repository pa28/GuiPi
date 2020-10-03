//
// Created by richard on 2020-10-01.
//


/*
    Another significant redesign to update the coding standards to C++17,
    reduce the amount of bare pointer handling (especially in user code),
    and focus on the RaspberryPi environment.
    
    License terms for the changes as well as the base nanogui-sdl code are
    contained int the LICENSE.txt file.
    
    A significant redesign of this code was contributed by Christian Schueller.

    Based on NanoGUI by Wenzel Jakob <wenzel@inf.ethz.ch>.
    Adaptation for SDL by Dalerank <dalerankn8@gmail.com>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#pragma once

#include <map>
#include <vector>
#include <type_traits>
#include <sdlgui/common.h>
#include <sdlgui/widget.h>
#include <sdlgui/Image.h>

namespace sdlgui {
    using namespace std;
    class ImageRepository : public Object {
    public:
        typedef vector<ImageDataList> ImageStore;
        typedef pair<ImageDataList::size_type, ImageStore::size_type> ImageStoreIndex;
        typedef std::function<void(ImageRepository &, ImageStoreIndex)> ImageChangedCallback;
        typedef map<ref<Widget>, ImageChangedCallback> ImageChangedCallbackList;

        map<ImageStoreIndex, ImageChangedCallbackList> mImageCallbackMap;

        ImageStore mImageStore;

        enum EventType {
            UP_EVENT, LEFT_EVENT, DOWN_EVENT, RIGHT_EVENT, CLICK_EVENT
        };

    public:

        ImageRepository() = default;
        ~ImageRepository() override = default;

        template<typename T>
        T incMod(T value, make_signed_t<T> increment, T modulus) {
            if (increment >= 0)
                return (value + static_cast<make_unsigned_t<T>>(increment)) % modulus;
            else {
                return (value + modulus - static_cast<make_unsigned_t<T>>(-increment)) % modulus;
            }
        }

        auto actionEvent(EventType event, ImageStoreIndex index) {
            switch (event) {
                case LEFT_EVENT:
                    index.first = incMod(index.first, -1, mImageStore.size());
                    index.second %= mImageStore.at(index.first).size();
                    break;
                case RIGHT_EVENT:
                    index.first = incMod(index.first, 1, mImageStore.size());
                    index.second %= mImageStore.at(index.first).size();
                    break;
                case UP_EVENT:
                    index.second = incMod(index.second, -1, mImageStore.at(index.first).size());
                    break;
                case DOWN_EVENT:
                    index.second = incMod(index.second, 1, mImageStore.at(index.first).size());
                    break;
                default:
                    break;
            }
            return index;
        }

        Vector2i imageSize(ImageStoreIndex imageStoreIndex) const {
            return Vector2i(mImageStore.at(imageStoreIndex.first).at(imageStoreIndex.second).w,
                            mImageStore.at(imageStoreIndex.first).at(imageStoreIndex.second).h);
        }

        const string & imageName(ImageStoreIndex index) const {
            return mImageStore.at(index.first).at(index.second).name;
        }

        void renderCopy(SDL_Renderer *renderer, ImageStoreIndex index, SDL_Rect &imgSrcRect, SDL_Rect &imgPaintRect) {
            SDL_RenderCopy(renderer, mImageStore.at(index.first).at(index.second).get(), &imgSrcRect, &imgPaintRect);
        }

        void push_back(ImageStore::size_type index, ImageData imageData) {
            mImageStore[index].push_back(move(imageData));
        }

        void addImageList(ImageDataList imageDataList) {
            mImageStore.push_back(move(imageDataList));
        }

        bool empty() const { return mImageStore.empty(); }

        bool empty(ImageStore::size_type idx) const { return mImageStore.at(idx).empty(); }

        auto size() const { return mImageStore.size(); }

        auto size(ImageStore::size_type idx) const { return mImageStore.at(idx).size(); }

        void addImangeChangeCallback(ImageStoreIndex index, const ref<Widget>& widget, const ImageChangedCallback& callback) {
            mImageCallbackMap[index][widget] = callback;
        }

        void removeImangeChangeCallback(ImageStoreIndex index, const ref<Widget>& widget) {
            mImageCallbackMap[index].erase(widget);
        }

        void setImage(ImageStoreIndex index, ImageData imageData) {
            mImageStore.at(index.first).at(index.second) = move(imageData);
            for (const auto& callback : mImageCallbackMap.at(index)) {
                (callback.second)(*this, index);
            }
        }


    };
}



