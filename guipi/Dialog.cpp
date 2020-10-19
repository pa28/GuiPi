//
// Created by richard on 2020-10-15.
//

#include "Dialog.h"
#include <sdlgui/screen.h>
#include <sdlgui/label.h>
#include <sdlgui/textbox.h>
#include <sdlgui/toolbutton.h>
#include <sdlgui/entypo.h>

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
                          10, 10);

    auto panel0 = add<Widget>();
    panel0->withLayout<BoxLayout>(Orientation::Vertical,
                                  Alignment::Minimum,
                                  0, 10);

    auto panel00 = panel0->add<Widget>();
    panel00->withLayout<GridLayout>(Orientation::Horizontal, 2,
                                    Alignment::Middle,
                                    0, 10);

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

    REAL_TEXT_BOX_SET(panel00,Latitude,Deg, 8, 4, "[-]?[0-9]{0,2}\\.?[0-9]{1,4}",90.)
    REAL_TEXT_BOX_SET(panel00,Longitude,Deg, 9, 4, "[-]?[0-9]{0,3}\\.?[0-9]{1,4}",180.)
    REAL_TEXT_BOX_SET(panel00,Elevation,m, 5, 1, "[+-]?[0-9]{0,4}\\.?[0-9]{0,1}",4000.)

    auto panel1 = add<Widget>();
    panel1->withLayout<GroupLayout>(10);

    panel1->add<Label>("System Management")
            ->withFontSize(20);
    auto panel10 = panel1->add<Widget>()
                    ->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Middle, 5, 5);
    mStopButton = panel10->add<ToolButton>(ENTYPO_ICON_LOGOUT, Button::Flags::ToggleButton);
    mRebootButton = panel10->add<ToolButton>(ENTYPO_ICON_CW, Button::Flags::ToggleButton);
    mUpgradeButton = panel10->add<ToolButton>(ENTYPO_ICON_INSTALL, Button::Flags::ToggleButton);
    mHaltButton = panel10->add<ToolButton>(ENTYPO_ICON_STOP, Button::Flags::ToggleButton)
            ->withChangeCallback([this](bool state) {
                if (state) systemButtonCallback(mHaltButton, SystemCmd::HALT);
            })
            ->withTextColor(Color{255, 0, 0, 255});
}

void
guipi::SettingsDialog::systemButtonCallback(sdlgui::ref<ToolButton> &button, SystemCmd cmd) {
    switch (cmd) {
        case SystemCmd::HALT:
            button->screen()->add<ResponseDialog>(button.get(), Question, "Halt?",
                          "Shut down the system?", "Yes", "No");
        default:
            break;
    }
}

guipi::ControlsDialog::ControlsDialog(Widget *parent, Widget *trigger, const std::string &title,
                                      const Vector2i &position)
          : Dialog(parent, trigger, title, position, true) {
    initialize();
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
}

void guipi::ControlsDialog::ephemerisSelectButton(sdlgui::ref<Widget> &parent, std::string_view label, int value) {
    parent->add<Button>(std::string{label})
            ->withFlags(Button::RadioButton)
            ->withPushed(mSettings->mEphemerisSource == value)
            ->withCallback([this,value](){
                mSettings->setEphemerisSource(value);
            })
            ->withFontSize(15);
}

guipi::ResponseDialog::ResponseDialog(Widget *parent, Widget *trigger, ResponseType responseType,
                                      const std::string &title, const string &message, const string &buttonText,
                                      const string &altButtonText)
    : Dialog(parent, trigger, title, Vector2i::Zero(), false) {
    setLayout(new BoxLayout(Orientation::Vertical,
                            Alignment::Middle, 10, 10));
    auto panel1 = add<Widget>()
            ->withLayout<BoxLayout>(Orientation::Horizontal,
                                    Alignment::Middle, 10, 15);
    int icon = 0;
    switch (responseType)
    {
        case Information: icon = ENTYPO_ICON_CIRCLED_INFO; break;
        case Question: icon = ENTYPO_ICON_CIRCLED_HELP; break;
        case Warning: icon = ENTYPO_ICON_WARNING; break;
    }

    auto iconLabel = panel1->add<Label>(std::string(utf8(icon).data()), "icons")
            ->withIconFontSize(50)
            ->withFontSize(50);

    panel1->add<Label>(message);

    auto panel2 = add<Widget>()
            ->withLayout<BoxLayout>(Orientation::Horizontal,
                                    Alignment::Middle, 0, 15);

    if (!altButtonText.empty())
    {
        panel2->add<Button>(altButtonText, ENTYPO_ICON_CIRCLED_CROSS)
                ->withCallback([&] { if (mCallback) mCallback(1); disposeDialog(); });
    }

    panel2->add<Button>(buttonText, ENTYPO_ICON_CHECK)
            ->withCallback([&] { if (mCallback) mCallback(0); disposeDialog(); });
    center();
}
