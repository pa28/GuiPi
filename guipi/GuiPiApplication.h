//
// Created by richard on 2020-10-02.
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

#include <string>
#include <fstream>
#include <SDL.h>
#include <sdlgui/common.h>
#include <sdlgui/Image.h>
#include <sdlgui/screen.h>

namespace guipi {
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

    class GuiPiApplication : public Screen {
    protected:

#if __cplusplus == 201703L
        static constexpr std::string_view brightnessDevice = "/sys/class/backlight/rpi_backlight/brightness";
#else
#define brightnessDevice "/sys/class/backlight/rpi_backlight/brightness"
#endif
        bool mHasBrightnessControl{true};
        bool mRunEventLoop{true};
        Vector2i mScreenSize;

    public:
        ~GuiPiApplication() override = default;

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

        void exitApplication() {
            mRunEventLoop = false;
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

        /**
         * Create a texture of an icon
         * @param renderer
         * @param iconCode the font code point for the icon
         * @param iconSize the point size of the font generating the icon
         * @param iconColor
         * @return
         */
        ImageData createIcon(int iconCode, int iconSize, const Color &iconColor);

        GuiPiApplication(SDL_Window *pwindow, int rwidth, int rheight, const std::string &caption = "GuiPi")
                : Screen(pwindow, Vector2i(rwidth, rheight), caption) {

            mScreenSize.x = rwidth;
            mScreenSize.y = rheight;

//            virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
//                if (Screen::keyboardEvent(key, scancode, action, modifiers))
//                    return true;
//
//                //if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
//                // {
//                //    setVisible(false);
//                //    return true;
//                //}
//                return false;
            }

//            virtual void draw(SDL_Renderer *renderer) {
//                if (auto pbar = gfind<ProgressBar>("progressbar")) {
//                    pbar->setValue(pbar->value() + 0.001f);
//                    if (pbar->value() >= 1.f)
//                        pbar->setValue(0.f);
//                }
//
//                Screen::draw(renderer);
//            }
//
//            virtual void drawContents() {
//            }

        };

    /**
     * Recommended main()
     * @return exit status
     */
#if 0
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

        // Replace GuiPiApplication with your class derived from GuiPiApplication
        sdlgui::ref<GuiPiApplication> screen = new GuiPiApplication(window, winWidth, winHeight, "Caption");

        screen->performLayout(screen->sdlRenderer());

        screen->eventLoop();

        SDL_DestroyRenderer(renderer);
        return 0;
    }
#endif
}



