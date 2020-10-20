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

    enum ResponseType {
        Information,
        Question,
        Warning,
    };

    enum ResponseButton {
        StandardButton,
        AlternateButton,
    };

    class Dialog : public Window {
    protected:
        sdlgui::ref<Widget> mTrigger;

        void disposeDialog();

        void dialogPerformLayout();

    public:
        ~Dialog() override = default;

        Dialog() = delete;

        Dialog(Widget *parent, Widget *trigger, const std::string &title, const Vector2i &position,
               bool buttonBar = true);
    };

    class SettingsDialog : public Dialog {
    public:
        ~SettingsDialog() override = default;

        SettingsDialog() = delete;

        SettingsDialog(Widget *parent, Widget *trigger, const std::string &title, const Vector2i &position);

        void initialize();

    protected:
        template<typename T>
        std::string realToString(T value, int precision) {
            static_assert(std::is_floating_point<T>::value, "value must be floating point.");
            std::stringstream strm;
            strm << std::fixed << std::setprecision( precision ) << value;
            return strm.str();
        }

        sdlgui::ref<ToolButton> mStopButton;
        sdlgui::ref<ToolButton> mRebootButton;
        sdlgui::ref<ToolButton> mUpgradeButton;
        sdlgui::ref<ToolButton> mHaltButton;

        enum SystemCmd {
            STOP,
            REBOOT,
            UPGRADE,
            HALT,
        };

        void systemButtonCallback(sdlgui::ref<ToolButton> &button, SystemCmd cmd);
    };

    class ControlsDialog : public Dialog {
    public:
        ~ControlsDialog() override = default;

        ControlsDialog() = delete;

        ControlsDialog(Widget *parent, Widget *trigger, const std::string &title, const Vector2i &position);

        void initialize();

    protected:
        sdlgui::ref<Button> mButton;

        void ephemerisSelectButton(sdlgui::ref<Widget> &parent, std::string_view label, int value);
    };

    class SatelliteSelector : public Dialog {
    protected:
        std::vector<std::string> mSelectedList{};

    public:

        SatelliteSelector() = delete;

        SatelliteSelector(Widget *parent, Widget *trigger, const std::string title);

        void setSelectedSatellite(const std::string& name, bool select);
    };

    class ResponseDialog : public Dialog {
    public:

        ResponseDialog() = delete;

        ResponseDialog(Widget *parent, Widget *trigger, ResponseType responseType,
                       const std::string &title, const std::string &message, const std::string &buttonText,
                       const std::string &altButtonText);

        sdlgui::ref<ResponseDialog> withCallback(std::function<void(ResponseButton)> callback) {
            mCallback = std::move(callback);
            return sdlgui::ref<ResponseDialog>{this};
        }

    protected:
        std::function<void(ResponseButton)> mCallback{};

    };
}



