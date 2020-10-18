# HamChrono

## See Also:
1. My [Debian Repository](Repository.md) for information on installing.

1. How to [auto start](AutoStart.md) on a Pi.

This will give you a brief introduction to HamChrono and how to use it.
The primary development target for the program is a Raspberry Pi with
the official 7-inch touch screen running without the desktop software.
In this case the program runs directly on the screen frame buffer and
is controlled by screen touch gestures. When run on a system with the
desktop software installed and running it can be controlled by mouse
gestures, left button clicks for presses, and left button drags for
finger drags.

The overall screen looks the same in both environments, except the desktop
version has a window frame.

![MainScreen](MainScreen.png)

## Callsign Button

Your callsign is displayed as the label of a button which, when pressed,
brings up the configurtion dialog.

![CallsignButton](CallsignButton.png)

The Settings Dialog allows you to set your Callsign, position and elevation.
At the moment this requires a keyboard and mouse even if you are using the 
touch screen interface. There is another way to set this information if
you don't want to connect a keyboard. Simply ssh into the Raspberry Pi
and start the program once setting this information on the command line:

`hamchrono -cs VE3YSH -lat 44.0 -lon -75.0 -el 121.0`

![SettingsDialog](SettingsDialog.png)

## GMT/UTC Time Display

Below the callsign button is the GMT/UTC time display. This also displays
the CPU temperature (Zone 3 on x86 systems) and CPU usage. When using the
official 7-inch Raspberry Pi display, dragging a finger left and right on
this dispplay will dim and brighten the display backlight.

![UTCDisplay](UTCDisplay.png)

## Local Time Display

Below the GMT/UTC time display is the local time display. This displays the
local time according to the system configuration. Not much else to say about
this.

![LocalTime](LocalTime.png)

## Distant Station Display

Finally, down the remainder of the left hand side is the distant station
display. This is a tab display with a "rocket" and a "location" icon on
the two tabs. These tabs hold the satellite prediction display, and the
terrestrial station data respectively. At the moment only the satellite
display is implemented. Pressing the tab icon will select the corresponding
tab.

### Satelite Prediction Data

For each of five satellites there is displayed the tracking icon, the
satellite short name (derived from the ephemeris data) and the pass
prediction data. 

![DXStation](DXStation.png)

## Supporting Data

Along the top, between the Callsign Button and the Switch Matrix is the
supporting data display. At present there is only NASA Solar Imageery.
Dragging a finger either up or down on the image will select the next or
previous image in the list. Pressing a finger on the image will call up
a larger display image. New images are loaded aproximately every hour.

![SolarImageSmall](SolarImageSmall.png)
![SolarImageLarge](SolarImageLarge.png)

## Switch Matrix

The switch matrix at the top right controls most of the action of the 
program. Not all the switches have a function at the moment, and the
icon may change as they aquire functions.

![SwitchMatrix](SwitchMatrix.png)

### Satellite/Terestrial Buttons

![SatTerSwitches](SatTerSwitches.png)

The buttons with the "rocket" and "location" icons, similar to the tabs
on the Distant Station Display select between satellite and terrestrial
station data. They are ganged toggle buttons. Both may be off, but only
one may be on at at time.

#### Satellite

![SatSwitch](SatSwitch.png)

When on satellite orbital position display is active, and automatic pass
tracking display is enabled. When one or more satelittes are passing in
view the display will automatically shift to azimuthal display, and the
right side will be replace by an azimuthe/elevation display of satellites
in view.

#### Terestrial

![TerSwitch](TerSwitch.png)

Not yet implemented. 

#### Celestial Objects

![CelSwitch](CelSwitch.png)

This toggle button controls the display of the position of celestial objects:
the Sun and the Moon.

#### Mercater/Azimuth Projection

![AzSwithc](AzSwitch.png)

This toggle button controls the displayed projection of the map: Mercator or 
Azmuthal; when not configured automatically for pass prediction display.

#### Screen Shot

![ScreenShot](ScreenShot.png)

The Screen Shot button will cause the program to save an image of the current
display to the current working directory `screenshot.bmp`.

#### More Controls

![MoreControls](MoreControls.png)

This button will bring up a dialog with more, less frequently used controls.
At present the only control is a set of radio buttons that allow selection
of ephemeris from several on-line sources.

![MoreControlsDialog](MoreControlsDialog.png)