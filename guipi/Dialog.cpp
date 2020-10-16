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

guipi::Dialog::Dialog(Widget *parent, const std::string &title, const Vector2i &position)
        : Window(parent, title, position) {
    if (buttonPanel())
        buttonPanel()->withLayout<BoxLayout>(Orientation::Horizontal, Alignment::Minimum, 0, 0)
                ->withFixedHeight(35)
                ->add<ToolButton>(ENTYPO_ICON_CHECK, Button::Flags::NormalButton)
                ->withCallback([&]() {
                    dispose();
                });
    buttonPanel()->add<ToolButton>(ENTYPO_ICON_SQUARED_CROSS, Button::Flags::NormalButton)
            ->withCallback([&]() {
                dispose();
            });
    setModal(true);
    dynamic_cast<Screen *>(window()->parent())->moveWindowToFront(window());
}

guipi::SettingsDialog::SettingsDialog(Widget *parent, const string &title, const Vector2i &position,
                                      const Vector2i &fixedSize)
        : Dialog(parent, title, position) {
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
    callsign->setFixedSize(Vector2i(100, 40));
    callsign->setValue(mSettings->mCallSign);
    callsign->setFontSize(20);
    callsign->setFormat("[A-Z0-9]{3,6}");

    panel00->add<Label>("Latitude")->withFontSize(20);
    auto latitude = panel00->add<TextBox>();
    latitude->setAlignment(TextBox::Alignment::Left);
    latitude->setEditable(true);
    latitude->setUnits("Deg");
//    latitude->setFixedSize(Vector2i(100, 40));
    latitude->setValue(realToString(mSettings->mLatitude));
    latitude->setFontSize(20);
    latitude->setFormat("[-]?[0-9]{0,2}\\.?[0-9]{1,4}");
    latitude->setMaxLength(8);

    panel00->add<Label>("Longitude")->withFontSize(20);
    auto longitude = panel00->add<TextBox>();
    longitude->setAlignment(TextBox::Alignment::Left);
    longitude->setEditable(true);
    longitude->setUnits("Deg");
//    longitude->setFixedSize(Vector2i(100, 40));
    longitude->setValue(realToString(mSettings->mLongitude));
    longitude->setFontSize(20);
    longitude->setFormat("[+-]?[0-9]{0,3}.?[0-9]{0,4}");
    longitude->setMaxLength(9);

}
