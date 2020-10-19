//
// Created by richard on 2020-10-15.
//

#include "Dialog.h"
#include <guipi/hamchrono.h>
#include <sdlgui/checkbox.h>
#include <sdlgui/screen.h>
#include <sdlgui/label.h>
#include <sdlgui/slider.h>
#include <sdlgui/textbox.h>
#include <sdlgui/toolbutton.h>
#include <sdlgui/tabwidget.h>
#include <sdlgui/entypo.h>
#include <guipi/GuiPiApplication.h>

using namespace sdlgui;
using namespace std;

guipi::Dialog::Dialog(Widget *parent, Widget *trigger, const std::string &title, const Vector2i &position,
                      bool buttonBar)
        : Window(parent, title, position), mTrigger(trigger) {
    if (mTrigger)
        mTrigger->setEnabled(false);
    if (buttonPanel() && buttonBar)
        buttonPanel()->add<ToolButton>(ENTYPO_ICON_SQUARED_CROSS, Button::Flags::NormalButton)
                ->withCallback([&]() {
                    disposeDialog();
                });
    setModal(true);
    dynamic_cast<Screen *>(window()->parent())->moveWindowToFront(window());
}

void guipi::Dialog::disposeDialog() {
    if (mTrigger) {
        mTrigger->requestFocus();
        mTrigger->setEnabled(true);
    }
    dispose();
}

guipi::SettingsDialog::SettingsDialog(Widget *parent, Widget *trigger, const std::string &title,
                                      const Vector2i &position)
        : Dialog(parent, trigger, "Settings Ver " XSTR(VERSION), position, true) {
    initialize();
    dialogPerformLayout();
}

#define REAL_TEXT_BOX_SET(parent, value, units, length, precision, format, range)  \
{                                                               \
    parent->add<Label>(# value)->withFontSize(20);              \
    auto widget = parent->add<TextBox>();                       \
    widget->setAlignment(TextBox::Alignment::Left);             \
    widget->setEditable(true);                                  \
    widget->setUnits(# units);                                  \
    widget->setValue(realToString(mSettings->m ## value, precision));    \
    widget->setFontSize(20);                                    \
    widget->setFormat(format);                                  \
    widget->setMaxLength(length);                               \
    widget->setFixedWidth(150);                                 \
    widget->setCallback([this](const std::string &text) -> bool {                  \
        auto v = std::strtof(text.c_str(), nullptr);            \
        if (abs(v) <= range)                                \
            mSettings->set ## value (v);                        \
        return true;                                            \
    });                                                         \
}


void guipi::SettingsDialog::initialize() {

    withLayout<BoxLayout>(Orientation::Horizontal,
                          Alignment::Minimum,
                          5, 5);

    auto panel0 = add<Widget>();
    panel0->withLayout<BoxLayout>(Orientation::Vertical,
                                  Alignment::Minimum,
                                  0, 0);

    auto panel00 = panel0->add<Widget>();
    panel00->withLayout<GridLayout>(Orientation::Horizontal, 2,
                                    Alignment::Middle,
                                    5, 5);

    panel00->add<Label>("Callsign")->withFontSize(20);
    auto callsign = panel00->add<TextBox>();
    callsign->setAlignment(TextBox::Alignment::Left);
    callsign->setEditable(true);
    callsign->setValue(mSettings->mCallSign);
    callsign->setFontSize(20);
    callsign->setFormat("[A-Z0-9]{3,8}");
    callsign->setMaxLength(8);
    callsign->setFixedWidth(150);
    callsign->setCallback([this](const std::string &text) -> bool {
        mSettings->setCallSign(text);
        return true;
    });

    REAL_TEXT_BOX_SET(panel00, Latitude, Deg, 8, 4, "[-]?[0-9]{0,2}\\.?[0-9]{1,4}", 90.)
    REAL_TEXT_BOX_SET(panel00, Longitude, Deg, 9, 4, "[-]?[0-9]{0,3}\\.?[0-9]{1,4}", 180.)
    REAL_TEXT_BOX_SET(panel00, Elevation, m, 5, 1, "[+-]?[0-9]{0,4}\\.?[0-9]{0,1}", 4000.)

    auto panel1 = add<Widget>();
    panel1->withLayout<GroupLayout>(10);

    panel1->add<Label>("System Management")
            ->withFontSize(20);
    auto panel10 = panel1->add<Widget>()
            ->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Middle, 5, 5);
    mStopButton = panel10->add<ToolButton>(ENTYPO_ICON_LOGOUT, Button::Flags::NormalButton)
            ->withCallback([this]() {
                systemButtonCallback(mStopButton, SystemCmd::STOP);
            });
    mRebootButton = panel10->add<ToolButton>(ENTYPO_ICON_CW, Button::Flags::NormalButton)
            ->withCallback([this]() {
                systemButtonCallback(mRebootButton, SystemCmd::REBOOT);
            });
    mUpgradeButton = panel10->add<ToolButton>(ENTYPO_ICON_INSTALL, Button::Flags::NormalButton)
            ->withCallback([this]() {
                systemButtonCallback(mUpgradeButton, SystemCmd::UPGRADE);
            });
    mHaltButton = panel10->add<ToolButton>(ENTYPO_ICON_STOP, Button::Flags::NormalButton)
            ->withCallback([this]() {
                systemButtonCallback(mHaltButton, SystemCmd::HALT);
            })
            ->withTextColor(Color{255, 0, 0, 255});
}

void
guipi::SettingsDialog::systemButtonCallback(sdlgui::ref<ToolButton> &button, SystemCmd cmd) {
    auto p = parent();
    switch (cmd) {
        case SystemCmd::STOP:
            screen()->add<ResponseDialog>(button.get(), Question, "Exit?",
                                          "Exit the program?", "Yes", "No")
                    ->withCallback([&](ResponseButton button) {
                        if (button == ResponseButton::StandardButton)
                            dynamic_cast<guipi::GuiPiApplication *>(screen())->exitApplication();
                    });
            break;
        case SystemCmd::REBOOT:
            screen()->add<ResponseDialog>(button.get(), Question, "Reboot?",
                                          "Reboot the system?", "Yes", "No")
                    ->withCallback([&](ResponseButton button) {
                        if (button == ResponseButton::StandardButton)
                            auto res = system("sudo reboot");
                    });
            break;
        case SystemCmd::UPGRADE:
            screen()->add<ResponseDialog>(button.get(), Question, "Upgrade?",
                                          "Upgrade system software?", "Yes", "No")
                    ->withCallback([&](ResponseButton button) {
                        if (button == ResponseButton::StandardButton)
                            auto res = system("sudo bash -c 'apt update && apt upgrade --assume-yes'");
                    });
            break;
        case SystemCmd::HALT:
            screen()->add<ResponseDialog>(button.get(), Question, "Halt?",
                                          "Shut down the system?", "Yes", "No")
                    ->withCallback([&](ResponseButton button) {
                        if (button == ResponseButton::StandardButton)
                            auto res = system("sudo halt");
                    });
            break;
        default:
            break;
    }
}

guipi::ControlsDialog::ControlsDialog(Widget *parent, Widget *trigger, const std::string &title,
                                      const Vector2i &position)
        : Dialog(parent, trigger, title, position, true) {
    initialize();
    dialogPerformLayout();
}

void guipi::ControlsDialog::initialize() {
    withLayout<BoxLayout>(Orientation::Horizontal,
                          Alignment::Minimum,
                          10, 10);

    auto panel1 = add<Widget>();
    panel1->withLayout<GroupLayout>(8);
    panel1->add<Label>("Ephemeris Source")
            ->withFontSize(20);
    ephemerisSelectButton(panel1, "Clear Sky Institude", 0);
    ephemerisSelectButton(panel1, "CelesTrack - Amateur Radio", 1);
    ephemerisSelectButton(panel1, "CelesTrack - Brightest", 2);
    ephemerisSelectButton(panel1, "CelesTrack - Cubesats", 3);

    auto elevationToString = [](float elevation) {
        return string("Min Elev. ") + to_string(roundToInt(elevation, 0.2f));
    };

    auto panel2 = add<Widget>()->withLayout<GroupLayout>(8);
    panel2->add<Label>("Satellite Pass Filters")->withFontSize(20);
    mButton = panel2->add<Button>("Select Satellites", ENTYPO_ICON_ROCKET)
            ->withCallback([&]() {
                screen()->add<SatelliteSelector>(mButton.get(), "Satellites of Interest");
            })->withId("select-satellites");
    panel2->add<TextBox>(elevationToString(mSettings->getPassMinElevation()), "Deg")
            ->withAlignment(TextBox::Alignment::Left)
            ->withId("min-pass-elevation");
    panel2->add<Slider>(mSettings->getPassMinElevation() / 90.f,
                        [&](Slider *s, float v) {
                            if (auto textBox = dynamic_cast<TextBox *>(s->parent()->find("min-pass-elevation"));
                                    textBox != nullptr) {
                                textBox->setValue(elevationToString(v * 90.f));
                            }
                        },
                        [&](float v) {
                            mSettings->setPassMinElevation(roundToFloat(v * 90.f, 0.2f));
                        });
}

void guipi::ControlsDialog::ephemerisSelectButton(sdlgui::ref<Widget> &parent, std::string_view label, int value) {
    parent->add<Button>(std::string{label})
            ->withFlags(Button::RadioButton)
            ->withPushed(mSettings->mEphemerisSource == value)
            ->withCallback([this, value]() {
                mSettings->setEphemerisSource(value);
            })
            ->withFontSize(15);
}

guipi::ResponseDialog::ResponseDialog(Widget *parent, Widget *trigger, ResponseType responseType,
                                      const std::string &title, const string &message, const string &buttonText,
                                      const string &altButtonText)
        : Dialog(parent, trigger, title, Vector2i{40, 40}, false) {
    setModal(true);
    withLayout<BoxLayout>(Orientation::Vertical,
                          Alignment::Middle,
                          10, 10);

    int icon = 0;
    switch (responseType) {
        case ResponseType::Information:
            icon = ENTYPO_ICON_CIRCLED_INFO;
            break;
        case ResponseType::Question:
            icon = ENTYPO_ICON_CIRCLED_HELP;
            break;
        case ResponseType::Warning:
            icon = ENTYPO_ICON_WARNING;
            break;
    }

    auto panel1 = add<Widget>()->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Middle, 5, 5);
    panel1->add<Label>(std::string(utf8(icon).data()), "icons")->withFontSize(50);
    panel1->add<Label>(message);

    auto panel2 = add<Widget>()->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Middle, 0, 15);
    if (!altButtonText.empty()) {
        panel2->add<Button>(altButtonText, ENTYPO_ICON_CIRCLED_CROSS)
                ->withCallback([&]() {
                    if (mCallback) mCallback(AlternateButton);
                    disposeDialog();
                });
    }

    panel2->add<Button>(buttonText, ENTYPO_ICON_CHECK)
            ->withCallback([&]() {
                if (mCallback) mCallback(StandardButton);
                disposeDialog();
            });

    dialogPerformLayout();
    center();
}

void guipi::Dialog::dialogPerformLayout() {
    requestFocus();
    screen()->performLayout();
}

guipi::SatelliteSelector::SatelliteSelector(Widget *parent, Widget *trigger, const std::string title)
        : Dialog(parent, trigger, title, Vector2i::Zero()) {
//    setFixedSize(Vector2i{DISPLAY_WIDTH - 20,DISPLAY_HEIGHT - 30});
//    auto base = add<Widget>()
    withFixedHeight(DISPLAY_HEIGHT - 35);
    withLayout<BoxLayout>(Orientation::Vertical, Alignment::Minimum, 10, 10);
    auto tab = add<TabWidget>();
    tab->withPosition(Vector2i{10, 40});

    auto app = dynamic_cast<HamChrono *>(screen());
    auto satMap = app->mEphemerisModel.getSatelliteEphemerisMap();
    auto passData = app->mEphemerisModel.getPassMonitorData();

    DateTime now{true};
    auto sat = satMap.begin();
    while (sat != satMap.end()) {
        Widget *layer = tab->createTab(sat->first + "...", 0);
        layer->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 10, 10);

        for (size_t panel0 = 0; panel0 < 4 && sat != satMap.end(); ++panel0) {
            auto p = layer->add<Widget>()->withLayout<GridLayout>(Orientation::Horizontal, 1, Alignment::Minimum, 0, 5);
            auto grid = dynamic_cast<GridLayout *>(p->layout().get());
            grid->setSpacing(0, 10);
            grid->setSpacing(1, 5);
            for (size_t panel1 = 0; panel1 < 15 && sat != satMap.end(); ++panel1, ++sat) {
                p->add<CheckBox>(sat->first)->withFontSize(15);
            }
        }
    }

    tab->setActiveTab(0);
    setFixedHeight(DISPLAY_HEIGHT);
    dialogPerformLayout();
}
