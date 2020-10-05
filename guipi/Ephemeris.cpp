//
// Created by richard on 2020-10-03.
//

#include "Ephemeris.h"

#include <utility>
#include <guipi/p13.h>

using namespace sdlgui;

void guipi::PlotPackage::predict(const guipi::Ephemeris &ephemeris) {
    if (mPlotItemType == CELESTIAL_BODY_MOON || mPlotItemType == EARTH_SATELLITE) {
        DateTime predictionTime;
        predictionTime.userNow();
        if (auto coord = ephemeris.predict(mName, predictionTime)) {
            mGeoCoord = coord.value();
        }

        mMapCoordValid = false;
    }
}

guipi::PlotPackage::PlotPackage(std::string plotName,
                                guipi::PlotItemType itemType)
                : mName(std::move(plotName)), mGeoCoord(), mMapCoord(), mMapCoordValid(false), mPlotItemType(itemType),
                  mDrawSize(Vector2i::Zero()) {
}

guipi::PlotPackage::PlotPackage(std::string name, guipi::PlotItemType itemType,
                                const sdlgui::Vector2f& geoCoord)
                : mName(std::move(name)), mPlotItemType(itemType), mGeoCoord(geoCoord), mMapCoord(), mMapCoordValid(false),
                  mDrawSize(Vector2i::Zero()) {
}

void guipi::PlotPackage::predictPass(const guipi::Ephemeris &ephemeris, const Observer &observer) {
    if (mPlotItemType == CELESTIAL_BODY_MOON || mPlotItemType == EARTH_SATELLITE) {
        mEarthsat = ephemeris.nextPass(mName, observer);
    }
}
