//
// Created by richard on 2020-10-02.
//

#include "GuiPiApplication.h"

sdlgui::ImageData guipi::GuiPiApplication::createIcon(int iconCode, int iconSize, const sdlgui::Color &iconColor) {
    auto icon = utf8(iconCode);
    Texture texture;
    mTheme->getTexAndRectUtf8(mSDL_Renderer, texture, 0, 0, icon.data(), "icons", iconSize, iconColor);
    return ImageData{move(texture)};
}
