//
// Created by richard on 2020-09-12.
//

#pragma once

#include <utility>
#include <chrono>
#include <future>
#include <sdlgui/widget.h>
#include <sdlgui/TimeBox.h>
#include <sdlgui/Image.h>
#include <guipi/EphemerisModel.h>
#include <sdlgui/ImageRepository.h>
#include <guipi/GfxPrimitives.h>
#include <guipi/PassTracker.h>

namespace guipi {
    using namespace sdlgui;
    using sdlgui::ref;

    /**
     * Compute an antipode
     * @param location y -> latitude, x-> longitude in radians
     * @return the antipode
     */
    static Vector2f antipode(const Vector2f &location) {
        return Vector2f{(location.x < 0 ? 1.f : -1.f) * ((float) M_PI - abs(location.x)), -location.y};
    }

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

        struct PositionData {
            float lat, lon;
            bool mapLocDirty;
            ImageRepository::ImageStoreIndex iconIdx;
            Vector2i mapLoc;
        };

    private:
        future<bool> mTransparentFuture;
        atomic_bool mTransparentReady;
        atomic_bool mTextureDirty{true};   //< True when the image needs to be re-drawn
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
        ref<ImageRepository> mIconRepository;
        bool mBackdropDirty{};
        bool mAzimuthalDisplay{false};
        bool mAzimuthalEffective{false};
        bool mSunMoonDisplay{false};
        bool mSatelliteDisplay{false};
        bool mMapsDirty{true};      //< True when the map surfaces need to be re-drawn

        bool mButton{false};        //< True when button 1 has been pressed
        bool mMotion{false};        //< True when the mouse has been in motion with button 1 pressed
        Vector2i mMotionStart{};    //< The starting point of the motion;
        Vector2i mMotionEnd{};      //< The ending point of the motion;

        Vector2f mStationLocation;  //< The longitude x, latitude y (in radians, West/South negative) of the station.
        Observer mObserver{};

        EphemerisModel::OrbitTrackingData mNewOrbitData;

        ImageRepository::ImageStoreIndex mBaseIconIndex;
        ImageRepository::ImageStoreIndex mOrbitBackgroundIndex;

        vector<PositionData> mWorkingOrbitData;
        vector<PositionData> mWorkingGeoData;
        vector<PositionData> mNewGeoData;

        vector<PositionData> mWorkingCelestialData;
        EphemerisModel::CelestialTrackingData mNewCellestialData;

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

    public:
        ref<PassTracker> mPassTracker;
    protected:
        friend class PassTracker;
        bool setAzimuthalEffective() {
            bool ae = mAzimuthalDisplay | (mPassTracker->activeTracking() && mSatelliteDisplay);
            if (mAzimuthalEffective != ae) {
                mAzimuthalEffective = ae;
                invalidateMapCoordinates();
            }
            mPassTracker->setVisible(mPassTracker->activeTracking() && mSatelliteDisplay);
            return mAzimuthalEffective;
        }

        static bool asyncTransparentForeground(GeoChrono *self) {
            return self->transparentForeground();
        }

    public:
        /**
         * (Constructor)
         * Construct a GeoChrono with no images
         * @param parent
         */
        explicit GeoChrono(Widget *parent);

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
        ref<GeoChrono> withForegroundFile(const string &filePath) {
            mForeground.path = filePath;
            mMapsDirty = true;
            return ref<GeoChrono>{this};
        }

        ref<GeoChrono> withBackdropFile(const string &filePath) {
            mBackdropTex.path = filePath;
            mBackdropDirty = true;
            return ref<GeoChrono>{this};
        }

        /**
         * Building help, add a List of Images
         * @param background
         * @return a reference to this GeoChrono
         */
        ref<GeoChrono> withBackgroundFile(const string &filePath) {
            mBackground.path = filePath;
            mMapsDirty = true;
            return ref<GeoChrono>{this};
        }

        /**
         * Building help, set the centre longitude.
         * @param stationLocation in degrees, West is negative
         * @return a reference to this GeoChrono.
         */
        ref<GeoChrono> withStationCoordinates(const Vector2f& stationLocation) {
            mStationLocation = stationLocation;
            mMapsDirty = true;
            mTextureDirty = true;
            return ref<GeoChrono>{this};
        }

        ref<GeoChrono> withObserver(const Observer observer) {
            mObserver = observer;
            if (mPassTracker)
                mPassTracker->withObserver(observer);
            return ref<GeoChrono>{this};
        }

        void setGeoData(vector<PositionData> newGeoData) { mNewGeoData = move(newGeoData); }

        void setCelestialTrackingData(EphemerisModel::CelestialTrackingData data) { mNewCellestialData = move(data); }

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
            setAzimuthalEffective();
        }

        void invalidateMapCoordinates() {
            for (auto &orbit : mWorkingOrbitData)
                orbit.mapLocDirty = true;
            for (auto &geo : mWorkingGeoData)
                geo.mapLocDirty = true;
            for (auto &cel : mWorkingCelestialData)
                cel.mapLocDirty = true;
        }

        ref<GeoChrono> withAzmuthalDisplay(bool azumthal) { setAzmuthalDisplay(azumthal); return ref<GeoChrono>{this}; }

        bool azmuthalDisplay() const { return mAzimuthalDisplay; }

        void setSunMoonDisplay(bool sunMoon) { mSunMoonDisplay = sunMoon; }

        bool sunMoonDisplay() const { return mSunMoonDisplay; }

        ref<GeoChrono> withSunMoonDisplay(bool sunMoon) { setSunMoonDisplay(sunMoon); return ref<GeoChrono>{this}; }

        void setSatelliteDisplay(bool satellite) {
            mSatelliteDisplay = satellite;
            if (satellite) {
                if (!mPassTracker->empty())
                    mPassTracker->setVisible(satellite);
            } else
                mPassTracker->setVisible(satellite);
        }

        bool satelliteDisplay() const { return mSatelliteDisplay; }

        ref<GeoChrono> withSatelliteDisplay(bool satellite) { setSatelliteDisplay(satellite); return ref<GeoChrono>{this}; }

        ref<GeoChrono> withImageRepository(const ref<ImageRepository> &iconRepository,
                                           ImageRepository::ImageStoreIndex &baseIdx,
                                           ImageRepository::ImageStoreIndex &orbBgrd,
                                           ImageRepository::ImageStoreIndex &passIdx) {
            mIconRepository = iconRepository;
            mBaseIconIndex = baseIdx;
            mOrbitBackgroundIndex = orbBgrd;
            mPassTracker->withIconRepository(iconRepository, passIdx);
            return ref<GeoChrono>{this};
        }

        void setOrbitalData(EphemerisModel::OrbitTrackingData data) {
            mNewOrbitData = move(data);
        }

        void setPasStrackingData(const EphemerisModel::PassTrackingData &data) {
            if (mPassTracker)
                mPassTracker->setPassTrackingData(data);
        }

        bool transparentForeground();

        /**
         * Render an icon on the map at a given geographic location.
         * @param renderer
         * @param mapLocation the coordinates of the top, left corner of the map on the screen
         * @param geoCoord the geographic coordinate of the icon
         * @param icon the icon pre-rendered as a texture.
         */
//        void renderMapIcon(SDL_Renderer *renderer, const Vector2i& mapLocation, PlotPackage &plotItem) const;

        /*
         * Convert a latitude longitude in radians to map coordinates.
         * @param lat latitude
         * @param lon longitude
         * @param mapSize the size of the map in pixels
         * @param location the station location for projection centring
         * @return a tuple with x and y co-ordinate.
         */
        Vector2i latLongToMap(float lat, float lon);

        auto passTracker() const { return mPassTracker; }

        vector <pair<SDL_Rect, SDL_Rect>>
        renderMapIconRect(const Vector2i &mapLocation, const Vector2i &geoCoord, const Vector2i &iconsSize) const;
    };

}
