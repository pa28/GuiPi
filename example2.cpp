/*
    sdlgui/example1.cpp -- C++ version of an example application that shows 
    how to use the various widget classes. 

    Based on NanoGUI by Wenzel Jakob <wenzel@inf.ethz.ch>.
    Adaptation for SDL by Dalerank <dalerankn8@gmail.com>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <fstream>
#include <sdlgui/common.h>
#include <sdlgui/screen.h>
#include <sdlgui/window.h>
#include <sdlgui/layout.h>
#include <sdlgui/label.h>
#include <sdlgui/checkbox.h>
#include <sdlgui/button.h>
#include <sdlgui/toolbutton.h>
#include <sdlgui/popupbutton.h>
#include <sdlgui/combobox.h>
#include <sdlgui/dropdownbox.h>
#include <sdlgui/progressbar.h>
#include <sdlgui/entypo.h>
#include <sdlgui/messagedialog.h>
#include <sdlgui/textbox.h>
#include <sdlgui/slider.h>
#include <sdlgui/imagepanel.h>
#include <sdlgui/imageview.h>
#include <sdlgui/vscrollpanel.h>
#include <sdlgui/colorwheel.h>
#include <sdlgui/graph.h>
#include <sdlgui/tabwidget.h>
#include <sdlgui/switchbox.h>
#include <sdlgui/formhelper.h>
#include <memory>
#include <sdlgui/TimeBox.h>
#include <sdlgui/ImageDisplay.h>
#include <sdlgui/GeoChrono.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include <iostream>

#if defined(_WIN32)
#include <SDL.h>
#else

#include <SDL2/SDL.h>

#endif
#if defined(_WIN32)
#include <SDL_image.h>
#else

#include <SDL2/SDL_image.h>

#endif

using std::cout;
using std::cerr;
using std::endl;

#undef main

using namespace sdlgui;

class Fps {
public:
    explicit Fps(int tickInterval = 30)
            : m_tickInterval(tickInterval), m_nextTime(SDL_GetTicks() + tickInterval) {
    }

    void next() {
        SDL_Delay(getTicksToNextFrame());

        m_nextTime += m_tickInterval;
    }

private:
    const int m_tickInterval;
    Uint32 m_nextTime;

    [[nodiscard]] Uint32 getTicksToNextFrame() const {
        Uint32 now = SDL_GetTicks();

        return (m_nextTime <= now) ? 0 : m_nextTime - now;
    }
};

class TestWindow : public Screen {
protected:

    static constexpr std::string_view brightnessDevice = "/sys/class/backlight/rpi_backlight/brightness";

    std::vector<SDL_Texture *> mImagesData;
    int mCurrentImage;

    Vector2f qthLatLon;
    Vector2i mScreenSize{800, 480};

    bool mHasBrightnessControl{true};
    bool mRunEventLoop{true};

    sdlgui::ref<GeoChrono> mGeoChrono;
    sdlgui::ref<ToolButton> mAzmuthalButton;
    sdlgui::ref<Window> mMainWindow;
    sdlgui::ref<Window> mPopupWindow;

public:
    ~TestWindow() override {
        cout << __PRETTY_FUNCTION__ << '\n';
        disposeWindow(mMainWindow.get());
    }

    template<typename T>
    constexpr T deg2rad(T deg) { return deg * M_PI / 180.; }

    static constexpr string_view map_path = "maps/";
    static constexpr string_view image_path = "images/";
    static constexpr string_view background_path = "backgrounds/";
    static constexpr string_view day_map = "day_earth_660x330.png";
    static constexpr string_view night_map = "night_earth_660x330.png";
    static constexpr string_view backdrop = "NASA_Nebula.png";

    void setBrightness(float brightness) {
        if (mHasBrightnessControl) {
            auto min = mTheme->mMinBrightness;
            auto max = mTheme->mMaxBrightness;
            auto bright = (float) (max - min) * brightness + (float) min;

            std::ofstream ofs;
            ofs.open(std::string(brightnessDevice), std::ofstream::out);
            if (ofs) {
                ofs << (int) bright << '\n';
                ofs.close();
            } else {
                // TODO: Better error reporting.
                mHasBrightnessControl = false;
                std::cerr << "Can not open " << brightnessDevice << std::endl;
            }
        }
    }

    void eventLoop() {
        try {
            SDL_Event e;
            Fps fps;

            while (mRunEventLoop) {
                //Handle events on queue
                while (SDL_PollEvent(&e) != 0) {
                    //User requests quit
                    if (e.type == SDL_QUIT) {
                        mRunEventLoop = false;
                        continue;
                    }

                    if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP) {
//                    onEvent(e);
                        SDL_Event mbe;

                        // Translate finger events to mouse evnets.
                        mbe.type = (e.type == SDL_FINGERDOWN) ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
                        mbe.button.timestamp = e.tfinger.timestamp;
                        mbe.button.windowID = SDL_GetWindowID(window());
                        mbe.button.which = SDL_TOUCH_MOUSEID;
                        mbe.button.button = SDL_BUTTON_LEFT;
                        mbe.button.state = (e.type == SDL_FINGERDOWN) ? SDL_PRESSED : SDL_RELEASED;
                        mbe.button.clicks = 1;
                        mbe.button.x = (Sint32) (e.tfinger.x * (float) mScreenSize.x);
                        mbe.button.y = (Sint32) (e.tfinger.y * (float) mScreenSize.y);
                        SDL_WarpMouseGlobal(mbe.button.x, mbe.button.y);
                        SDL_PushEvent(&mbe);
                    } else if (e.type == SDL_FINGERMOTION) {
                        //printEvent( std::cout, e );
                        SDL_WarpMouseGlobal((Sint32) (e.tfinger.x * (float) mScreenSize.x),
                                            (Sint32) (e.tfinger.y * (float) mScreenSize.y));
                    } else
                        onEvent(e);
                }

                SDL_SetRenderDrawColor(mSDL_Renderer, 0x0, 0x0, 0x0, 0xff);
                SDL_RenderClear(mSDL_Renderer);

                drawAll();

                // Render the rect to the screen
                SDL_RenderPresent(mSDL_Renderer);

                fps.next();
            }
        }

        catch (const std::runtime_error &e) {
            throw e;
        }
    }


    TestWindow(SDL_Window *pwindow, int rwidth, int rheight)
            : Screen(pwindow, Vector2i(rwidth, rheight), "Raspberry Pi") {

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

        auto sun_images = loadImageDataDirectory(mSDL_Renderer, string(image_path));

        // Create an image repeater to use with the image display.
        sdlgui::ref<ImageRepeater> imageRepeater = add<ImageRepeater>(Vector2i(210, 0), Vector2i(450, 450));
        topArea->add<ImageDisplay>()->withImages(sun_images)
                ->withRepeater(imageRepeater)
                ->setCallback([=](ImageDisplay &w, ImageDisplay::EventType e) {
                    switch (e) {
                        case ImageDisplay::RIGHT_EVENT:
                        case ImageDisplay::DOWN_EVENT:
                            w.setImageIndex(w.getImageIndex() + 1);
                            break;
                        case ImageDisplay::LEFT_EVENT:
                        case ImageDisplay::UP_EVENT:
                            w.setImageIndex(w.getImageIndex() - 1);
                            break;
                        case ImageDisplay::CLICK_EVENT:
                            w.repeatImage();
                            break;
                    }
                })
                ->withFixedWidth(topAreaSize.y)
                ->withFixedHeight(topAreaSize.y)
                ->withId("smallimages");

        topArea->add<Widget>()->withFixedSize(Vector2i(topAreaSize.y,topAreaSize.y));

        topArea->add<Widget>()->withFixedSize(Vector2i(topAreaSize.y,topAreaSize.y));

        auto mapArea = botArea->add<Widget>()->withFixedSize(mapAreaSize)->withId("mapArea")
                ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 0, 0);

        mGeoChrono = mapArea->add<GeoChrono>()
                ->withStationCoordinates(qthLatLon)
                ->withBackgroundFile(string(map_path) + string(night_map))
                ->withForegroundFile(string(map_path) + string(day_map))
                ->withBackdropFile(string(background_path) + string(backdrop))
                ->withFixedSize(Vector2i(EARTH_BIG_W, EARTH_BIG_H));

        auto switches = topArea->add<Widget>()->withPosition(Vector2i(620, 0))
                ->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0);

        switches->add<Widget>()->withPosition(Vector2i(620, 0))
                ->withFixedSize(Vector2i(40, topAreaSize.y))
                ->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 12)
                ->add<ToolButton>(ENTYPO_ICON_COMPASS, Button::Flags::ToggleButton)->_and()
                ->add<ToolButton>(ENTYPO_ICON_MOON, Button::Flags::ToggleButton)
                ->withPushed(/*mGeoChrono->sunMoonDisplay()*/ false )
                ->withChangeCallback([&](bool state) {} )
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

        performLayout(mSDL_Renderer);
    }

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
        if (Screen::keyboardEvent(key, scancode, action, modifiers))
            return true;

        //if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        // {
        //    setVisible(false);
        //    return true;
        //}
        return false;
    }

    virtual void draw(SDL_Renderer *renderer) {
        if (auto pbar = gfind<ProgressBar>("progressbar")) {
            pbar->setValue(pbar->value() + 0.001f);
            if (pbar->value() >= 1.f)
                pbar->setValue(0.f);
        }

        Screen::draw(renderer);
    }

    virtual void drawContents() {
    }

private:
};


int main(int /* argc */, char ** /* argv */) {
    char rendername[256] = {0};
    SDL_RendererInfo info;

    SDL_Init(SDL_INIT_VIDEO);   // Initialize SDL2

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

    sdlgui::ref<TestWindow> screen = sdlgui::ref<TestWindow>(new TestWindow(window, winWidth, winHeight));

#if 1
    screen->eventLoop();
#else
    Fps fps;

    bool quit = false;
    try {
        SDL_Event e;
        while (!quit) {
            //Handle events on queue
            while (SDL_PollEvent(&e) != 0) {
                //User requests quit
                if (e.type == SDL_QUIT) {
                    quit = true;
                }
                screen->onEvent(e);
            }

            SDL_SetRenderDrawColor(renderer, 0xd3, 0xd3, 0xd3, 0xff);
            SDL_RenderClear(renderer);

            screen->drawAll();

            // Render the rect to the screen
            SDL_RenderPresent(renderer);

            fps.next();
        }
    }
    catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
#if defined(_WIN32)
        MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
        std::cerr << error_msg << endl;
#endif
        return -1;
    }
#endif
    SDL_DestroyRenderer(renderer);
    return 0;
}
