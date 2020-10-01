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

#include <vector>
#include <sdlgui/common.h>
#include <sdlgui/Image.h>

namespace sdlgui {
    using namespace std;
    class ImageRepository : public Object {
    private:
        typedef vector<ImageDataList> ImageStore;

        ImageStore mImageStore;

    public:

        ImageRepository() = default;
        ~ImageRepository() override = default;

        auto push_back(ImageStore::size_type index, ImageData imageData) {
            mImageStore[index].push_back(move(imageData));
        }

        auto setImage(ImageStore::size_type idx0, ImageDataList::size_type idx1, ImageData imageData) {
            mImageStore.at(idx0).at(idx1) = move(imageData);
            // TODO: call callbacks.
        }
    };
}



