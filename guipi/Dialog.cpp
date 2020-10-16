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
                        ->withCallback([&](){
                            dispose();
                        });
        buttonPanel()->add<ToolButton>(ENTYPO_ICON_SQUARED_CROSS, Button::Flags::NormalButton)
                ->withCallback([&]() {
                        dispose();
                });
    setModal(true);
    dynamic_cast<Screen*>(window()->parent())->moveWindowToFront(window());
}

guipi::SettingsDialog::SettingsDialog(Widget *parent, const string &title, const Vector2i &position, const Vector2i &fixedSize)
        : Dialog(parent, title, position) {
    withLayout<BoxLayout>(Orientation::Vertical,
                          Alignment::Minimum,
                          10, 10);

    auto panel0 = add<Widget>();
    panel0->withLayout<BoxLayout>(Orientation::Vertical,
                                  Alignment::Minimum,
                                  0,10);

    auto panel00 = add<Widget>();
    panel00->withLayout<BoxLayout>(Orientation::Horizontal,
                                   Alignment::Middle,
                                   0, 10);

    panel00->add<Label>("Callsign")->withFontSize(20);
    auto textBox = panel00->add<TextBox>();
    textBox->setAlignment(TextBox::Alignment::Left);
    textBox->setEditable(true);
    textBox->setFixedSize(Vector2i(100, 40));
    textBox->setValue("VE3YSH");
    textBox->setFontSize(20);
    textBox->setFormat("[A-Z0-9]{3,6}");

}
