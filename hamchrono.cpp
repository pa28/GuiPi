/*
    sdlgui/example1.cpp -- C++ version of an example application that shows 
    how to use the various widget classes. 

    Based on NanoGUI by Wenzel Jakob <wenzel@inf.ethz.ch>.
    Adaptation for SDL by Dalerank <dalerankn8@gmail.com>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <algorithm>
#include <mutex>
#include <string_view>
#include <thread>
#include <SDL2/SDL.h>
#include <sdlgui/entypo.h>
#include <guipi/GeoChrono.h>
#include <sdlgui/ImageRepository.h>
#include <sdlgui/ImageDisplay.h>
#include <sdlgui/button.h>
#include <sdlgui/layout.h>
#include <sdlgui/toolbutton.h>
#include <sdlgui/widget.h>
#include <sdlgui/tabwidget.h>
#include <sdlgui/TimeBox.h>
#include <guipi/GuiPiApplication.h>
#include <guipi/Ephemeris.h>
#include <guipi/SatelliteDataDisplay.h>

using namespace sdlgui;

namespace guipi {
    class HamChrono : public GuiPiApplication {
    protected:

        Vector2f qthLatLon;
        Vector2i mScreenSize{800, 480};
        Observer mObserver{};

        bool mHasBrightnessControl{true};
        bool mRunEventLoop{true};

        sdlgui::ref<GeoChrono> mGeoChrono;
        sdlgui::ref<ToolButton> mAzmuthalButton;
        sdlgui::ref<Window> mMainWindow;
        sdlgui::ref<Window> mPopupWindow;
        sdlgui::ref<ImageRepository> mImageRepository;

        Ephemeris mEphemeris;
        thread mEphemerisThread;
        mutex mEphemerisMutex;
        Timer<HamChrono> mTimer;

        ref<ImageRepository> mIconRepository;
        ref<SatelliteDataDisplay> mSatelliteDataDisplay;

    public:
        ~HamChrono() override = default;

        template<typename T>
        constexpr T deg2rad(T deg) { return deg * M_PI / 180.; }

#if __cplusplus == 201703L
        static constexpr string_view map_path = "maps/";
        static constexpr string_view image_path = "images/";
        static constexpr string_view background_path = "backgrounds/";
        static constexpr string_view day_map = "day_earth_660x330.png";
        static constexpr string_view night_map = "night_earth_660x330.png";
        static constexpr string_view backdrop = "NASA_Nebula.png";
#else
        const char *map_path = "maps/";
        const char *image_path = "images/";
        const char *background_path = "backgrounds/";
        const char *day_map = "day_earth_660x330.png";
        const char *night_map = "night_earth_660x330.png";
        const char *backdrop = "NASA_Nebula.png";
#endif

#if __cplusplus == 201703L
        struct PlotPackageConfig {
            string_view name;
            PlotItemType itemType;
            int icon;
            int size;
            array<uint8_t ,4>color;
        };

        static constexpr array<PlotPackageConfig, 9> mPlotPackageConfig = {
                PlotPackageConfig{"QTH", PlotItemType::GEO_LOCATION_QTH, ENTYPO_ICON_HAIR_CROSS,
                                  50, {0, 255, 0, 255}},
                PlotPackageConfig{"A_QTH", PlotItemType::GEO_LOCATION_ANTIPODE, ENTYPO_ICON_HAIR_CROSS,
                                  50, {255, 0, 0, 255}},
                PlotPackageConfig{"Moon", PlotItemType::CELESTIAL_BODY_MOON, ENTYPO_ICON_MOON,
                                  50, {255, 255, 255, 255}},
                PlotPackageConfig{"Sun", PlotItemType::CELESTIAL_BODY_SUN, ENTYPO_ICON_LIGHT_UP,
                                  50, {255, 255, 0, 255}},
                PlotPackageConfig{"", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {0, 255, 0, 255}},
                PlotPackageConfig{"", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {255, 128, 128, 255}},
                PlotPackageConfig{"", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {255, 0, 255, 255}},
                PlotPackageConfig{"", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {128, 64, 255, 255}},
                PlotPackageConfig{"", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {255, 0, 0, 255}}
        };
#else
        struct PlotPackageConfig {
            const char *name;
            PlotItemType itemType;
            int icon;
            int size;
            array<uint8_t, 4> color;
        };

        vector<guipi::HamChrono::PlotPackageConfig> mPlotPackageConfig{
                PlotPackageConfig{"QTH", PlotItemType::GEO_LOCATION_QTH, ENTYPO_ICON_HAIR_CROSS,
                                  50, {0, 255, 0, 255}},
                PlotPackageConfig{"A_QTH", PlotItemType::GEO_LOCATION_ANTIPODE, ENTYPO_ICON_HAIR_CROSS,
                                  50, {255, 0, 0, 255}},
                PlotPackageConfig{"Moon", PlotItemType::CELESTIAL_BODY_MOON, ENTYPO_ICON_MOON,
                                  50, {255, 255, 255, 255}},
                PlotPackageConfig{"Sun", PlotItemType::CELESTIAL_BODY_SUN, ENTYPO_ICON_LIGHT_UP,
                                  50, {255, 255, 0, 255}},
                PlotPackageConfig{"NOAA_15", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {0, 255, 0, 255}},
                PlotPackageConfig{"NOAA_18", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {255, 128, 128, 255}},
                PlotPackageConfig{"NOAA_19", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {255, 0, 255, 255}},
                PlotPackageConfig{"NOAA_20", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {128, 64, 255, 255}},
                PlotPackageConfig{"ISS", PlotItemType::EARTH_SATELLITE, ENTYPO_ICON_RECORD,
                                  30, {255, 0, 0, 255}}
        };
#endif

        static Vector2f antipode(const Vector2f &location) {
            return Vector2f{(location.x < 0 ? 1.f : -1.f) * ((float) M_PI - abs(location.x)), -location.y};
        }

        vector<PlotPackage> buildPlotPackage() {
            vector<PlotPackage> plotPackage;

            ImageRepository::ImageStoreIndex idx{0, 0};
            for (auto &conf : mPlotPackageConfig) {
                if (conf.icon) {
                    ImageData imageData{createIcon(conf.icon, conf.size,
                                                   Color{get<0>(conf.color), get<1>(conf.color), get<2>(conf.color),
                                                         get<3>(conf.color)})};
                    mIconRepository->push_back(idx.first, move(imageData));
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

            mGeoChrono->withImageRepository(mIconRepository);
            return move(plotPackage);
        }

        HamChrono(SDL_Window *pwindow, int rwidth, int rheight)
                : GuiPiApplication(pwindow, rwidth, rheight, "HamChrono"),
                  mTimer(*this, &HamChrono::timerCallback, 60000) {
            mIconRepository = new ImageRepository{};

            qthLatLon = Vector2f(deg2rad(-76.0123), deg2rad(44.9016));
            mObserver = Observer{44.9016, -76.0123, 0.};

            Vector2i mapAreaSize(660, 330);
            Vector2i topAreaSize(mScreenSize.x, mScreenSize.y - mapAreaSize.y);
            Vector2i botAreaSize(mScreenSize.x, mScreenSize.y - topAreaSize.y);
            Vector2i sideBarSize(mScreenSize.x - mapAreaSize.x, mapAreaSize.y);

            timerCallback(0);

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
                    ->withObserver(mObserver)
                    ->withStationCoordinates(qthLatLon)
                    ->withBackgroundFile(string(map_path) + string(night_map))
                    ->withForegroundFile(string(map_path) + string(day_map))
                    ->withBackdropFile(string(background_path) + string(backdrop))
                    ->withFixedSize(Vector2i(EARTH_BIG_W, EARTH_BIG_H));

            auto plotPackage = buildPlotPackage();
            mGeoChrono->setPlotPackage(plotPackage);
            timerCallback(0); // TODO: should clean this up, hoist the prediction/sorting functin.

            auto switches = topArea->add<Widget>()
                    ->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0);
            switches->add<Widget>()->withPosition(Vector2i(620, 0))
                    ->withFixedSize(Vector2i(40, topAreaSize.y))
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 12)
                    ->add<ToolButton>(ENTYPO_ICON_ROCKET, Button::Flags::ToggleButton)
                    ->withPushed(mGeoChrono->satelliteDisplay())
                    ->withChangeCallback([&](bool state) {
                        mGeoChrono->setSatelliteDisplay(state);
                        if (state)
                            if (auto rocket = this->find("Location", true)) {
                                dynamic_cast<ToolButton *>(rocket)->withPushed(false);
                            }
                    })
                    ->withId("Rocket")->_and()
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
                    ->add<ToolButton>(ENTYPO_ICON_LOCATION, Button::Flags::ToggleButton)
                    ->withChangeCallback([&](bool state) {
                        if (state)
                            if (auto rocket = this->find("Rocket", true)) {
                                dynamic_cast<ToolButton *>(rocket)->withPushed(false);
                                mGeoChrono->setSatelliteDisplay(false);
                            }
                    })->withId("Location")->_and()
                    ->add<ToolButton>(ENTYPO_ICON_NETWORK, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_COMPASS, Button::Flags::ToggleButton);

            switches->add<Widget>()->withPosition(Vector2i(620, 0))
                    ->withFixedSize(Vector2i(40, topAreaSize.y))
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 12)
                    ->add<ToolButton>(ENTYPO_ICON_THREE_DOTS, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_LIGHT_DOWN, Button::Flags::ToggleButton)->_and()
                    ->add<ToolButton>(ENTYPO_ICON_HAIR_CROSS, Button::Flags::ToggleButton);

//            auto esv = sideBar->add<Widget>()->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 0, 0);
//
//            for (auto &plot : mGeoChrono->getPlotPackage()) {
//                if (plot.mPlotItemType == EARTH_SATELLITE) {
//                    esv->add<Widget>()->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0)
//                            ->add<Label>(plot.mName)->withFont(mTheme->mBoldFont)->withFontSize(15);
//                }
//            }

            auto tab = sideBar->add<TabWidget>();

            Widget *layer = tab->createTab("S", ENTYPO_ICON_ROCKET);
            layer->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 0));

            // Use overloaded variadic add to fill the tab widget with Different tabs.
            mSatelliteDataDisplay = layer->add<SatelliteDataDisplay>(mGeoChrono)->withFixedWidth(sideBarSize.x - 20);

            layer = tab->createTab("D", ENTYPO_ICON_LOCATION);
            layer->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 0));

            // Use overloaded variadic add to fill the tab widget with Different tabs.
            layer->add<Label>("Stations", "sans-bold")->withFixedWidth(sideBarSize.x - 20);

            tab->setActiveTab(0);
        }

        void drawContents() override {
        }

    public:
        /**
         * The timer callback
         * @param interval
         * @return the new interval
         */

#if __cplusplus == 201703L
        static constexpr array<string_view, 9> initialSatelliteList{
                "ISS",
                "AO-92", // AO-91
                "SO-50",
                "IO-26",
                "DIWATA-2",
                "FOX-1B",
                "AO-7",
                "AO-27",
                "AO-73"
        };
#else
        vector<char const *> initialSatelliteList{
                "ISS",
                "AO-92", // AO-91
                "SO-50",
                "IO-26",
                "DIWATA-2",
                "FOX-1B",
                "AO-7",
                "AO-27",
                "AO-73"
        };
#endif

        Uint32 timerCallback(Uint32 interval) {
            // Reap the ephemeris loading thread.
            if (mEphemerisThread.joinable()) {
                mEphemerisThread.join();
            }

            // If there is no ephemris loaded, try to get it.
            if (mEphemeris.empty()) {
                mEphemerisThread = thread([this]() {
                    lock_guard<mutex> lockGuard(mEphemerisMutex);
                    mEphemeris.loadEphemeris();
                });

                return interval;
            }

            lock_guard<mutex> lockGuard(mEphemerisMutex);

            // Predict location and next pass for each currently predicted satellite.
            DateTime now;
            bool changed = false;
            now.userNow();
            for (auto &plotItem : mGeoChrono->getPlotPackage()) {
                if (plotItem.mName.empty()) {
                    changed = true;
                } else if (plotItem.mPlotItemType == EARTH_SATELLITE && !plotItem.mName.empty()) {
                    plotItem.predict(mEphemeris);
                    plotItem.predictPass(mEphemeris, mObserver);
                    plotItem.mEarthsat.roundPassTimes();

                    // If the predicted rise time is less than two minutes away
                    if (auto timeToRise = (plotItem.mEarthsat.riseTime() - now) * 86400.; timeToRise < 120) {
                        auto satellite = mEphemeris.satellite(plotItem.mName);
                        if (satellite)
                            mGeoChrono->passTracker()->addSatellite(satellite.value());
                    }
                }
            }

            // Sort in order of increasing rise time, and look for changes in order.
            sort(mGeoChrono->getPlotPackage().begin(), mGeoChrono->getPlotPackage().end(),
                 [&changed](PlotPackage &p0, PlotPackage &p1) {
                     auto r = p0.compareLt(p1);
                     changed |= r;
                     return r;
                 });

            // If the order has changed it means that the list should be refreshed.
            if (changed) {
                // Get ephemeris for the list of satellites the user wants tracked,
                // and predict the next pass.
                vector<pair<string, Earthsat>> passList;
                for (auto &sat : initialSatelliteList) {
                    Earthsat earthsat{};
                    earthsat = mEphemeris.nextPass(string(sat), mObserver);
                    passList.emplace_back(pair<string, Earthsat>{string(sat), earthsat});
                }

                // Sort the list by rise time.
                sort(passList.begin(), passList.end(), [](auto p0, auto p1) {
                    return p0.second.riseTime() < p1.second.riseTime();
                });

//                // Debugging display
//                for (auto &pass : passList) {
//                    std::cout << pass.first << ": " << pass.second.riseTime() << '\n';
//                }

                // Move the new list into the GeoChrono and predict the current location.
                auto pass = passList.begin();
                for (auto &plotItem : mGeoChrono->getPlotPackage()) {
                    if (pass == passList.end())
                        break;
                    if (plotItem.mPlotItemType == EARTH_SATELLITE) {
                        plotItem.mName = pass->first;
                        plotItem.mEarthsat = pass->second;
                        plotItem.predict(mEphemeris);
                        ++pass;
                    }
                }

                // Debugging output.
//                std::cout << "Changed\n";
//
//                for (auto &plotItem : mGeoChrono->getPlotPackage()) {
//                    std::cout << plotItem.mName << '\n';
//                    if (plotItem.mEarthsat.passFound()) {
//                        std::cout << "Rise: " << plotItem.mEarthsat.riseTime() << " @ "
//                                  << plotItem.mEarthsat.riseAzimuth() << '\n'
//                                  << "Set:  " << plotItem.mEarthsat.setTime() << " @ "
//                                  << plotItem.mEarthsat.setAzimuth() << "\n\n";
//                    } else
//                        std::cout << "No pass\n\n";
//                }
            }

            // Update the satellite display
            if (changed && mSatelliteDataDisplay)
                mSatelliteDataDisplay->updateSatelliteData();
            return interval;
        }
    };
}

using namespace guipi;

int main(int /* argc */, char ** /* argv */) {
#if __cplusplus == 201703L
    std::cerr << "C++17\n";
#elif __cplusplus == 201402L
    std::cerr << "C++14\n";
#else
    std::cerr << "C++ unkonwn\n";
#endif
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
