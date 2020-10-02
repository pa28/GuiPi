/*
    sdlgui/example1.cpp -- C++ version of an example application that shows 
    how to use the various widget classes. 

    Based on NanoGUI by Wenzel Jakob <wenzel@inf.ethz.ch>.
    Adaptation for SDL by Dalerank <dalerankn8@gmail.com>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <string_view>
#include <SDL2/SDL.h>
#include <GuiPi/GuiPiApplication.h>
#include <sdlgui/entypo.h>
#include <sdlgui/GeoChrono.h>
#include <sdlgui/ImageRepository.h>
#include <sdlgui/ImageDisplay.h>
#include <sdlgui/button.h>
#include <sdlgui/layout.h>
#include <sdlgui/toolbutton.h>
#include <sdlgui/widget.h>

using namespace sdlgui;

namespace guipi {
    class HamChrono : public GuiPiApplication {
    protected:

        Vector2f qthLatLon;
        Vector2i mScreenSize{800, 480};

        bool mHasBrightnessControl{true};
        bool mRunEventLoop{true};

        sdlgui::ref<GeoChrono> mGeoChrono;
        sdlgui::ref<ToolButton> mAzmuthalButton;
        sdlgui::ref<Window> mMainWindow;
        sdlgui::ref<Window> mPopupWindow;
        sdlgui::ref<ImageRepository> mImageRepository;

    public:
        ~HamChrono() override = default;

        template<typename T>
        constexpr T deg2rad(T deg) { return deg * M_PI / 180.; }

        static constexpr string_view map_path = "maps/";
        static constexpr string_view image_path = "images/";
        static constexpr string_view background_path = "backgrounds/";
        static constexpr string_view day_map = "day_earth_660x330.png";
        static constexpr string_view night_map = "night_earth_660x330.png";
        static constexpr string_view backdrop = "NASA_Nebula.png";


        HamChrono(SDL_Window *pwindow, int rwidth, int rheight)
                : GuiPiApplication(pwindow, rwidth, rheight, "HamChrono") {

            qthLatLon = Vector2f(deg2rad(-76.), deg2rad(45.));

            Vector2i mapAreaSize(660, 330);
            Vector2i topAreaSize(mScreenSize.x, mScreenSize.y - mapAreaSize.y);
            Vector2i botAreaSize(mScreenSize.x, mScreenSize.y - topAreaSize.y);
            Vector2i sideBarSize(mScreenSize.x - mapAreaSize.x, mapAreaSize.y);

            mMainWindow = add<Window>("", Vector2i::Zero())->withBlank(true)
                    ->withFixedSize(mScreenSize)
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 0, 0);

            auto topArea = mMainWindow->add<Widget>()
                    ->withFixedSize(topAreaSize)
                    ->withId("topArea")
                    ->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0);

            auto botArea = mMainWindow->add<Widget>()
                    ->withFixedSize(botAreaSize)
                    ->withId("botArea")
                    ->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0);

            auto timeSet = topArea->add<Widget>()
                    ->withId("timeSet")
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 5, 5);

            auto qthButton = timeSet->add<Button>("VE3YSH", ENTYPO_ICON_COG)
                    ->withIconFontSize(50)
                    ->withFontSize(40);

            timeSet->add<TimeBox>()
                    ->setCallback([&](TimeBox &tb, float value) {
                        setBrightness(value);
                    })
                    ->withId("GMT");

            auto controlBar = timeSet->add<Widget>()->withId("controls")
                    ->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 11);

            auto sideBar = botArea->add<Widget>()
                    ->withFixedSize(sideBarSize)
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 5, 5)
                    ->withId("sidebar");

            sideBar->add<TimeBox>(true, true)->withId("localtime");

            mImageRepository = new ImageRepository();
            mImageRepository->addImageList(loadImageDataDirectory(mSDL_Renderer, string(image_path)));

            // Create an image repeater to use with the image display.
            auto imageRepeater = add<ImageRepeater>(Vector2i(210, 0), Vector2i(450, 450));
            topArea->add<ImageDisplay>()->withImageRepository(mImageRepository)
                    ->withRepeater(imageRepeater)
                    ->setCallback([=](ImageDisplay &w, ImageRepository::EventType e) {
                        auto index = w.getImageIndex();
                        auto size0 = w.imageRepository()->size();
                        auto size1 = w.imageRepository()->size(index.first);
                        switch (e) {
                            case ImageRepository::RIGHT_EVENT:
                            case ImageRepository::DOWN_EVENT:
                            case ImageRepository::LEFT_EVENT:
                            case ImageRepository::UP_EVENT:
                                w.setImageIndex(w.imageRepository()->actionEvent(e, w.getImageIndex()));
                                break;
                            case ImageRepository::CLICK_EVENT:
                                w.repeatImage();
                                break;
                        }
                    })
                    ->withFixedWidth(topAreaSize.y)
                    ->withFixedHeight(topAreaSize.y)
                    ->withId("smallimages");

            topArea->add<Widget>()->withFixedSize(Vector2i(topAreaSize.y, topAreaSize.y));

            topArea->add<Widget>()->withFixedSize(Vector2i(topAreaSize.y, topAreaSize.y));

            auto mapArea = botArea->add<Widget>()->withFixedSize(mapAreaSize)->withId("mapArea")
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 0, 0);

            mGeoChrono = mapArea->add<GeoChrono>()
                    ->withStationCoordinates(qthLatLon)
                    ->withBackgroundFile(string(map_path) + string(night_map))
                    ->withForegroundFile(string(map_path) + string(day_map))
                    ->withBackdropFile(string(background_path) + string(backdrop))
                    ->withFixedSize(Vector2i(EARTH_BIG_W, EARTH_BIG_H));

            topArea->add<Widget>()->withPosition(Vector2i(620, 0))
                    ->withFixedSize(Vector2i(40, topAreaSize.y))
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 5, 2)
                    ->add<ToolButton>(ENTYPO_ICON_NETWORK, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_COMPASS, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_MOON, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_GLOBE, Button::Flags::ToggleButton)
                    ->withPushed(mGeoChrono->azmuthalDisplay())
                    ->withChangeCallback([=](bool state) { mGeoChrono->setAzmuthalDisplay(state); });
        }

        virtual void drawContents() {
        }

    private:
    };

}

using namespace guipi;

int main(int /* argc */, char ** /* argv */) {
    char rendername[256] = {0};
    SDL_RendererInfo info;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);   // Initialize SDL2

    SDL_Window *window;        // Declare a pointer to an SDL_Window

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    int winWidth = 1024;
    int winHeight = 768;

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
            "An SDL2 window",         //    const char* title
            SDL_WINDOWPOS_UNDEFINED,  //    int x: initial x position
            SDL_WINDOWPOS_UNDEFINED,  //    int y: initial y position
            winWidth,                      //    int w: width, in pixels
            winHeight,                      //    int h: height, in pixels
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
            SDL_WINDOW_ALLOW_HIGHDPI        //    Uint32 flags: window options, see docs
    );

    // Check that the window was successfully made
    if (window == NULL) {
        // In the event that the window could not be made...
        std::cout << "Could not create window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    auto context = SDL_GL_CreateContext(window);

    for (int it = 0; it < SDL_GetNumRenderDrivers(); it++) {
        SDL_GetRenderDriverInfo(it, &info);
        strcat(rendername, info.name);
        strcat(rendername, " ");
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    sdlgui::ref<HamChrono> screen = new HamChrono(window, winWidth, winHeight);

    screen->performLayout(screen->sdlRenderer());

    screen->eventLoop();
    SDL_DestroyRenderer(renderer);
    return 0;
}
