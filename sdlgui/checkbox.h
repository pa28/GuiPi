/*
    sdl_gui/checkbox.h -- Two-state check box widget

    Based on NanoGUI by Wenzel Jakob <wenzel@inf.ethz.ch>.
    Adaptation for SDL by Dalerank <dalerankn8@gmail.com>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#pragma once

#include <sdlgui/widget.h>
#include <vector>
#include <memory>

NAMESPACE_BEGIN(sdlgui)

class  CheckBox : public Widget
{
public:
    explicit CheckBox(Widget *parent, std::string caption = "Untitled",
             std::function<void(CheckBox*,bool)> callback = std::function<void(CheckBox*,bool)>());

    const std::string &caption() const { return mCaption; }
    void setCaption(const std::string &caption) { mCaption = caption; }

    const bool &checked() const { return mChecked; }
    void setChecked(const bool &checked) { mChecked = checked; }

    ref<CheckBox> withChecked(bool value) { setChecked(value); return ref<CheckBox>{this}; }

    const bool &pushed() const { return mPushed; }
    void setPushed(const bool &pushed) { mPushed = pushed; }

    std::function<void(CheckBox*,bool)> callback() const { return mCallback; }
    void setCallback(const std::function<void(CheckBox*,bool)> &callback) { mCallback = callback; }

    virtual bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers);
    Vector2i preferredSize(SDL_Renderer *ctx) const override;
    void draw(SDL_Renderer *ctx) override;
    virtual void drawBody(SDL_Renderer* renderer);
protected:
    std::string mCaption;
    bool mPushed, mChecked;
    std::string mFont;

    Texture _captionTex;
    Texture _pointTex;

    std::function<void(CheckBox*,bool)> mCallback;

    struct AsyncTexture;
    typedef std::shared_ptr<AsyncTexture> AsyncTexturePtr;
    std::vector<AsyncTexturePtr> _txs;
};

NAMESPACE_END(sdlgui)
