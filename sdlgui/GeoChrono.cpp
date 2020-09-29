//
// Created by richard on 2020-09-12.
//

#include <cstdint>
#include <cmath>
#include <tuple>
#include <functional>
#include <thread>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_pixels.h>
#include <sdlgui/screen.h>
#include <sdlgui/Image.h>
#include "GeoChrono.h"

#include "nanovg.h"

#define NANOVG_RT_IMPLEMENTATION
#define NANORT_IMPLEMENTATION

#include "nanovg_rt.h"

namespace sdlgui {

    /**
     * Handle mouse motion events
     * @param p postion of the mouse
     * @param rel the relative motion since the last event
     * @param button which button was actioned (it must be down)
     * @param modifiers keyboard modifiers
     * @return true if processed.
     */
    bool GeoChrono::mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button,
                                     int modifiers) {
        if (button) {
            if (!mMotion) {
                mMotionStart = p;
                mMotionEnd = p;
                mButton = mMotion = true;
            } else {
                mMotionEnd = p;
            }
        }
        return Widget::mouseMotionEvent(p, rel, button, modifiers);
    }

    /**
     * Handle mouse button events
     * @param p postion of the mouse
     * @param button which button was actioned
     * @param down true if the button is down
     * @param modifiers keyboard modifiers
     * @return true if processed.
     */
    bool GeoChrono::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
        if (button) {
            if (down) {
                if (!mButton) {
                    mMotion = false;
                }
                mButton = true;
            } else {
                if (mCallback) {
                    auto d = mMotionEnd - mMotionStart;
                    if (mMotion && d.x * d.x + d.y * d.y > 25) {
                        if (abs(d.y) >= abs(d.x)) {
                            if (d.y > 0)
                                mCallback(*this, DOWN_EVENT);
                            else
                                mCallback(*this, UP_EVENT);
                        } else {
                            if (d.x > 0)
                                mCallback(*this, RIGHT_EVENT);
                            else
                                mCallback(*this, LEFT_EVENT);
                        }
                        mMotionStart = mMotionEnd = Vector2i(0, 0);
                        mMotion = mButton = false;
                    } else {
                        mCallback(*this, CLICK_EVENT);
                    }
                }
            }
        }
        return Widget::mouseButtonEvent(p, button, down, modifiers);
    }

/* solve a spherical triangle:
 *           A
 *          /  \
 *         /    \
 *      c /      \ b
 *       /        \
 *      /          \
 *    B ____________ C
 *           a
 *
 * given A, b, c find B and a in range -PI..B..PI and 0..a..PI, respectively..
 * cap and Bp may be NULL if not interested in either one.
 * N.B. we pass in cos(c) and sin(c) because in many problems one of the sides
 *   remains constant for many values of A and b.
 */
    void solveSphere(float A, float b, float cc, float sc, float &cap, float &Bp) {
        float cb = cosf(b), sb = sinf(b);
        float sA, cA = cosf(A);
        float x, y;
        float ca;
        float B;

        ca = cb * cc + sb * sc * cA;
        if (ca > 1.0F) ca = 1.0F;
        if (ca < -1.0F) ca = -1.0F;
        cap = ca;

        if (sc < 1e-7F)
            B = cc < 0 ? A : (float) M_PI - A;
        else {
            sA = sinf(A);
            y = sA * sb * sc;
            x = cb - ca * cc;
            B = y != 0.0 ? (x != 0.0 ? atan2(y, x) : (y > 0.0 ? M_PI_2 : -M_PI_2)) : (x >= 0.0 ? 0.0 : M_PI);
        }

        Bp = B;
    }

    /**
     * Transform a Mercator map pixel into an Azimuthal map latitude and longitude in radians
     * @param x The map x pixel location 0 on the left
     * @param y The map y pixel location 0 at the top
     * @param mapSize the width (x) and height (y) of the map in pixels
     * @param location the longitude (x) and latitude (y) of the center of the projection
     * @param siny pre-computed sine of the latitude
     * @param cosy pre-computed cosine of the latitude
     * @return [valid, latitude, longitude ], valid if the pixel is on the Earth,
     * latitude -PI..+PI West to East, longitude +PI/2..-PI/2 North to South
     */
    tuple<bool, float, float>
    xyToAzLatLong(int x, int y, const Vector2i &mapSize, const Vector2f &location, float siny, float cosy) {
        bool onAntipode = x > mapSize.x / 2;
        auto w2 = (mapSize.y / 2) * (mapSize.y / 2);
        auto dx = onAntipode ? x - (3 * mapSize.x) / 4 : x - mapSize.x / 4;
        auto dy = mapSize.y / 2 - y;
        auto r2 = dx * dx + dy * dy;    // radius squared

        if (r2 <= w2) {
            auto b = sqrt((float) r2 / (float) w2) * M_PI_2;
            auto A = M_PI_2 - atan2((float) dy, (float) dx);

            float ca, B;
            solveSphere(A, b, (onAntipode ? -siny : siny), cosy, ca, B);
            auto lt = (float) M_PI_2 - acos(ca);
            auto lat_d = rad2deg(lt);
            auto lg = fmod(location.x + B + (onAntipode ? 6. : 5.) * (float) M_PI, 2 * M_PI) - (float) M_PI;
            auto long_d = rad2deg(lg);
            return tuple<bool, float, float>{true, lt, lg};
        }
        return tuple<bool, float, float>{false, 0, 0};
    }

    /**
     * Draw the Geographic Chronograph
     * @param renderer
     */
    void GeoChrono::draw(SDL_Renderer *renderer) {
        int ax = getAbsoluteLeft();
        int ay = getAbsoluteTop();

        PntRect clip = getAbsoluteCliprect();
        SDL_Rect clipRect = pntrect2srect(clip);

        /**
         * Maps are dirty when the base images have changed, or been loaded.
         */
        if (mMapsDirty) {
            generateMapSurfaces(renderer);
        }

        if (mBackdropDirty) {
            mBackdropDirty = false;
            mBackdropImage.reset(IMG_Load(mBackdropTex.path.c_str()));
            if (mBackdropImage) {
                mBackdropTex.set(SDL_CreateTextureFromSurface(renderer, mBackdropImage.get()));
            }
        }

        // Ensure all the maps are here
        if (mDayMap && mNightMap && mDayAzMap and mNightAzMap) {
            Vector2i p = Vector2i(0, 0);
            p += Vector2i(ax, ay);
            int imgw = mForeground.w;
            int imgh = mForeground.h;

            // If the asynchronous drawing thread is done, join it.
            if (mTransparentThread.joinable()) {
                mTransparentThread.join();
            }

            // Textures are dirty when there is an event that makes them out of date with the
            // desired display state. Periodically for the sun illumination foot print.
            // Spawn up a thread to get things back in sync in the background.
            if (mTextureDirty) {
                mTransparentThread = thread([this, renderer]() {
                    lock_guard<mutex> lockGuard(mTransparentMutex);
                    transparentForeground();
                    mTextureDirty = false;
                });
            }

            // Regardless of the current state carry on using the old textures until new ones are ready.

            // If they are ready, replace the old ones.
            if (mTransparentReady) {
                lock_guard<mutex> lockGuard(mTransparentMutex);

                mForegroundAz.set(SDL_CreateTextureFromSurface(renderer, mTransparentMapAz.get()));
                mForegroundAz.h = mTransparentMapAz->h;
                mForegroundAz.w = mTransparentMapAz->w;
                mForegroundAz.name = "*autogen*";

                mForeground.set(SDL_CreateTextureFromSurface(renderer, mTransparentMap.get()));
                mForeground.h = mTransparentMap->h;
                mForeground.w = mTransparentMap->w;
                mForeground.name = "*autogen*";
                mTransparentReady = false;

                mTransparentReady = false;
            }

            // Display the map with solar illumination by stacking the day map (transparent where it is dark)
            // on top of the night map.
            if (mAzimuthalDisplay) {
                SDL_BlendMode mode;
                SDL_GetTextureBlendMode(mForegroundAz.get(), &mode);
                SDL_SetTextureBlendMode(mForegroundAz.get(), SDL_BLENDMODE_BLEND);
                SDL_SetTextureBlendMode(mBackgroundAz.get(), SDL_BLENDMODE_BLEND);

                SDL_Rect src{0, 0, mForegroundAz.w, mForegroundAz.h};
                SDL_Rect dst{p.x, p.y, mForegroundAz.w, mForegroundAz.h};
                if (mBackdropTex)
                    SDL_RenderCopy(renderer, mBackdropTex.get(), &src, &dst);
                SDL_RenderCopy(renderer, mBackgroundAz.get(), &src, &dst);
                SDL_RenderCopy(renderer, mForegroundAz.get(), &src, &dst);
            } else {
                auto offset = computeOffset();

                SDL_BlendMode mode;
                SDL_GetTextureBlendMode(mForeground.get(), &mode);
                SDL_SetTextureBlendMode(mForeground.get(), SDL_BLENDMODE_BLEND);
                SDL_SetTextureBlendMode(mBackground.get(), SDL_BLENDMODE_BLEND);

                SDL_Rect src0{mForeground.w - (int) offset, 0, (int) offset, imgh};
                SDL_Rect dst0{p.x, p.y, (int) offset, imgh};
                SDL_RenderCopy(renderer, mBackground.get(), &src0, &dst0);
                SDL_RenderCopy(renderer, mForeground.get(), &src0, &dst0);

                SDL_Rect src1{0, 0, mForeground.w - (int) offset, imgh};
                SDL_Rect dst1{p.x + (int) offset, p.y, mForeground.w - (int) offset, imgh};
                SDL_RenderCopy(renderer, mBackground.get(), &src1, &dst1);
                SDL_RenderCopy(renderer, mForeground.get(), &src1, &dst1);
            }
        }

        Widget::draw(renderer);
    }

    /**
     * Generate Mercator and Azimuthal maps from a set of Mercator maps which are images on disk.
     * @param renderer
     */
    void GeoChrono::generateMapSurfaces(SDL_Renderer *renderer) {

        // Initialize surfaces for each layer of each map including transparent versions of the day map
        mTransparentMap.reset(SDL_CreateRGBSurface(0, EARTH_BIG_W, EARTH_BIG_H, 32, rmask, gmask, bmask, amask));
        mTransparentMapAz.reset(SDL_CreateRGBSurface(0, EARTH_BIG_W, EARTH_BIG_H, 32, rmask, gmask, bmask, amask));
        mDayMap.reset(SDL_CreateRGBSurface(0, EARTH_BIG_W, EARTH_BIG_H, 32, rmask, gmask, bmask, amask));
        mNightMap.reset(SDL_CreateRGBSurface(0, EARTH_BIG_W, EARTH_BIG_H, 32, rmask, gmask, bmask, amask));
        mDayAzMap.reset(SDL_CreateRGBSurface(0, EARTH_BIG_W, EARTH_BIG_H, 32, rmask, gmask, bmask, amask));
        mNightAzMap.reset(SDL_CreateRGBSurface(0, EARTH_BIG_W, EARTH_BIG_H, 32, rmask, gmask, bmask, amask));

        // Use a temporary surface to load the maps and BLIT them onto the Mercator surfaces.
        // This corrects for any size anomalies
        Surface pngFile;
        pngFile.reset(IMG_Load(mForeground.path.c_str()));
        SDL_BlitSurface(pngFile.get(), nullptr, mDayMap.get(), nullptr);
        pngFile.reset(IMG_Load(mBackground.path.c_str()));
        SDL_BlitSurface(pngFile.get(), nullptr, mNightMap.get(), nullptr);

        // Compute Azmuthal maps from the Mercator maps
        float siny = sin(mStationLocation.y);
        float cosy = cos(mStationLocation.y);
        for (int y = 0; y < mDayMap->h; y += 1) {
            for (int x = 0; x < mDayMap->w; x += 1) {
                // Radius from centre of the hempishpere
                auto[valid, lat, lon] = xyToAzLatLong(x, y, Vector2i(EARTH_BIG_W, EARTH_BIG_H), mStationLocation, siny,
                                                      cosy);
                auto lat_d = rad2deg(lat);
                auto lon_d = rad2deg(lon);

                if (valid) {
                    auto xx = min(EARTH_BIG_W - 1, (int) round((float) EARTH_BIG_W * ((lon + M_PI) / (2 * M_PI))));
                    auto yy = min(EARTH_BIG_H - 1, (int) round((float) EARTH_BIG_H * ((M_PI_2 - lat) / M_PI)));
                    mDayAzMap.pixel(x, y) = mDayMap.pixel(xx, yy);
                    mNightAzMap.pixel(x, y) = mNightMap.pixel(xx, yy);
                }
            }
        }

        // The background (night) maps are good the way they are so save them for later.
        mBackground.set(SDL_CreateTextureFromSurface(renderer, mNightMap.get()));
        mBackground.w = EARTH_BIG_W;
        mBackground.h = EARTH_BIG_H;
        mBackground.name = "*auto_gen*";

        mBackgroundAz.set(SDL_CreateTextureFromSurface(renderer, mNightAzMap.get()));
        mBackgroundAz.w = EARTH_BIG_W;
        mBackgroundAz.h = EARTH_BIG_H;
        mBackgroundAz.name = "*auto_gen*";

        // The maps are good, but not current for the situation.
        mMapsDirty = false;
        mTextureDirty = true;
    }

    /**
     * The callback stub for the timer that invalidates the textures at regular intervals
     * so the solar illumination can be kept in time with real life.
     * @param interval
     * @return
     */
    Uint32 GeoChrono::timerCallback(Uint32 interval) {
        mTextureDirty = true;
        return interval;
    }

    /**
     * Compute the sub-solar geographic coordinates, used in plotting the solar ilumination.
     * @return a tuple with the latitude, longitude in radians
     */
    std::tuple<double, double> subSolar() {
        using namespace std::chrono;
        auto epoch = system_clock::now();
        time_t tt = system_clock::to_time_t(epoch);

        double JD = (tt / 86400.0) + 2440587.5;
        double D = JD - 2451545.0;
        double g = 357.529 + 0.98560028 * D;
        double q = 280.459 + 0.98564736 * D;
        double L = q + 1.915 * sin(M_PI / 180 * g) + 0.020 * sin(M_PI / 180 * 2 * g);
        double e = 23.439 - 0.00000036 * D;
        double RA = 180 / M_PI * atan2(cos(M_PI / 180 * e) * sin(M_PI / 180 * L), cos(M_PI / 180 * L));
        auto lat = asin(sin(M_PI / 180 * e) * sin(M_PI / 180 * L));
        auto lat_d = rad2deg(lat);
        double GMST = fmod(15 * (18.697374558 + 24.06570982441908 * D), 360.0);
        auto lng_d = fmod(RA - GMST + 36000.0 + 180.0, 360.0) - 180.0;
        auto lng = deg2rad(lng_d);

        return std::make_tuple(lat, lng);
    }

    /**
     * Plot the solar illumination area in the Alpha channel of the daytime map for Mercator and Azimuthal.
     */
    void GeoChrono::transparentForeground() {

        mTransparentMap.reset(SDL_CreateRGBSurface(0, mDayMap->w, mDayMap->h, 32, rmask, gmask, bmask, amask));
        mTransparentMapAz.reset(SDL_CreateRGBSurface(0, mDayMap->w, mDayMap->h, 32, rmask, gmask, bmask, amask));

        SDL_SetSurfaceBlendMode(mDayAzMap.get(), SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(mDayAzMap.get(), nullptr, mTransparentMapAz.get(), nullptr);

        SDL_SetSurfaceBlendMode(mDayMap.get(), SDL_BLENDMODE_BLEND);
        SDL_BlitSurface(mDayMap.get(), nullptr, mTransparentMap.get(), nullptr);

        auto[latS, lonS] = subSolar();
        float siny = sin(mStationLocation.y);
        float cosy = cos(mStationLocation.y);

        // Three loops for: Longitude, Latitude, and map form (Mercator, Azimuthal).
        // This lets us use the common calculations without repeating them, easier than
        // debugging two areas with the same computation.
        for (int x = 0; x < mTransparentMap->w; x += 1) {
            for (int y = 0; y < mTransparentMap->h; y += 1) {
                for (int az = 0; az < 2; ++az) {
                    uint32_t alpha = 255;
                    bool valid;
                    float latE;
                    float lonE;
                    if (az == 1) {
                        // The Azimuthal coordinates that correspond to a map pixel
                        auto tuple = xyToAzLatLong(x, y, Vector2i(EARTH_BIG_W, EARTH_BIG_H),
                                                   mStationLocation, siny, cosy);
                        valid = get<0>(tuple);
                        latE = get<1>(tuple);
                        lonE = get<2>(tuple);
                    } else {
                        // The Mercator coordinates for the same map pixel
                        valid = true;
                        lonE = (float) ((float) x - (float) mTransparentMap->w / 2.f) * (float) M_PI /
                               (float) ((float) mTransparentMap->w / 2.);
                        latE = (float) ((float) mTransparentMap->h / 2.f - (float) y) * (float) M_PI_2 /
                               (float) ((float) mTransparentMap->h / 2.);
                    }
                    if (valid) {
                        // Compute the amont of solar illumination and use it to compute the pixel alpha value
                        // GrayLineCos sets the interior angle between the sub-solar point and the location.
                        // GrayLinePower sets how fast it gets dark.
                        auto cosDeltaSigma = sin(latS) * sin(latE) + cos(latS) * cos(latE) * cos(abs(lonS - lonE));
                        double fract_day;
                        if (cosDeltaSigma < 0) {
                            if (cosDeltaSigma > GrayLineCos) {
                                fract_day = 1.0 - pow(cosDeltaSigma / GrayLineCos, GrayLinePow);
                                alpha = (uint32_t) (fract_day * 247.0) + 8;
                            } else
                                alpha = 8;  // Set the minimun alpha to keep some daytime colour on the night side
                        }
                    } else
                        alpha = 0;

                    // Set the alpha channel in the appropriate map.
                    if (az == 1) {
                        auto pixel = set_a_value(mTransparentMapAz.pixel(x, y), alpha);
                        mTransparentMapAz.pixel(x, y) = pixel;
                    } else {
                        auto pixel = set_a_value(mTransparentMap.pixel(x, y), alpha);
                        mTransparentMap.pixel(x, y) = pixel;
                    }
                }
            }
        }

        // They're ready!
        mTransparentReady = true;
    }
}
