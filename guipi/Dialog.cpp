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
    mTrigger->setEnabled(true);
    dispose();
}

guipi::SettingsDialog::SettingsDialog(Widget *parent, Widget *trigger, const std::string &title,
                                      const Vector2i &position,
                                      const Vector2i &fixedSize)
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

    panel00->add<Label>("Version ")->withFontSize(15);
    panel00->add<Label>(XSTR(VERSION))->withFontSize(15);
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
}

guipi::ControlsDialog::ControlsDialog(Widget *parent, const string &title, const Vector2i &position,
                                      const Vector2i &fixedSize) : Dialog(parent, title, position) {
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
