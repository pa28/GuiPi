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
#include <guipi/GuiPiApplication.h>
#include <guipi/Ephemeris.h>
#include <sdlgui/entypo.h>
#include <guipi/GeoChrono.h>
#include <sdlgui/ImageRepository.h>
#include <sdlgui/ImageDisplay.h>
#include <sdlgui/button.h>
#include <sdlgui/layout.h>
#include <sdlgui/toolbutton.h>
#include <sdlgui/widget.h>
#include <sdlgui/TimeBox.h>

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

        Ephemeris mEphemeris;
        Timer<HamChrono> mTimer;

        ref<ImageRepository> mIconRepository;

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

        struct PlotPackageConfig {
            string_view name;
            PlotItemType itemType;
            int icon;
            int size;
            array<uint8_t ,4>color;
        };

        static constexpr array<PlotPackageConfig, 6> mPlotPackageConfig = {
                PlotPackageConfig{"QTH", PlotItemType::GEO_LOCATION_QTH, ENTYPO_ICON_HAIR_CROSS,
                                  50, {0, 255, 0, 255}},
                PlotPackageConfig{"A_QTH", PlotItemType::GEO_LOCATION_ANTIPODE, ENTYPO_ICON_HAIR_CROSS,
                                  50, {255, 0, 0, 255}},
                PlotPackageConfig{"Moon", PlotItemType::CELESTIAL_BODY_MOON, ENTYPO_ICON_MOON,
                                  50, {255, 255, 255, 255}},
                PlotPackageConfig{"Sun", PlotItemType::CELESTIAL_BODY_SUN, ENTYPO_ICON_LIGHT_UP,
                                  50, {255, 255, 0, 255}},
                PlotPackageConfig{"NOAA_15", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_DOT,
                                  50, {0, 255, 0, 255}},
                PlotPackageConfig{"ISS", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_DOT,
                                  50, {255, 0, 0, 255}}
        };

        static Vector2f antipode(const Vector2f &location) {
            return Vector2f {(location.x < 0 ? 1.f : -1.f) * ((float)M_PI - abs(location.x)), -location.y};
        }

        vector<PlotPackage> buildPlotPackage() {
            vector<PlotPackage> plotPackage;

            ImageRepository::ImageStoreIndex idx{0,0};
            for( auto & conf : mPlotPackageConfig) {
                if (conf.icon) {
                    ImageData imageData{createIcon(conf.icon, conf.size,
                               Color{get<0>(conf.color), get<1>(conf.color), get<2>(conf.color), get<3>(conf.color)})};
                    mIconRepository->push_back(idx.first,move(imageData));
                    PlotPackage plot;
                    switch (conf.itemType) {
                        case GEO_LOCATION_QTH:
                            plot = PlotPackage{conf.name, conf.itemType, qthLatLon};
                            break;
                        case GEO_LOCATION_ANTIPODE:
                            plot = PlotPackage{conf.name, conf.itemType, antipode(qthLatLon)};
                            break;
                        case CELESTIAL_BODY_SUN:
                        case CELESTIAL_BODY_MOON:
                        case EARTH_SATELLITE:
                            plot = PlotPackage{conf.name, conf.itemType};
                            break;
                        default:
                            throw (logic_error("Can't configure PlotPackage/Icon as defined."));
                    }
                    plot.mImageRepository = mIconRepository;
                    plot.mImageIndex = idx;
                    plotPackage.push_back(plot);
                    ++idx.second;
                } else {
                    throw (logic_error("Can't configure PlotPackage/Icon as defined."));
                }
            }

            return move(plotPackage);
        }

        HamChrono(SDL_Window *pwindow, int rwidth, int rheight)
                : GuiPiApplication(pwindow, rwidth, rheight, "HamChrono")
                , mTimer(*this, &HamChrono::timerCallback, 5000)
                {
            mIconRepository = new ImageRepository{};

            qthLatLon = Vector2f(deg2rad(-77.), deg2rad(45.));

            Vector2i mapAreaSize(660, 330);
            Vector2i topAreaSize(mScreenSize.x, mScreenSize.y - mapAreaSize.y);
            Vector2i botAreaSize(mScreenSize.x, mScreenSize.y - topAreaSize.y);
            Vector2i sideBarSize(mScreenSize.x - mapAreaSize.x, mapAreaSize.y);

            mEphemeris.loadEphemeris();

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
                        if (e == ImageRepository::CLICK_EVENT)
                            w.repeatImage();
                        else
                            w.setImageIndex(w.imageRepository()->actionEvent(e, w.getImageIndex()));
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

//            if (auto moon = mEphemeris.predict("Moon"))
//                mGeoChrono->setSubLunar(moon.value());
//            if (auto iss = mEphemeris.predict("ISS"))
//                mGeoChrono->setRocketCoord(iss.value());

            auto plotPackage = buildPlotPackage();
            mGeoChrono->setPlotPackage(plotPackage);

            auto switches = topArea->add<Widget>()
                    ->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0);
            switches->add<Widget>()->withPosition(Vector2i(620, 0))
                    ->withFixedSize(Vector2i(40, topAreaSize.y))
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 12)
                    ->add<ToolButton>(ENTYPO_ICON_COMPASS, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_MOON, Button::Flags::ToggleButton)
                            ->withPushed(mGeoChrono->sunMoonDisplay())
                            ->withChangeCallback([&](bool state) { mGeoChrono->setSunMoonDisplay(state); })
                            ->_and()
                    ->add<ToolButton>(ENTYPO_ICON_GLOBE, Button::Flags::ToggleButton)
                            ->withPushed(mGeoChrono->azmuthalDisplay())
                            ->withChangeCallback([&](bool state) { mGeoChrono->setAzmuthalDisplay(state); });

            switches->add<Widget>()->withPosition(Vector2i(620, 0))
                    ->withFixedSize(Vector2i(40, topAreaSize.y))
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 12)
                    ->add<ToolButton>(ENTYPO_ICON_NETWORK, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_LOCATION, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_ROCKET, Button::Flags::ToggleButton);

            switches->add<Widget>()->withPosition(Vector2i(620, 0))
                    ->withFixedSize(Vector2i(40, topAreaSize.y))
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 12)
                    ->add<ToolButton>(ENTYPO_ICON_THREE_DOTS, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_LIGHT_DOWN, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_HAIR_CROSS, Button::Flags::ToggleButton);
        }

        void drawContents() override {
        }

    public:
        /**
         * The timer callback
         * @param interval
         * @return the new interval
         */
        Uint32 timerCallback(Uint32 interval) {
            for (auto & plotItem : mGeoChrono->getPlotPackage()) {
                if (plotItem.mPlotItemType == CELESTIAL_BODY_MOON || plotItem.mPlotItemType == EARTH_SATELLITE)
                    plotItem.predict(mEphemeris);
            }
//            if (auto moon = mEphemeris.predict("Moon"))
//                mGeoChrono->setSubLunar(moon.value());
//            if (auto iss = mEphemeris.predict("ISS"))
//                mGeoChrono->setRocketCoord(iss.value());
            return interval;
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

    int winWidth = 800;
    int winHeight = 480;

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

    sdlgui::ref<HamChrono> app{new HamChrono(window, winWidth, winHeight)};

    app->performLayout(app->sdlRenderer());

    app->eventLoop();

    SDL_DestroyRenderer(renderer);
    return 0;
}
