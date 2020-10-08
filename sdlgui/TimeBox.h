//
// Created by richard on 2020-09-09.
//

#ifndef SDLGUI_TIMEBOX_H
#define SDLGUI_TIMEBOX_H

#include <iomanip>
#include <string_view>
#include <sdlgui/common.h>
#include <sdlgui/label.h>
#include <sdlgui/layout.h>
#include <sdlgui/widget.h>

namespace sdlgui {
    using namespace std;
    using namespace std::chrono;

    /**
     * @class Timer
     * @tparam T the class of the timer client
     *
     * A helper class to provide an interval timer callback to a client class.
     * See SDL_AddTimer
     *
     */
    template <class T> class Timer {
    public:
        typedef Uint32 (T::*ClientCallback)(Uint32);

    private:
        T& mClient; //* the client object
        ClientCallback mCallback;   //* the client member function to call back
        SDL_TimerID sdlTimerId;

        /**
         * The static function passed to SDL_AddTimer as the callback
         * @param interval the current interval
         * @param param the pointer to this Timer
         * @return the next interval or 0 to stop the timer
         */
        static Uint32 TimerCallbackStub(Uint32 interval, void *param) {
            auto timer = static_cast<Timer*>(param);
            return (timer->mClient.*timer->mCallback)(interval);
        }

    public:

        Timer() = delete;
        Timer(const Timer &) = delete;
        Timer(Timer &) = delete;

        /**
         * Crate the Timer
         * @param client the client object
         * @param callback the client objects member callback method
         * @param interval the requested interval
         */
        Timer(T& client, ClientCallback callback, Uint32 interval) : mClient(client), mCallback(callback) {
            sdlTimerId = SDL_AddTimer(interval, Timer::TimerCallbackStub, this);
        }

        /**
         * Remove the SDLTimer when destroyed.
         */
        ~Timer() {
            SDL_RemoveTimer(sdlTimerId);
        }
    };

    /**
     * class TimeBox
     * A widget to display the current date and time.
     */

    class TimeBox : public Widget {
    private:
#if __cplusplus == 201703L
        static constexpr array<string_view, 7> mDays = {
                "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        static constexpr array<string_view, 12> mMonths = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
#else
        static constexpr array<const char*, 7> mDays {
                "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
        };
        static constexpr array<const char*, 12> mMonths = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
#endif

        Timer<TimeBox> mTimer;
        const time_put<char> &locale_time_put;
        time_point <system_clock> mEpoch;
        duration<double> mElapsedSeconds{};
        bool mIsLocalTime{false};
        bool mSmallBox{false};
        bool mConfigured{false};
        bool mFontSet{false};
        bool mFontSizeSet{false};
        ref<Widget> mTimeDisplay;
        ref<Widget> mDateDisplay;
        ref<Label> mHoursMins;
        ref<Label> mSeconds;
        ref<Label> mDate;

        int mTimeBoxHoursMinFontSize{};
        int mTimeBoxSecFontSize{};
        int mTimeBoxDateFontSize{};

        std::string mTimeBoxTimeFont;
        std::string mTimeBoxDateFont;

        bool mMotion{false}, mButton{false};
        Vector2i mMotionStart, mMotionEnd;

        std::function<void(TimeBox &, float )> mCallback;

        /**
         * Widget default constructor.
         */

    public:

        TimeBox() = delete;

        explicit TimeBox(Widget *parent);

        ~TimeBox() override = default;

        void initialize();

        /**
         * Constructor
         * @param parent the parent Widget
         * @param font the font to use or an empty string for the default
         * @param fontSize the font size to use or -1 for the default
         */
        TimeBox(Widget *parent, bool small, bool localTime = false);

        /**
         * Render time for a specific time.
         * @param now the time to be rendered.
         */
        void renderTime(const time_point <system_clock> &now);

        /**
         * The timer callback
         * @param interval
         * @return the new interval
         */
        Uint32 timerCallback(Uint32 interval);

        /**
         * A helper function to convert the system time to a localized time
         * @param strm The stream to output time, that is imbued with the locale
         * @param fill The fill char
         * @param tm The std::tm struct with the desired time.
         * @param fmt A format string (see std::locale_time_put::put()
         * @return an iterator one past the last char generated.
         */
        auto put_locale_time( stringstream& strm, char fill, const std::tm *tm , const std::string &fmt) {
            if (fmt.empty()) {
                throw std::runtime_error("Format must not be empty");
            } else {
                const char *fmtbeg = fmt.c_str();
                const char *fmtend = fmtbeg + fmt.size();
                return locale_time_put.put(strm, strm, fill, tm, fmtbeg, fmtend);
            }
        }

        /**
         * Get the currently set callback function.
         * @return the callback function.
         */
        [[nodiscard]] std::function<void(TimeBox &, float)> callback() const { return mCallback; }

        /**
         * Set the call back function
         * @param callback the desired callback function, a simple but usefule lambda is:
         * [](TimeBox &w, float value) {
                            // Do someting with value
                        }
         * @return a reference to this ImageDisplay
         */
        TimeBox *setCallback(const std::function<void(TimeBox &, float)> &callback) {
            mCallback = callback;
            return this;
        }

        bool mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;

//        bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) override;
    };
}

#endif //SDLGUI_TIMEBOX_H
