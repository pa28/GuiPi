//
// Created by richard on 2020-09-09.
//

#include <locale>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <sdlgui/common.h>
#include <sdlgui/layout.h>
#include "TimeBox.h"

static Uint32 TimeBoxCallbackStub(Uint32 interval, void *param);

#ifdef BCMHOST
static constexpr std::string_view SystemTempDevice = "/sys/class/thermal/thermal_zone0/temp";
#else
static constexpr std::string_view SystemTempDevice = "/sys/class/thermal/thermal_zone2/temp";
#endif

namespace sdlgui {
    TimeBox::TimeBox(Widget *parent)
            : Widget(parent),
              mTimer(*this, &TimeBox::timerCallback, 1000),
              mIsLocalTime(false),
              mSmallBox(false),
              locale_time_put(use_facet<time_put<char>>(locale())) {
        initialize();
    }

    void TimeBox::renderTime(const std::chrono::time_point<std::chrono::system_clock> &now) {
        using namespace std::chrono;
        auto tt = system_clock::to_time_t(now);
        auto tm = mIsLocalTime ? *localtime(&tt) : *gmtime(&tt);

        stringstream hm;
        hm.imbue(locale());

        put_locale_time(hm, ' ', &tm,
                        mSmallBox ? mTheme->mTimeBoxSmallHoursMinFmt : mTheme->mTimeBoxHoursMinFmt);
        mHoursMins->setCaption(hm.str());

        hm.str("");
        put_locale_time(hm, ' ', &tm,
                        mSmallBox ? mTheme->mTimeBoxSmallSecFmt : mTheme->mTimeBoxSecFmt);
        mSeconds->setCaption(hm.str());

        hm.str("");
        put_locale_time(hm, ' ', &tm,
                        mSmallBox ? mTheme->mTimeBoxSmallDateFmt : mTheme->mTimeBoxDateFmt);
        mDate->setCaption(hm.str());
    }

    Uint32 TimeBoxCallbackStub(Uint32 interval, void *param) {
        interval = static_cast<TimeBox *>(param)->timerCallback(interval);
        return interval;
    }

    Uint32 TimeBox::timerCallback(Uint32 interval) {
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = now - mEpoch;
        auto delta_seconds = elapsed_seconds - mElapsedSeconds;
        mElapsedSeconds = elapsed_seconds;
        renderTime(now);
        if (!mSmallBox)
            readCPUTemperature();

        return 1005 - chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    }

    TimeBox::TimeBox(Widget *parent, bool small, bool localTime)
            : Widget(parent),
              mTimer(*this, &TimeBox::timerCallback, 1000),
              mSmallBox(small),
              mIsLocalTime(localTime),
              locale_time_put(use_facet<time_put<char>>(locale())) {
        initialize();
    }

    void TimeBox::initialize() {
        if (mSmallBox) {
            mTimeBoxHoursMinFontSize = mTheme->mTimeBoxSmallHoursMinFontSize;
            mTimeBoxSecFontSize = mTheme->mTimeBoxSmallSecFontSize;
            mTimeBoxDateFontSize = mTheme->mTimeBoxSmallDateFontSize;
        } else {
            mTimeBoxHoursMinFontSize = mTheme->mTimeBoxHoursMinFontSize;
            mTimeBoxSecFontSize = mTheme->mTimeBoxSecFontSize;
            mTimeBoxDateFontSize = mTheme->mTimeBoxDateFontSize;
        }

        if (mSmallBox) {
            mTimeBoxDateFont = mTheme->mTimeBoxSmallDateFont;
            mTimeBoxTimeFont = mTheme->mTimeBoxSmallTimeFont;
        } else {
            mTimeBoxDateFont = mTheme->mTimeBoxDateFont;
            mTimeBoxTimeFont = mTheme->mTimeBoxTimeFont;
        }

        // Compute an offset which will (hopefully) line hours, mins and secs up by their tops
        // TODO: Label widgets should set their size based on the font size used.
        auto timeSizeDiff = std::abs(mTimeBoxHoursMinFontSize - mTimeBoxSecFontSize) / 2;

        // Construct a time Box
        withLayout<BoxLayout>(Orientation::Vertical,
                              Alignment::Minimum,
                              0, 5);
        mTimeDisplay = add<Widget>();
        mTimeDisplay->withLayout<BoxLayout>(Orientation::Horizontal,
                                            Alignment::Minimum,
                                            0, 5);
        mDateDisplay = add<Widget>();
        mDateDisplay->withLayout<BoxLayout>(Orientation::Horizontal,
                                            Alignment::Minimum,
                                            0, 5);

        if (!mSmallBox)
            mMonitor = add<Widget>()->withLayout<BoxLayout>(Orientation::Horizontal,
                                                            Alignment::Minimum,
                                                            0, 5);

        mHoursMins = mTimeDisplay->add<Label>("")
                ->withFont(mTimeBoxTimeFont)
                ->withFontSize(mTimeBoxHoursMinFontSize)
                ->withFixedHeight(mTimeBoxHoursMinFontSize);

        mSeconds = mTimeDisplay->add<Label>("")->withFont(mTimeBoxTimeFont);
        mSeconds->withFontSize(mTimeBoxSecFontSize)->withFixedHeight(mTimeBoxSecFontSize + timeSizeDiff);

        mDate = mDateDisplay->add<Label>("")->withFont(mTimeBoxDateFont);
        mDate->withFontSize(mTimeBoxDateFontSize)->withFixedHeight(mTimeBoxDateFontSize);

        if (!mSmallBox)
            mTemperature = mMonitor->add<Label>("")->withFont(mTimeBoxDateFont);

        mEpoch = std::chrono::system_clock::now();
        renderTime(mEpoch);
        if (!mSmallBox)
            readCPUTemperature();
    }

    bool TimeBox::mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button,
                                   int modifiers) {
        if (button) {
            auto level = sdlgui::clamp((float) p.x / (float) width(), 0.0F, 1.0F);
            if (mCallback) {
                mCallback(*this, level);
            }
        }
        return Widget::mouseMotionEvent(p, rel, button, modifiers);
    }

    void TimeBox::readCPUTemperature() {
        if (mHasTemperatureDevice) {
            std::ifstream ifs;
            ifs.open(std::string(SystemTempDevice), std::ofstream::in);
            if (ifs) {
                int temperature;
                std::stringstream sstrm;
                ifs >> temperature;
                ifs.close();
                sstrm << "CPU " << roundToInt((float) temperature / 1000.) << 'C';
                mTemperature->setCaption(sstrm.str());
                if (temperature < mTheme->mCPUNormalMax)
                    mTemperature->setColor(mTheme->mCPUNormal);
                else if (temperature < mTheme->mCPUWarningMax)
                    mTemperature->setColor(mTheme->mCPUWarning);
                else
                    mTemperature->setColor(mTheme->mCPUAlert);
            } else {
                // TODO: Better error reporting.
                mHasTemperatureDevice = false;
                std::cerr << "Can not open " << SystemTempDevice << std::endl;
            }
        }
    }

//    bool TimeBox::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
//        if (button) {
//            if (down) {
//                if (!mButton) {
//                    mMotion = false;
//                }
//                mButton = true;
//            } else {
//                if (mCallback) {
//                    auto d = mMotionEnd - mMotionStart;
//                    if (mMotion && d.x * d.x + d.y * d.y > 25) {
//                        if (abs(d.y) >= abs(d.x)) {
//                            if (d.y > 0)
//                                mCallback(*this, DOWN_EVENT);
//                            else
//                                mCallback(*this, UP_EVENT);
//                        } else {
//                            if (d.x > 0)
//                                mCallback(*this, RIGHT_EVENT);
//                            else
//                                mCallback(*this, LEFT_EVENT);
//                        }
//                        mMotionStart = mMotionEnd = Vector2i(0, 0);
//                        mMotion = mButton = false;
//                    } else {
//                        mCallback(*this, CLICK_EVENT);
//                    }
//                }
//            }
//        }
//        return Widget::mouseButtonEvent(p, button, down, modifiers);
//    }

}
