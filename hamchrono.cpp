/*
    sdlgui/example1.cpp -- C++ version of an example application that shows 
    how to use the various widget classes. 

    Based on NanoGUI by Wenzel Jakob <wenzel@inf.ethz.ch>.
    Adaptation for SDL by Dalerank <dalerankn8@gmail.com>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <algorithm>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
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
#include <guipi/EphemerisModel.h>
#include <guipi/SatelliteDataDisplay.h>
//#include <guipi/SatelliteDataDisplay.h>

using namespace sdlgui;

namespace guipi {
    class HamChrono : public GuiPiApplication {
    protected:

        Vector2f qthLatLon;
        Vector2f aQthLatLon;
        Vector2i mScreenSize{800, 480};
        Observer mObserver{};

        bool mHasBrightnessControl{true};
        bool mRunEventLoop{true};

        sdlgui::ref<GeoChrono> mGeoChrono;
        sdlgui::ref<ToolButton> mAzmuthalButton;
        sdlgui::ref<Window> mMainWindow;
        sdlgui::ref<Window> mPopupWindow;
        sdlgui::ref<ImageRepository> mImageRepository;

        ref<ImageRepository> mIconRepository;
        ref<SatelliteDataDisplay> mSatelliteDataDisplay;

        string mHomeDir;

        EphemerisModel mEphemerisModel{};

    public:
        ~HamChrono() override = default;

        Timer<HamChrono> mTimer;

        template<typename T>
        constexpr T deg2rad(T deg) { return deg * M_PI / 180.; }

#if __cplusplus == 201703L
        static constexpr string_view map_path = "/var/lib/hamchrono/maps/";
        static constexpr string_view image_path = "/.hamchrono/images/";
        static constexpr string_view background_path = "/var/lib/hamchrono/backgrounds/";
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

        static Vector2f antipode(const Vector2f &location) {
            return Vector2f{(location.x < 0 ? 1.f : -1.f) * ((float) M_PI - abs(location.x)), -location.y};
        }

        struct IconRepositoryData {
            int icon;
            int size;
            array<uint8_t, 4> color;
        };

        // Icons for geographic locations ImageRepository::ImageStoreIndex idx{0, x}
        static constexpr array<IconRepositoryData, 3> mGeoLocationIcons = {
                IconRepositoryData{ENTYPO_ICON_HAIR_CROSS, 50, {0x00, 0xFF, 0x00, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_HAIR_CROSS, 50, {0xFF, 0x00, 0x00, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD,40, {0x00, 0x00, 0x00, 0xFF}}
        };

        // Icons for celestial objects ImageRepository::ImageStoreIndex idx{1, x}
        static constexpr array<IconRepositoryData, 2> mCelestialIcons = {
                IconRepositoryData{ENTYPO_ICON_LIGHT_UP, 50, {255, 255, 0, 255}},
                IconRepositoryData{ENTYPO_ICON_MOON, 50, {255, 255, 255, 255}}
        };

        // Icons for satellites ImageRepository::ImageStoreIndex idx{2, x}
        static constexpr array<IconRepositoryData, 5> mSatOrbitIcons = {
                IconRepositoryData{ENTYPO_ICON_RECORD,30, {0x00, 0xFF, 0, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD,30, {0x80, 0xC0, 0, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD,30, {0xA0, 0xA0, 0, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD,30, {0xC0, 0x40, 0, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD,30, {0xFF, 0x00, 0, 0xFF}}
        };

        void buildIconRepository() {
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

        static constexpr array<pair<string_view, string_view>, 5> NasaSolarImages {
        pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0193.jpg", "AIA 193 Å" },
        pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_211193171.jpg", "AIA 211 Å, 193 Å, 171 Å" },
        pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_HMIB.jpg", "HMI Magnetogram" },
        pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_HMIIC.jpg", "HMI Intensitygram" },
        pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0171.jpg", "AIA 171 Å" }
        };

        void screenShot() {
            SDL_Surface *sshot = SDL_CreateRGBSurface(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
            SDL_RenderReadPixels(mSDL_Renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
            SDL_SaveBMP(sshot, "screenshot.bmp");
            SDL_FreeSurface(sshot);
        }

        static tuple<SDL_Surface *, time_point<std::chrono::system_clock>>
        curlFetchImage(SDL_Renderer *renderer, const string &url, const string &homedir, const string &name) {
            SDL_Texture *texture = nullptr;
            try {
                // Set the URL.
                curlpp::options::Url myUrl(url);
                curlpp::Easy myRequest;

                ofstream response;
                string fileName = homedir + string{image_path} + name + ".jpg";
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

            return make_tuple(nullptr,chrono::system_clock::now());
        }

        Uint32 timerCallback(Uint32 interval) {
            for (ImageRepository::ImageStoreIndex idx{0,0}; idx.second < mImageRepository->size(idx.first); ++idx.second) {
                mImageRepository->mFutureStore[idx] = async(curlFetchImage, mSDL_Renderer, mImageRepository->image(idx).path,
                                                            mHomeDir, mImageRepository->image(idx).name);
            }
            return interval;
        }

        HamChrono(SDL_Window *pwindow, int rwidth, int rheight, const string &homedir, const string &callsign, const Vector2f &geoCoord)
                : GuiPiApplication(pwindow, rwidth, rheight, "HamChrono"),
                mTimer{*this, &HamChrono::timerCallback, 3600000}{
            mHomeDir = homedir;
            SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "2");
            mIconRepository = new ImageRepository{};
            buildIconRepository();

            mImageRepository = new ImageRepository();
            for (auto &image : NasaSolarImages) {
                ImageData imageData{};
                imageData.path = image.first;
                imageData.name = image.second;
                mImageRepository->push_back(0, move(imageData));
            }

            for (ImageRepository::ImageStoreIndex idx{0,0}; idx.second < mImageRepository->size(idx.first); ++idx.second) {
                mImageRepository->mFutureStore[idx] = async(curlFetchImage, mSDL_Renderer,
                                                            mImageRepository->image(idx).path,
                                                            mHomeDir, mImageRepository->image(idx).name);
            }

            qthLatLon.x = deg2rad(geoCoord.x);
            qthLatLon.y = deg2rad(geoCoord.y);
            aQthLatLon = antipode(qthLatLon);
            mObserver = Observer{ geoCoord.y, geoCoord.x, 0.};

            Vector2i mapAreaSize(EARTH_BIG_W, EARTH_BIG_H);
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
                    ->withFixedWidth(220)
                    ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 5, 5);

            auto qthButton = timeSet->add<Button>(callsign, ENTYPO_ICON_COG)
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

            // Create an image repeater to use with the image display.
            auto imageRepeater = add<ImageRepeater>(Vector2i(210, 0), Vector2i(450, 450));
            imageRepeater->setCallback([=](ImageDisplay &w, ImageRepository::EventType e) {
                        screenShot();
                    });
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

            ImageRepository::ImageStoreIndex satIdx{2, 0};
            ImageRepository::ImageStoreIndex orbBgnd{ 0, 2};
            ImageRepository::ImageStoreIndex trackIdx{ 0, 1};
            mGeoChrono = mapArea->add<GeoChrono>()
                    ->withImageRepository(mIconRepository, satIdx, orbBgnd, trackIdx)
                    ->withObserver(mObserver)
                    ->withStationCoordinates(qthLatLon)
                    ->withBackgroundFile(string(map_path) + string(night_map))
                    ->withForegroundFile(string(map_path) + string(day_map))
                    ->withBackdropFile(string(background_path) + string(backdrop))
                    ->withFixedSize(Vector2i(EARTH_BIG_W, EARTH_BIG_H));

            mGeoChrono->setGeoData(vector<GeoChrono::PositionData>{ {qthLatLon.y, qthLatLon.x, true, ImageRepository::ImageStoreIndex{ 0, 0}},
                                                          {aQthLatLon.y, aQthLatLon.x, true, ImageRepository::ImageStoreIndex{0, 1} }});

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
                    ->add<ToolButton>(ENTYPO_ICON_CAMERA, Button::Flags::NormalButton)
                            ->withCallback([&](){
                                screenShot();
                            });

            auto tab = sideBar->add<TabWidget>();

            Widget *layer = tab->createTab("S", ENTYPO_ICON_ROCKET);
            layer->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 0));

            // Use overloaded variadic add to fill the tab widget with Different tabs.
            mSatelliteDataDisplay = layer->add<SatelliteDataDisplay>(mIconRepository, ImageRepository::ImageStoreIndex{2,0})
                    ->withFixedWidth(sideBarSize.x - 20);

            layer = tab->createTab("D", ENTYPO_ICON_LOCATION);
            layer->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 0));

            // Use overloaded variadic add to fill the tab widget with Different tabs.
            layer->add<Label>("Stations", "sans-bold")->withFixedWidth(sideBarSize.x - 20);

            tab->setActiveTab(0);

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

            mEphemerisModel.setObserver(mObserver);
            mEphemerisModel.loadEphemerisLibrary();
            mEphemerisModel.setSatellitesOfInterest(); //"ISS,AO-92,FO-99,IO-26,DIWATA-2,FOX-1B,AO-7,AO-27,AO-73,SO-50");
            mEphemerisModel.timerCallback(0);
        }

        void drawContents() override {
        }
    };
}

using namespace guipi;

class InputParser{
public:
    InputParser (int &argc, char **argv){
        for (int i=1; i < argc; ++i)
            this->tokens.push_back(std::string(argv[i]));
    }
    /// @author iain
    const std::string& getCmdOption(const std::string &option) const{
        std::vector<std::string>::const_iterator itr;
        itr =  std::find(this->tokens.begin(), this->tokens.end(), option);
        if (itr != this->tokens.end() && ++itr != this->tokens.end()){
            return *itr;
        }
        static const std::string empty_string;
        return empty_string;
    }
    /// @author iain
    bool cmdOptionExists(const std::string &option) const{
        return std::find(this->tokens.begin(), this->tokens.end(), option)
               != this->tokens.end();
    }
private:
    std::vector <std::string> tokens;
};


int main(int argc, char ** argv) {
    InputParser inputParser{argc, argv};

    Vector2f geoCoord{0., 0.};
    string callsign = inputParser.getCmdOption("-cs");
    const string latitude = inputParser.getCmdOption("-lat");
    const string longitude = inputParser.getCmdOption("-lon");

    if (callsign.empty())
        callsign = "CALL";
    if (!latitude.empty() && !longitude.empty()) {
        geoCoord.y = std::strtod(latitude.c_str(), nullptr);
        geoCoord.x = std::strtod(longitude.c_str(), nullptr);
    }

    string homdir{getenv("HOME")};
    if (system( "mkdir -p ~/.hamchrono/images"))
        std::cerr << "Could not make directory '~/.hamchrono/images'\n";

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

    sdlgui::ref<HamChrono> app{new HamChrono(window, winWidth, winHeight, homdir, callsign, geoCoord)};

    app->performLayout(app->sdlRenderer());

    app->eventLoop();

    SDL_DestroyRenderer(renderer);
    return 0;
}
