//
// Created by richard on 2020-10-15.
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

#include <sdlgui/window.h>
#include <string>
#include <utility>
#include <guipi/Settings.h>
#include <sstream>
#include <iomanip>

namespace guipi {
    using namespace sdlgui;

    class Dialog : public Window {
    public:
        ~Dialog() override = default;

        Dialog() = delete;

        Dialog(Widget *parent, const std::string &title, const Vector2i &position);

    protected:
        Window *mParentWindow;
    };

    class SettingsDialog : public Dialog {
    public:
        ~SettingsDialog() override = default;

        SettingsDialog() = delete;

        SettingsDialog(Widget *parent, const std::string &title, const Vector2i &position,
                       const Vector2i &fixedSize);

        void initialize();

    protected:
        template<typename T>
        std::string realToString(T value) {
            static_assert(std::is_floating_point<T>::value, "value must be floating point.");
            std::stringstream strm;
            strm << std::fixed << std::setw( 9 ) << std::setprecision( 4 ) << value;
            auto str = strm.str();
            auto spc = str.find_first_not_of(' ');
            if (spc != std::string::npos && spc > 0)
                return str.substr(spc);
            return str;
        }
    };
}



