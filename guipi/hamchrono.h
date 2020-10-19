//
// Created by richard on 2020-10-19.
//

#pragma once

#include <sdlgui/common.h>
#include <sdlgui/entypo.h>
#include <guipi/EphemerisModel.h>
#include <guipi/GuiPiApplication.h>
#include <guipi/GeoChrono.h>
#include <guipi/SatelliteDataDisplay.h>
#include <guipi/Settings.h>


namespace guipi {

    /**
     * @class
     * HamChrono is the main class of the application by the same name.
     */
    class HamChrono : public GuiPiApplication {
    protected:

        Vector2f qthLatLon;     //!< The station location (Latitude Longitude) in radians
        Vector2f aQthLatLon;    //!< The station antipode.
        Vector2i mScreenSize{DISPLAY_WIDTH, DISPLAY_HEIGHT};
        Observer mObserver{};   //!< Observer for pass prediction (Latitude, Longitude, Altitude ) in degrees/meters.

        //* Local references to child widgets
        sdlgui::ref<GeoChrono> mGeoChrono;      //!< The GeoChron widget
        sdlgui::ref<Window> mMainWindow;        //!< The main window
        sdlgui::ref<ImageRepository> mImageRepository;      //!< The image repository for images
        sdlgui::ref<ImageRepository> mIconRepository;       //!< The image repository for icons
        sdlgui::ref<SatelliteDataDisplay> mSatelliteDataDisplay;        //!< Satellite data display widget

        EphemerisModel mEphemerisModel;       //!< The epheris model, includes the current library

    public:
        ~HamChrono() override = default;

        Timer<HamChrono> mTimer;        //!< An interval timer, computing satellite predictions

        // Path names to installed resources
        static constexpr string_view map_path = "/var/lib/hamchrono/maps/";     //!< Maps directory
        static constexpr string_view image_path = "/.hamchrono/images/";        //!< Image cache directory
        static constexpr string_view background_path = "/var/lib/hamchrono/backgrounds/";       //!< Backgrounds
        static constexpr pair<string_view, string_view> day_map = {"day_earth_", ".png"};    //!< Day map
        static constexpr pair<string_view, string_view> night_map = {"night_earth_", ".png"};    //!< Night map
        static constexpr string_view backdrop = "NASA_Nebula.png";  //! The current background.

        /**
         * @struct IconRepositoryData
         * Structure to hold initialization data for icons
         */
        struct IconRepositoryData {
            int icon;   //!< the icon code for the ENTYPO icon font.
            int size;   //!< the icon size as a point in font speak.
            array<uint8_t, 4> color;    //!< The requested colour.
        };

        /**
         * @brief Icons are held in an ImageRepository so they may be rendered by multiple widgets. Images in the
         * repository are addressed by a std::pair of integers. In this case the first selects a category, second
         * selects a specific icon.
         */
        // Icons for geographic locations ImageRepository::ImageStoreIndex idx{0, x}
        static constexpr array<IconRepositoryData, 3> mGeoLocationIcons = {
                IconRepositoryData{ENTYPO_ICON_HAIR_CROSS, 50, {0x00, 0xFF, 0x00, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_HAIR_CROSS, 50, {0xFF, 0x00, 0x00, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD, 40, {0x00, 0x00, 0x00, 0xFF}}
        };

        // Icons for celestial objects ImageRepository::ImageStoreIndex idx{1, x}
        static constexpr array<IconRepositoryData, 2> mCelestialIcons = {
                IconRepositoryData{ENTYPO_ICON_LIGHT_UP, 50, {255, 255, 0, 255}},
                IconRepositoryData{ENTYPO_ICON_MOON, 50, {255, 255, 255, 255}}
        };

        // Icons for satellites ImageRepository::ImageStoreIndex idx{2, x}
        static constexpr array<IconRepositoryData, 5> mSatOrbitIcons = {
                IconRepositoryData{ENTYPO_ICON_RECORD, 30, {0x00, 0xFF, 0, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD, 30, {0x80, 0xC0, 0, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD, 30, {0xA0, 0xA0, 0, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD, 30, {0xC0, 0x40, 0, 0xFF}},
                IconRepositoryData{ENTYPO_ICON_RECORD, 30, {0xFF, 0x00, 0, 0xFF}}
        };

        // Links to NASA solar images downloaded for display
        static constexpr array<pair<string_view, string_view>, 5> NasaSolarImages {
                pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0193.jpg", "AIA 193 Å" },
                pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_211193171.jpg", "AIA 211 Å, 193 Å, 171 Å" },
                pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_HMIB.jpg", "HMI Magnetogram" },
                pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_HMIIC.jpg", "HMI Intensitygram" },
                pair<string_view, string_view>{ "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0171.jpg", "AIA 171 Å" }
        };

        HamChrono(SDL_Window *pwindow, int rwidth, int rheight, const string &homedir, const string &callsign,
                  const Observer &observer);

        void buildIconRepository();

        void screenShot();

        static tuple<SDL_Surface *, time_point<std::chrono::system_clock>>
        curlFetchImage(const string &url, const string &homedir, const string &name);

        Uint32 timerCallback(Uint32 interval);

        void initialize();
    };
}