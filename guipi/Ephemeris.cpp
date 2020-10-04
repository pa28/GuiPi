//
// Created by richard on 2020-10-03.
//

#include "Ephemeris.h"

guipi::PlotPackage::PlotPackage(guipi::PlotPackage &&other) noexcept
    : plotItemType{other.plotItemType}, name{other.name}, imageData(move(other.imageData)), geoCoordinates(other.geoCoordinates),
    coastingCoords(move(other.coastingCoords)), coastingInterval(other.coastingInterval),
    mapCoordinates(other.mapCoordinates), mapCoordinatesValid(other.mapCoordinatesValid) {}

guipi::PlotPackage& guipi::PlotPackage::operator=(guipi::PlotPackage &&other)  noexcept {
    plotItemType = other.plotItemType;
    name = other.name;
    imageData = move(other.imageData);
    geoCoordinates = other.geoCoordinates;
    coastingCoords = move(other.coastingCoords);
    coastingInterval = other.coastingInterval;
    mapCoordinates = other.mapCoordinates;
    mapCoordinatesValid = other.mapCoordinatesValid;
    return *this;
}

guipi::PlotPackage::PlotPackage(const Ephemeris &ephemeris, const std::string &plotName, sdlgui::ImageData iconImageData,
                                PlotItemType itemType, uint32_t coastInterval, size_t coastCount) : guipi::PlotPackage() {
    plotItemType = itemType;
    imageData = move(iconImageData);
    name = plotName;
    coastInterval = coastInterval;
    predict(ephemeris, coastCount);
}

bool guipi::PlotPackage::predict(const Ephemeris &ephemeris, size_t coastingCount) {
    DateTime predictionTime;
    predictionTime.userNow();
    if ( auto coord = ephemeris.predict(name, predictionTime))
        geoCoordinates = coord.value();
    else
        return false;

    for( int i = 0; i < coastingCount; ++i ) {
        predictionTime += coastingInterval;
        if ( auto coord = ephemeris.predict(name, predictionTime))
            coastingCoords.push_back(coord.value());
        else
            break;
    }

    mapCoordinatesValid = false;
    return true;
}

bool guipi::PlotPackage::rePredict(const guipi::Ephemeris &ephemeris) {
    DateTime predictionTime;
    predictionTime.userNow();
    if ( auto coord = ephemeris.predict(name, predictionTime))
        geoCoordinates = coord.value();

    for( auto coastCoord : coastingCoords ) {
        predictionTime += coastingInterval;
        if ( auto coord = ephemeris.predict(name, predictionTime))
            coastCoord = coord.value();
        else
            break;
    }

    mapCoordinatesValid = false;
    return true;
}

guipi::PlotPackage::PlotPackage(const std::string &name, sdlgui::ImageData imageData, guipi::PlotItemType itemType,
                                sdlgui::Vector2f geoCoord)
                        : name(name), imageData(move(imageData)), plotItemType(itemType), geoCoordinates(geoCoord) {

}
