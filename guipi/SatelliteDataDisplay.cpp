//
// Created by richard on 2020-10-06.
//

#include "SatelliteDataDisplay.h"
#include <sstream>
#include <iomanip>
#include <sdlgui/layout.h>
#include <sdlgui/screen.h>

using namespace sdlgui;

guipi::SatelliteDataDisplay::SatelliteDataDisplay(Widget *parent)
        : Widget(parent),
          mTimer(*this, &SatelliteDataDisplay::timerCallback, 1000) {

}

guipi::SatelliteDataDisplay::SatelliteDataDisplay(Widget *parent, ref<ImageRepository> imageRepo,
                                                  ImageRepository::ImageStoreIndex baseIdx)
        : Widget(parent), mImageRepository(move(imageRepo)), mBaseIdx(move(baseIdx)),
          mTimer(*this, &SatelliteDataDisplay::timerCallback, 1000) {
    withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 0, 0);
    for (size_t idx = 0; idx < mImageRepository->size(mBaseIdx.first); ++idx) {
        add<Satellite>(mImageRepository, ImageRepository::ImageStoreIndex{mBaseIdx.first, idx});
    }
}

void guipi::SatelliteDataDisplay::updateSatelliteData(const EphemerisModel::PassMonitorData &passMonitorData) {
    auto child = mChildren.begin();

    for (auto &child : mChildren) {
        auto sat = dynamic_cast<Satellite*>(child);
        sat->mName->setCaption("");
        sat->mInfo->setCaption("");
        sat->mActive = false;
    }
    for (auto &pass : passMonitorData) {
        if (child != mChildren.end()) {
            auto sat = dynamic_cast<Satellite *>(*child);
            sat->update(pass);
            ++child;
        } else
            break;
    }

    performLayout(screen()->renderer());
}

Uint32 guipi::SatelliteDataDisplay::timerCallback(Uint32 interval) {
    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = now - mEpoch;
    auto delta_seconds = elapsed_seconds - mElapsedSeconds;
    if (delta_seconds.count() > 0.9) {
        mElapsedSeconds = elapsed_seconds;
        for (auto &child : mChildren) {
            auto sat = dynamic_cast<Satellite *>(child);
            if (sat)
                sat->update();
        }
    }

    if (delta_seconds.count() > 1.0)
        fmod(delta_seconds.count(), 1.0) * 1000.0;
    else if (delta_seconds.count() < 0.990)
        return 1000.0 - (delta_seconds.count() * 1000.0);

    return 1000;
}

guipi::SatelliteDataDisplay::Satellite::Satellite(Widget *parent, const ref<ImageRepository> &imageRepository,
                                                  ImageRepository::ImageStoreIndex index)
        : Widget(parent), mImageIndex(move(index)), mImageRepository(imageRepository) {
    withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 5, 0);
    auto line0 = add<Widget>()->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 0, 0);
    mIcon = line0->add<Widget>()->withFixedSize(Vector2i(20, 20));
    auto line1 = add<Widget>()->withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 0, 0);
    mName = line1->add<Label>("");
    mInfo = line1->add<Label>("")->withFontSize(12);
}

void guipi::SatelliteDataDisplay::Satellite::update(const EphemerisModel::PassData &passData) {
    mName->setCaption(std::get<0>(passData));
    mActive = true;
    riseTime = std::get<1>(passData);
    setTime = std::get<2>(passData);
    update();
}

void guipi::SatelliteDataDisplay::Satellite::update() {
    if (mActive) {
        DateTime now{true};
        stringstream info;
        auto dt = riseTime - now;
        if (dt < 0)
            info << timeToString(setTime, now);
        else
            info << timeToString(riseTime, now) << " - "
                 << timeToString(setTime, riseTime);
        mInfo->setCaption(info.str());
    } else
        mInfo->setCaption("");
}

string guipi::SatelliteDataDisplay::Satellite::timeToString(const DateTime &time, const DateTime &now) {
    stringstream str;
    auto diff = time - now;
    char sep = 'h';
    if (diff >= 0) {
        diff *= 24;
        uint a = diff;
        if (a == 0) {
            sep = ':';
            diff *= 60;
            a = diff;
        }
        uint b = (diff - a) * 60;
        str << setw(2) << setfill('0') << a << sep << setw(2) << setfill('0') << b;
    }

    return str.str();
}

void guipi::SatelliteDataDisplay::Satellite::draw(SDL_Renderer *renderer) {
    Widget::draw(renderer);
    auto size = mImageRepository->imageSize(renderer, mImageIndex);
    SDL_Rect src{0, 0, size.x, size.y};
    SDL_Rect paint{mIcon->getAbsoluteLeft(), mIcon->getAbsoluteTop(), size.x, size.y};
    mImageRepository->renderCopy(renderer, mImageIndex, src, paint);
}
