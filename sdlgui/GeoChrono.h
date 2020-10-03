//
// Created by richard on 2020-09-12.
//

#ifndef SDLGUI_GEOCHRONO_H
#define SDLGUI_GEOCHRONO_H


#include <utility>
#include <chrono>
#include <mutex>
#include <thread>
#include <sdlgui/widget.h>
#include <sdlgui/TimeBox.h>
#include <sdlgui/Image.h>


#define USE_COMPILED_MAPS 0
#define USER_SET_CENTRE_LONG 1

namespace sdlgui {

    template<typename T>
    constexpr T deg2rad(T deg) { return deg * M_PI / 180.; }

    template<typename T>
    constexpr T rad2deg(T rad) { return rad * 180. / M_PI; }

    std::tuple<double, double> subSolar();

    /**
     * @class GeoChrono
     * A minimalist Image display widget capable of resizing and displaying one image on an ImageList
     */
    class GeoChrono : public Widget {
    public:
        enum EventType {
            UP_EVENT, LEFT_EVENT, DOWN_EVENT, RIGHT_EVENT, CLICK_EVENT
        };

        static constexpr double GrayLineCos = -0.208;
        static constexpr double GrayLinePow = 0.75;

        Timer<GeoChrono> mTimer;

    private:
        struct SurfaceDeleter {
            void operator()(SDL_Surface *surface) {
                SDL_FreeSurface(surface);
            }
        };

        struct Surface : public unique_ptr<SDL_Surface,SurfaceDeleter> {
            auto &pixel(int x, int y) {
                auto *pixels = (Uint32 *) get()->pixels;
                return pixels[(y * get()->w) + x];
            }
        };

        mutex mTransparentMutex;
        thread mTransparentThread;
        atomic<bool> mTransparentReady{false};
        ImageData mForeground;      //< The foreground image
        ImageData mBackground;      //< The background image
        ImageData mForegroundAz;
        ImageData mBackgroundAz;
        ImageData mBackdropTex;
        Surface mTransparentMap;    //< The surface holding the day map with transparency
        Surface mTransparentMapAz;  //< The surface holding the day azmuthal map with transparency
        Surface mDayMap;            //< The surface holding the day map
        Surface mNightMap;          //< The surface holding the night map
        Surface mDayAzMap;          //< The surface holding the generated day Azmuthal map
        Surface mNightAzMap;        //< The surface holding the generated night Azuthal map
        Surface mBackdropImage;
        bool mBackdropDirty;
        bool mAzimuthalDisplay{false};
        bool mSunMoonDisplay{false};
        bool mTextureDirty{true};   //< True when the image needs to be re-drawn
        bool mMapsDirty{true};      //< True when the map surfaces need to be re-drawn

        bool mButton{false};        //< True when button 1 has been pressed
        bool mMotion{false};        //< True when the mouse has been in motion with button 1 pressed
        Vector2i mMotionStart{};    //< The starting point of the motion;
        Vector2i mMotionEnd{};      //< The ending point of the motion;

        Vector2f mStationLocation;  //< The longitude x, latitude y (in radians, West/South negative) of the station.

        std::function<void(GeoChrono &, EventType)> mCallback;

        /**
         * The timer callback
         * @param interval
         * @return the new interval
         */
        Uint32 timerCallback(Uint32 interval);


        /**
         * Generate day and night surfaces from compiled in data.
         */
        void generateMapSurfaces(SDL_Renderer *renderer);

        [[nodiscard]] auto computeOffset() const {
            return (int)round((2.*M_PI - mStationLocation.x) * ((float)EARTH_BIG_W / (2.f * M_PI))) % EARTH_BIG_W;
        }

        struct AsyncTexture;
        typedef std::shared_ptr<AsyncTexture> AsyncTexturePtr;
        std::vector<AsyncTexturePtr> _txs;

        ImageData mSunIcon;
        float mSubSolarLat, mSubSolarLon;

    public:
        /**
         * (Constructor)
         * Construct a GeoChrono with no images
         * @param parent
         */
        explicit GeoChrono(Widget *parent) : Widget(parent), mTimer(*this, &GeoChrono::timerCallback, 60000),
                               mDayMap{nullptr}, mNightMap{nullptr}, mTransparentMap{nullptr}, mStationLocation{0} {}

        bool mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;

        bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) override;

        /**
         * Override the Widget draw method.
         * @param renderer the renderer to draw to.
         */
        void draw(SDL_Renderer *renderer) override;

        /**
         * Building help, add a List of Images
         * @param foreground
         * @return a reference to this GeoChrono
         */
        ref<GeoChrono> withForeground(ImageData &foreground) {
            mForeground = std::move(foreground);
            return this;
        }

        ref<GeoChrono> withForegroundFile(const string &filePath) {
            mForeground.path = filePath;
            mMapsDirty = true;
            return this;
        }

        ref<GeoChrono> withBackdropFile(const string &filePath) {
            mBackdropTex.path = filePath;
            mBackdropDirty = true;
            return this;
        }

        /**
         * Building help, add a List of Images
         * @param background
         * @return a reference to this GeoChrono
         */
        ref<GeoChrono> withBackground(ImageData &background) {
            mBackground = std::move(background);
            return this;
        }

        ref<GeoChrono> withBackgroundFile(const string &filePath) {
            mBackground.path = filePath;
            mMapsDirty = true;
            return this;
        }

        /**
         * Building help, set the centre longitude.
         * @param stationLocation in degrees, West is negative
         * @return a reference to this GeoChrono.
         */
        ref<GeoChrono> withStationCoordinates(Vector2f stationLocation) {
            mStationLocation = stationLocation;
            mMapsDirty = true;
            mTextureDirty = true;
            return this;
        }

        /**
         * Get the currently set callback function.
         * @return the callback function.
         */
        [[nodiscard]] std::function<void(GeoChrono &, EventType)> callback() const { return mCallback; }

        /**
         * Set the call back function
         * @param callback the desired callback function, a simple but usefule lambda is:
         * [](GeoChrono &w, GeoChrono::EventType e) {
                            switch(e) {
                                case GeoChrono::RIGHT_EVENT:
                                case GeoChrono::DOWN_EVENT:
                                    w.setImageIndex(w.getImageIndex()+1);
                                    break;
                                case GeoChrono::LEFT_EVENT:
                                case GeoChrono::UP_EVENT:
                                    w.setImageIndex(w.getImageIndex()-1);
                                    break;
                            }
                        }
         * @return a reference to this GeoChrono
         */
        GeoChrono &setCallback(const std::function<void(GeoChrono &, EventType)> &callback) {
            mCallback = callback;
            return *this;
        }

        void setAzmuthalDisplay(bool azmuthal) {
            mAzimuthalDisplay = azmuthal;
        }

        ref<GeoChrono> withAzmuthalDisplay(bool azumthal) { setAzmuthalDisplay(azumthal); return this; }

        bool azmuthalDisplay() const { return mAzimuthalDisplay; }

        void setSunMoonDisplay(bool sunMoon) { mSunMoonDisplay = sunMoon; }

        bool sunMoonDisplay() const { return mSunMoonDisplay; }

        ref<GeoChrono> withSunMoonDisplay(bool sunMoon) { setSunMoonDisplay(sunMoon); return this; }

        void transparentForeground();

        /**
         * Convert a latitude longitude in radians to map coordinates.
         * @param lat latitude
         * @param lon longitude
         * @param mapSize the size of the map in pixels
         * @param location the station location for projection centring
         * @return a tuple with x and y co-ordinate.
         */
        tuple<int, int> latLongToMap(float lat, float lon);
    };

}


#endif //SDLGUI_GEOCHRONO_H
