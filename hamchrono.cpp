/*
    sdlgui/example1.cpp -- C++ version of an example application that shows 
    how to use the various widget classes. 

    Based on NanoGUI by Wenzel Jakob <wenzel@inf.ethz.ch>.
    Adaptation for SDL by Dalerank <dalerankn8@gmail.com>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
#include "guipi/hamchrono.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <sdlgui/entypo.h>
#include <sdlgui/ImageRepository.h>
#include <sdlgui/ImageDisplay.h>
#include <sdlgui/button.h>
#include <sdlgui/layout.h>
#include <sdlgui/toolbutton.h>
#include <sdlgui/widget.h>
#include <sdlgui/tabwidget.h>
#include <sdlgui/TimeBox.h>
#include <guipi/Dialog.h>
#include <guipi/GeoChrono.h>
#include <guipi/GuiPiApplication.h>
#include <guipi/EphemerisModel.h>
#include <guipi/SatelliteDataDisplay.h>
#include <guipi/Settings.h>

using namespace sdlgui;

namespace guipi {
    using namespace std;

    /**
     * @brief build the icon repository
     */
    void HamChrono::buildIconRepository() {
        ImageRepository::ImageStoreIndex idx{0, 0};
        for (auto &conf : mGeoLocationIcons) {
            ImageData imageData{createIcon(conf.icon, conf.size,
                                           Color{get<0>(conf.color), get<1>(conf.color), get<2>(conf.color),
                                                 get<3>(conf.color)})};
            mIconRepository->push_back(idx.first, move(imageData));
        }
        ++idx.first;

        for (auto &conf : mCelestialIcons) {
            ImageData imageData{createIcon(conf.icon, conf.size,
                                           Color{get<0>(conf.color), get<1>(conf.color), get<2>(conf.color),
                                                 get<3>(conf.color)})};
            mIconRepository->push_back(idx.first, move(imageData));
        }
        ++idx.first;

        for (auto &conf : mSatOrbitIcons) {
            ImageData imageData{createIcon(conf.icon, conf.size,
                                           Color{get<0>(conf.color), get<1>(conf.color), get<2>(conf.color),
                                                 get<3>(conf.color)})};
            mIconRepository->push_back(idx.first, move(imageData));
        }
    }

    /**
     * @brief Capture a screen shot.
     */
    void HamChrono::screenShot() {
        SDL_Surface *sshot = SDL_CreateRGBSurface(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 32, 0x00ff0000, 0x0000ff00,
                                                  0x000000ff, 0xff000000);
        SDL_RenderReadPixels(mSDL_Renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
        SDL_SaveBMP(sshot, "screenshot.bmp");
        SDL_FreeSurface(sshot);
    }

    /**
     * @brief Fetch an image and render it to a surface which is then rendered to a texture.
     * Intended to be called in a std::future to fetch the image in the background.
     * @param url the URL to fetch
     * @param homedir the users home directory, used to find the image cache directory.
     * @param name the name, displayed to the user, and used to generate the cache file name.
     * @return
     */
    tuple<SDL_Surface *, time_point<std::chrono::system_clock>>
    HamChrono::curlFetchImage(const string &url, const string &homedir, const string &name) {
        try {
            // Set the URL.
            curlpp::options::Url myUrl(url);
            curlpp::Easy myRequest;

            std::filesystem::path fileName{homedir};
            fileName.append(user_directory).append(image_path).append(name).replace_extension(".jpg");
            ofstream response;
            response.open(fileName, fstream::out | fstream::trunc);
            if (response) {
                myRequest.setOpt(new curlpp::options::Url(url));
                myRequest.setOpt(new curlpp::options::WriteStream(&response));

                // Send request and get a result.
                myRequest.perform();
                response.close();
                auto surface = IMG_Load(fileName.c_str());
                if (surface == nullptr) {
                    std::cerr << "Unable to load image from '" << fileName << "'\n";
                } else {
                    return make_tuple(surface, chrono::system_clock::now());
                }
            } else {
                std::cerr << "Unable to open '" << fileName << "' for writing.\n";
                perror("System Error.");
            }
        }

        catch (curlpp::RuntimeError &e) {
            std::cout << e.what() << std::endl;
        }

        catch (curlpp::LogicError &e) {
            std::cout << e.what() << std::endl;
        }

        return make_tuple(nullptr, chrono::system_clock::now());
    }

    /**
     * @brief Update the images at regular intervals
     * @param interval the interval in milliseconds
     * @return the value used for the next interval
     */
    Uint32 HamChrono::timerCallback(Uint32 interval) {
        for (ImageRepository::ImageStoreIndex idx{0, 0}; idx.second < mImageRepository->size(idx.first); ++idx.second) {
            mImageRepository->mFutureStore[idx] = async(curlFetchImage,
                                                        mImageRepository->image(idx).path,
                                                        mSettings->mHomeDir, mImageRepository->image(idx).name);
        }
        return interval;
    }

    /**
     * @brief The main application class, and top level widget. The constructor builds the widget tree.
     * @param pwindow the SDL_Window which becomes the Screen
     * @param rwidth screen width
     * @param rheight screen height
     * @param homedir the user's home directory
     * @param callsign the user's callsign, if provided on the command line
     * @param observer the user's location if provided on the command line
     */
    HamChrono::HamChrono(SDL_Window *pwindow, int rwidth, int rheight, const string &homedir, const string &callsign,
                         const Observer &observer)
            : GuiPiApplication(pwindow, rwidth, rheight, "HamChrono " XSTR(VERSION)),
              mTimer{*this, &HamChrono::timerCallback, 3600000} {
        mSettings = new Settings{homedir + "/.hamchrono/settings.sqlite"};
        mSettings->mHomeDir = homedir;
        mSettings->initializeSettingsDatabase();

        mEphemerisModel.setSettings(mSettings);

        if (!callsign.empty()) {
            mSettings->mCallSign = callsign;
            if (observer.LO > -200.) {
                mSettings->mLongitude = (float) observer.LO;
                mSettings->mLatitude = (float) observer.LA;
                mSettings->mElevation = (float) observer.HT;
            }
            mSettings->writeAllValues();
        }

        initialize();
    }

    void HamChrono::initialize() {
        setSettings(mSettings);

        /*
         * Process the callback initiated when the value of settings is changed at some point in
         * the system.
         */
        mSettings->addCallback([this](guipi::Settings::Parameter parameter) {
            switch (parameter) {
                case Settings::Parameter::EphemerisSource:
                    mEphemerisModel.loadEphemerisLibrary(mSettings->mEphemerisSource);
                    mEphemerisModel.setSatellitesOfInterest(mSettings->mSatellitesOfInterest);
                    mEphemerisModel.timerCallback(0);
                    break;
                case Settings::Parameter::SatellitesOfInterest:
                    mEphemerisModel.setSatellitesOfInterest(mSettings->mSatellitesOfInterest);
                    break;
                case Settings::Parameter::CallSign:
                    if (Widget *widget = find("qthButton", true); widget != nullptr) {
                        if (auto button = dynamic_cast<Button *>(widget); button != nullptr) {
                            button->setCaption(mSettings->mCallSign);
                        }
                    }
                    break;
                default:
                    break;
            }
        });

        // Set image scaling quality to the highest value available
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

        // Create the image repository for icons and build it
        mIconRepository = new ImageRepository{};
        buildIconRepository();

        // Create the image repository for images and fill it with the initialization data
        mImageRepository = new ImageRepository();
        for (auto &image : NasaSolarImages) {
            ImageData imageData{};
            imageData.path = image.first;
            imageData.name = image.second;

            // If there is already an image in cache, load it to display while waiting on new images.
            std::filesystem::path imagePath{mSettings->mHomeDir};
            imagePath.append(user_directory).append(image_path).append(imageData.name).replace_extension(".jpg");

            std::error_code ec;
            if (std::filesystem::exists(imagePath, ec)) {

                auto surface = IMG_Load(imagePath.c_str());
                imageData.set(SDL_CreateTextureFromSurface(mSDL_Renderer, surface));
                SDL_FreeSurface(surface);
                // Determine the write time of the image cache, convert to system time.
                auto writeTime = std::filesystem::last_write_time(imagePath, ec);
                auto sysWriteTime = time_point_cast<system_clock::duration>(writeTime - decltype(writeTime)::clock::now() +
                        system_clock::now());
                imageData.loaded = fileClockToSystemClock(writeTime);
            }
            mImageRepository->push_back(0, move(imageData));
        }

        // Create the std::future / std::async to fetch the images.
        for (ImageRepository::ImageStoreIndex idx{0, 0}; idx.second < mImageRepository->size(idx.first); ++idx.second) {
            using namespace std::chrono;
            // if the age of the caching file is more than 30 minutes fetch the image asynchronously.
            if (timePointDiff<minutes>(system_clock::now(), mImageRepository->image(idx).loaded) > 30)
                mImageRepository->mFutureStore[idx] = async(curlFetchImage,
                                                        mImageRepository->image(idx).path,
                                                        mSettings->mHomeDir, mImageRepository->image(idx).name);
        }

        // TODO: Deprecate since widgets can get this information from the Settings object.
        qthLatLon.x = deg2rad(mSettings->mLongitude);
        qthLatLon.y = deg2rad(mSettings->mLatitude);
        aQthLatLon = antipode(qthLatLon);
        mObserver = Observer{mSettings->mLatitude, mSettings->mLongitude, mSettings->mElevation};

        // Set up sizes of the various screen areas
        Vector2i mapAreaSize(EARTH_BIG_W, EARTH_BIG_H);
        Vector2i topAreaSize(mScreenSize.x, mScreenSize.y - mapAreaSize.y);
        Vector2i botAreaSize(mScreenSize.x, mScreenSize.y - topAreaSize.y);
        Vector2i sideBarSize(mScreenSize.x - mapAreaSize.x, mapAreaSize.y);

        /**
         * @brief The remainder of the constructor/initialization creates the widget tree and sets up internal
         * linkages.
         */
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

        auto qthButton = timeSet->add<Button>(mSettings->mCallSign)
                ->withCallback([this]() {
                    add<SettingsDialog>(find("qthButton", true), "Settings",
                                        Vector2i{40, 40});
                })
                ->withIconFontSize(50)
                ->withFixedWidth(210)
                ->withFontSize(40)
                ->withId("qthButton");

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

        // Create an image repeater to use with the image display.
        topArea->add<ImageDisplay>()->withImageRepository(mImageRepository)
                ->setCallback([=](ImageDisplay &w, ImageRepository::EventType e) {
                    if (e == ImageRepository::CLICK_EVENT) {
                        auto imageRepeater = add<ImageRepeater>(Vector2i(210, 0), Vector2i(440, 440));
                        performLayout();
                        w.withRepeater(imageRepeater);
                        w.repeatImage();
                    } else
                        w.setImageIndex(w.imageRepository()->actionEvent(e, w.getImageIndex()));
                })
                ->withFixedWidth(topAreaSize.y)
                ->withFixedHeight(topAreaSize.y)
                ->withId("smallimages");

        topArea->add<Widget>()->withFixedSize(Vector2i(topAreaSize.y, topAreaSize.y));

        topArea->add<Widget>()->withFixedSize(Vector2i(topAreaSize.y, topAreaSize.y));

        auto mapArea = botArea->add<Widget>()->withFixedSize(mapAreaSize)->withId("mapArea")
                ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 0, 0);

        ImageRepository::ImageStoreIndex satIdx{2, 0};
        ImageRepository::ImageStoreIndex orbBgnd{0, 2};
        ImageRepository::ImageStoreIndex trackIdx{0, 1};

        std::filesystem::path night_map_path{static_directory};
        night_map_path.append(map_path).append(night_map.first).replace_extension(night_map.second);
        std::filesystem::path day_map_path{static_directory};
        day_map_path.append(map_path).append(day_map.first).replace_extension(day_map.second);
        std::filesystem::path backdrop_path{static_directory};
        backdrop_path.append(background_path).append(backdrop);

        mGeoChrono = mapArea->add<GeoChrono>()
                ->withImageRepository(mIconRepository, satIdx, orbBgnd, trackIdx)
                ->withBackgroundFile(night_map_path.string())
                ->withForegroundFile(day_map_path.string())
                ->withBackdropFile(backdrop_path.string())
                ->withFixedSize(Vector2i(EARTH_BIG_W, EARTH_BIG_H));

        mGeoChrono->setSatelliteDisplay(mSettings->mSatelliteTracking);
        mGeoChrono->setSunMoonDisplay(mSettings->mCelestialTracking);
        mGeoChrono->setAzmuthalDisplay(mSettings->mAzimuthalDisplay);

        auto switches = topArea->add<Widget>()
                ->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0);
        switches->add<Widget>()->withPosition(Vector2i(620, 0))
                ->withFixedSize(Vector2i(40, topAreaSize.y))
                ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 12)
                ->add<ToolButton>(ENTYPO_ICON_ROCKET, Button::Flags::ToggleButton)
                ->withPushed(mGeoChrono->satelliteDisplay())
                ->withChangeCallback([&](bool state) {
                    mGeoChrono->setSatelliteDisplay(state);
                    mSettings->setSatelliteTracking(state ? 1 : 0);
                    if (state)
                        if (auto location = this->find("Location", true)) {
                            dynamic_cast<ToolButton *>(location)->withPushed(false);
                            mSettings->setGeoPositions(0);
                        }
                })
                ->withId("Rocket")->_and()
                ->add<ToolButton>(ENTYPO_ICON_MOON, Button::Flags::ToggleButton)
                ->withPushed(mGeoChrono->sunMoonDisplay())
                ->withChangeCallback([&](bool state) {
                    mGeoChrono->setSunMoonDisplay(state);
                    mSettings->setCelestialTracking(state ? 1 : 0);
                })
                ->_and()
                ->add<ToolButton>(ENTYPO_ICON_GLOBE, Button::Flags::ToggleButton)
                ->withPushed(mGeoChrono->azmuthalDisplay())
                ->withChangeCallback([&](bool state) {
                    mGeoChrono->setAzmuthalDisplay(state);
                    mSettings->setAzimuthalDisplay(state ? 1 : 0);
                });

        switches->add<Widget>()->withPosition(Vector2i(620, 0))
                ->withFixedSize(Vector2i(40, topAreaSize.y))
                ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 12)
                ->add<ToolButton>(ENTYPO_ICON_LOCATION, Button::Flags::ToggleButton)
                ->withPushed(mSettings->mGeoPositions)
                ->withChangeCallback([&](bool state) {
                    mSettings->setGeoPositions(state ? 1 : 0);
                    if (state)
                        if (auto rocket = this->find("Rocket", true)) {
                            dynamic_cast<ToolButton *>(rocket)->withPushed(false);
                            mGeoChrono->setSatelliteDisplay(false);
                            mSettings->setSatelliteTracking(0);
                        }
                })->withId("Location")->_and()
                ->add<ToolButton>(ENTYPO_ICON_NETWORK, Button::Flags::ToggleButton)->_and()
                ->add<ToolButton>(ENTYPO_ICON_COMPASS, Button::Flags::ToggleButton);

        switches->add<Widget>()->withPosition(Vector2i(620, 0))
                ->withFixedSize(Vector2i(40, topAreaSize.y))
                ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 12)
                ->add<ToolButton>(ENTYPO_ICON_THREE_DOTS, Button::Flags::NormalButton)
                ->withCallback([this]() {
                    add<ControlsDialog>(find("switches_controls", true), "Controls",
                                        Vector2i{40, 40});
//                        this->performLayout();
                })
                ->withId("switches_controls")->_and()
                ->add<ToolButton>(ENTYPO_ICON_LIGHT_DOWN, Button::Flags::ToggleButton)->_and()
                ->add<ToolButton>(ENTYPO_ICON_CAMERA, Button::Flags::NormalButton)
                ->withCallback([&]() {
                    screenShot();
                });

        auto tab = sideBar->add<TabWidget>();
        tab->withFixedSize(Vector2i {sideBarSize.x-10, 260});

        Widget *layer = tab->createTab("S", ENTYPO_ICON_ROCKET);
        layer->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 0));

        // Use overloaded variadic add to fill the tab widget with Different tabs.
        mSatelliteDataDisplay = layer->add<SatelliteDataDisplay>(mIconRepository,
                         ImageRepository::ImageStoreIndex{2, 0})
                ->withFixedWidth(sideBarSize.x - 20);

        layer = tab->createTab("D", ENTYPO_ICON_LOCATION);
        layer->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 0));

        // Use overloaded variadic add to fill the tab widget with Different tabs.
        layer->add<Label>("Stations", "sans-bold")->withFixedWidth(sideBarSize.x - 20);

        tab->setActiveTab(mSettings->mSideBarActiveTab);
        tab->setCallback([this](int activeTab) {
            mSettings->setSideBarActiveTab(activeTab);
        });

        mEphemerisModel.setPassMonitorCallback([this](auto data) {
            mSatelliteDataDisplay->updateSatelliteData(data);
        });

        mEphemerisModel.setOrbitTrackingCallback([this](auto data) {
            mGeoChrono->setOrbitalData(data);
        });

        mEphemerisModel.setPassTrackingCallback([this](auto data) {
            mGeoChrono->setPasStrackingData(data);
        });

        mEphemerisModel.setCelestialTrackingCallback([this](auto data) {
            mGeoChrono->setCelestialTrackingData(data);
        });

        mEphemerisModel.loadEphemerisLibraryWait(mSettings->mEphemerisSource);
        mEphemerisModel.setSatellitesOfInterest(
                mSettings->mSatellitesOfInterest); //"ISS,AO-92,FO-99,IO-26,DIWATA-2,FOX-1B,AO-7,AO-27,AO-73,SO-50");
        mEphemerisModel.timerCallback(0);
    }
}


using namespace guipi;

class InputParser {
public:
    InputParser(int &argc, char **argv) {
        for (int i = 1; i < argc; ++i)
            this->tokens.emplace_back(argv[i]);
    }

    /// @author iain
    [[nodiscard]] const std::string &getCmdOption(const std::string &option) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->tokens.begin(), this->tokens.end(), option);
        if (itr != this->tokens.end() && ++itr != this->tokens.end()) {
            return *itr;
        }
        static const std::string empty_string;
        return empty_string;
    }

    /// @author iain
    [[nodiscard]] bool cmdOptionExists(const std::string &option) const {
        return std::find(this->tokens.begin(), this->tokens.end(), option)
               != this->tokens.end();
    }

private:
    std::vector<std::string> tokens;
};


int main(int argc, char **argv) {
    InputParser inputParser{argc, argv};
    Observer observer{-100, -200, 0};
    string callsign{};

    if (inputParser.cmdOptionExists("-cs")) {
        callsign = inputParser.getCmdOption("-cs");
        if (inputParser.cmdOptionExists("-lat"))
            observer.LA = std::strtod(inputParser.getCmdOption("-lat").c_str(), nullptr);
        if (inputParser.cmdOptionExists("-lon"))
            observer.LO = std::strtod(inputParser.getCmdOption("-lon").c_str(), nullptr);
        if (inputParser.cmdOptionExists("-el"))
            observer.HT = std::strtod(inputParser.getCmdOption("-el").c_str(), nullptr);
    }

    string homdir{getenv("HOME")};
    std::filesystem::path imageDir{homdir};
    imageDir.append(HamChrono::user_directory).append(HamChrono::image_path);
    std::filesystem::create_directories(imageDir);

    std::filesystem::path ephemerisDir{homdir};
    ephemerisDir.append(HamChrono::user_directory).append(HamChrono::ephem_path);
    std::filesystem::create_directories(ephemerisDir);

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
    if (window == nullptr) {
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

    sdlgui::ref<HamChrono> app{new HamChrono(window, winWidth, winHeight, homdir, callsign, observer)};

    app->performLayout(app->sdlRenderer());

    app->eventLoop();

    SDL_DestroyRenderer(renderer);
    return 0;
}
