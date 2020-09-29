# HamChrono - HamClock reimagined for the RaspberryPi 

![Mercator Projection](https://raw.githubusercontent.com/pa28/GuiPi/porting/ScreenShots/ScreenshotMercator.png)

I have started this project mainly to bring my
personal desires to the great features available
from [HamClock](https://www.clearskyinstitute.com/ham/HamClock/).
My advice is that you start there, then check out
my design goals to see if this project may provide
some benefit to you. I am not here to criticize HamClock,
only to bring a slightly different vision to packaging
the features.

![Azimuthal Projection](https://raw.githubusercontent.com/pa28/GuiPi/porting/ScreenShots/ScreenshotAzimuthal.png)

[HamClock](https://www.clearskyinstitute.com/ham/HamClock/)
by Elwood Downey WB0OEW is an
[Arduino](https://www.arduino.cc/) and
[Raspberry Pi](https://www.raspberrypi.org/)
project that provides a number of useful features
for amatuer radio operators. It is quite a nice
tool as it is. The main limitation for Raspberry Pi
users *is* that it comes from the Arduino world. 
Getting the number of features running on such a
limited platform is a tour-de-force, and then
porting it to the Pi is another significant
achievement. The Pi is capable of much more than
even the most advanced Arduino. So I became
interested in seeing what I could to to unlock
those features for Pi users.

![Image Repeater](https://raw.githubusercontent.com/pa28/GuiPi/porting/ScreenShots/ScreenshotRepeater.png)

## [Porting to modern C++.](https://github.com/pa28/GuiPi/blob/porting/LanguageFeatures.md)

The following are the things that I sought to do
better on the Raspberry Pi:
* HamClock, like most Arduino programs (including
my own), is based on an endless loop with limited or
no event driven architecture. This is an appropriate
design for Arduino, but on a pre-emptive multi-taksing
operating system it leads to programs that run flat out
without pause consuming CPU cycles, generating heat,
and keeping other programs from using those cycles.
* I run HamClock on a 7 inch touch screen in a
[SmartyPi](https://smarticase.com/) case. This is a great
form factor for convenient deployment in the shack. At
800x400 pixels and 7 inches the screen gets very busy and
using touch becomes a matter of poke and hope you hit the
right spot. HamClock can be made to run on the frame buffer
without a screen manager, which is a nice way to provide
an appliance-like feel. Elwood has written a GUI system for
HamClock that certainly gets the job done for the minimalist
target Arduino system, yet another laudable achievement.
But if we are building specifically for the Raspberry Pi
there are frame buffer GUI libraries we can use.
* Finally I am going to port Elwood's code to modern C++
so that it may be more easily expanded and maintained, at
least when run on a full multitasking OS. If you desire
the minimalist system for portability, low power or other
reasons, then HamClock the way Elwood wrote it is what you
want.

# Dependencies

## Libraries

1. I was going to use [LVGL](https://lvgl.io/)
a "Light and Versitile Graphics Library". The use of this
libbrary on the 7 inch RaspberryPi display is described by
[VK3ERW](http://www.vk3erw.com/index.php/16-software/63-raspberry-pi-official-7-touchscreen-and-littlevgl).
This library needs to be compile for the target. It was my
intention to provide a package for use on the RaspberryPi.
Unfortunately I found the following:
    1. No standardized build environment;
        1. Have to move Makefiles around and put your own together.
        There is flexability here, but this makes geeping up with 
        changes hard.
        1. Shared library not provided for, again you can build it
        yourself...
        1. Does not support common image types or file system out
        of the box.

1. [SDL2](https://www.libsdl.org/) and
[NanoGUI-SDL](https://github.com/dalerank/nanogui-sdl)
    1. This is very nice library, but has an incomplete and uneven feel. The developers 
    have also become busy with other things. However, it is open source and I can fork 
    the repository and begin taking it in the direction I need. My port of NanoGUI-SDL
    is included in the project.

1. [GNU CGICC](https://www.gnu.org/software/cgicc/doc/index.html)
a [Common Gateway Interface](https://en.wikipedia.org/wiki/Common_Gateway_Interface)
library. This will allow my implementation to provide a
[RESTful](https://searchapparchitecture.techtarget.com/definition/RESTful-API)
API (as HamClock does) for remote control. Using the CGI
interface will allow the application to be tied into a web
server running on the RaspberryPi which will offload much of the
configuration, security and maintenance work of a public facing 
access.

1. [SOCI](http://soci.sourceforge.net/) a C++ Database Access Library.

1. [libcurl](https://curl.haxx.se/libcurl/) the multiprotocol transfer library. ```sudo apt-get install libcurl4-openssl-dev```

    1. [curlpp](http://www.curlpp.org/) C++ wrapper around libcURL. ```sudo apt-get install libcurlpp-dev```

1. C++17 compliant standard library.

    1. C++17 or better compiler package (if compiling from source).

1. CMake version 3.10 or better (if compiling from source).

## Data Sources

1. [ClearSkyInstitute.Com](http://clearskyinstitute.com/)
1. [NASA](https://www.nasa.gov/)
    1. [NASA SDO Data](https://sdo.gsfc.nasa.gov/data/)

*73*
Richard Buckley
VE3YSH
