# HamChrono - HamClock reimagined for the RaspberryPi 

![Mercator Projection](https://github.com/pa28/GuiPi/blob/main/ScreenShots/ScreenshotMercator.png)

I have started this project mainly to bring my
personal desires to the great features available
from [HamClock](https://www.clearskyinstitute.com/ham/HamClock/).
My advice is that you start there, then check out
my design goals to see if this project may provide
some benefit to you. I am not here to criticize HamClock,
only to bring a slightly different vision to packaging
the features.

![Azimuthal Projection](https://github.com/pa28/GuiPi/blob/main/ScreenShots/ScreenshotAzimuthal.png)

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

![Image Repeater](https://github.com/pa28/GuiPi/blob/main/ScreenShots/ScreenshotRepeater.png)

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

![TrackingDisplay](https://github.com/pa28/GuiPi/blob/main/ScreenShots/ScreenshotTracking.png)

This started off as a simple exercise to port HamClock to
using the full capabilities available on a Raspberry Pi;
but it has quickly become an parallel effort of that, and
porting and extending NanoGui-SDL to work well, not just on
a Raspberry Pi, but on the 7 inch touch display from
Raspberry Pi. I will document the porting progress of each
below.

### Porting NanoGui-SDL

My work here is centering around making the library more
targeted towards using in a 7 inch touch screen environment.

1. **Supporting the Touch Screen** I have provided basic
support by transcoding finger touch, release and motion events
to equivalent mouse events. This works, but there may be
better solutions to investigate.

1. **GUI Presentation** In a touch environment, the active
elements must be sized large enough and separated well 
enough to be easy to activate by a finger without a frustrating
number of mistakes. NanoGui-SDL has a built in Theme system
that can be used to set different program wide default values.
There are also a large number of hard code values. As I identify
the purpose of each I am replacing them with a Theme value.

1. **Font Support** The library supports True Type Fonts (TTF),
but comes with three fonts bundled which are loaded in an inflexible
way. I am migrating this system to one capable of using any True
Type font loaded onto the system. This will be easier when
Raspberry Pi OS fully supports C++17 with `filesystem`.

1. **Smart Pointers** NanoGui-SDL comes with a very nice reference
counting shared pointer system wich is much better suited to a
hierarchical object model than `std::shared_ptr`. Unfortunately
it is not used as widely or consistently as I'd like; so I am
extending its use where it is well suited and using `std::unique_ptr`
and `std::shared_ptr` in other areas.

1. **Memory Leaks** The library isn't really leaky, but it does have
the rather unfortunate structure that many C programmers employ:
the program exit will free all remaining allocated memory. This is
true, but it limits the ability of code quality tools, like Valgrind,
from easily finding more important leaks. Even a small, on-going 
memory leak can be catastrophic to a program intended to run for very
long periods of time. So I'm cleaning up everything I can track down.

### Porting HamClock

1. **Satellite Ephemeris** Loading ephemeris from the back end is ported.
Computing satellite position and predicting passes is also ported.

1. **Geo Chron Display** This has also been ported using Mercator projection
maps loaded from image files on the file system rather than compiling map
data into the binary. This latter was a very creative solution to support
HamClock on an ESP device. This method is much more flexible. The day-night
display is implemented by manipulating the alpha channel of the day map and
drawing it on top of the night map. When computng the Azimuthal projection
any image co-ordinate not "on" the earth are made transparent on both day
and night maps so an interesting background may be seen "behind" the Earth.
Most of the heavy lifting is done by the Graphics Co-processor and threaded
which results in a very responsive program, even on a Pi 3B.

1. **Full Sized Sun Images** As a side benefit of using a capbable GUI library
and the Graphics Co-processor the program isn't limited to displaying pre-sized
thumb nails. The new program downloads the original images from the NASA
website and can display them in nearly full size in a pop-up window.

### New Widgets and Classes

Writing any program on top of a general purpose GUI library will eventually
lead to writing new Widgets. These are the ones I have written for HamChrono.

1. **TimeBox** A widget to display the time and date. It is two binary mode
settings (giving four separate modes):
    1. *Time Zone* Either GMT or the system local time zone with official Daylight
    Savings Time.
    2. *Large or Small* Either full time with seconds and date with the year, or
    abbreviated by omitting the seconds and year.
    3. The larger version also has a system monitor for temperature.

1. **ImageDisplay** Display one of a set of images loaded from the files system
(or, eventually, downloaded directly) to a thumb nail area. Touch semantics
for selecting an image from the set.

1. **ImageRepeater** Repeat the display of an image in a larger windows. Tied
to an `ImageDisplay`.

1. **ImageRepository** A non-drawing object that will maintain the set
of images and allow any `ImageDisplay` or `ImageRepeater` to render the image
at a specified location and size. The Model-View-Controller pattern with the
`ImageRepository` providing the model, and the `ImageDisplay`/`Image Repeater`
providing the display and controller.

1. **GeoChrono** The heart and soul of HamClock and HamChrono. Modeled on the
mechanical [Geochron](https://www.geochron.com/) clock which had pride of place in
my early workplaces this form of time display graphically conveys a wealth of
information important to communicators. This version is modeled closely on the
implementation in [HamClock](https://www.clearskyinstitute.com/ham/HamClock/).

1. **PassTracker** A helper Widget for GeoChrono which displays the Azimuth-Elevation
of objects being tracked.

1. **Ephemeris Model** This class manages all the ephemeris data and calculations in
one location, distributing the results as needed throughout the program using callbacks.

1. **Settings** A class to maintain user modifiable sttings in an SQLite3 database in
the users' home directory.

# Dependencies

## Libraries

1. I was going to use [LVGL](https://lvgl.io/)
a "Light and Versitile Graphics Library". The use of this
libbrary on the 7 inch RaspberryPi display is described by
[VK3ERW](http://www.vk3erw.com/index.php/16-software/63-raspberry-pi-official-7-touchscreen-and-littlevgl).
This library needs to be compiled for the target. It was my
intention to provide a package for use on the RaspberryPi.
Unfortunately I found the following:
    1. No standardized build environment;
        1. Have to move Makefiles around and put your own together.
        There is flexibility here, but this makes keeping up with 
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
    1. Install SDL2 on Debian derivatives: `sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev`
    
1. [SDL2_gfx](https://www.ferzkopp.net/Software/SDL2_gfx/Docs/html/index.html) a graphics
primitive library that adds additional drawing capability to SDL2. Using the primitive drawing
functions. Still thinking about the rest.

1. [GNU CGICC](https://www.gnu.org/software/cgicc/doc/index.html)
a [Common Gateway Interface](https://en.wikipedia.org/wiki/Common_Gateway_Interface)
library. This will allow my implementation to provide a
[RESTful](https://searchapparchitecture.techtarget.com/definition/RESTful-API)
API (as HamClock does) for remote control. Using the CGI
interface will allow the application to be tied into a web
server running on the RaspberryPi which will offload much of the
configuration, security and maintenance work of a public facing 
access.
    1. `sudo apt install libcgicc-dev`

1. [SOCI](http://soci.sourceforge.net/) a C++ Database Access Library.
    1. `sudo apt install libsoci-dev`

1. [SQLite3](https://www.sqlite.org/index.html) QLite is a C-language
library that implements a small, fast, self-contained, high-reliability,
full-featured, SQL database engine.
    1. `sudo apt install libsqlite3-dev`

1. [libcurl](https://curl.haxx.se/libcurl/) the multiprotocol transfer library. ```sudo apt-get install libcurl4-openssl-dev```

    1. [curlpp](http://www.curlpp.org/) C++ wrapper around libcURL. ```sudo apt-get install libcurlpp-dev```

1. C++17 compliant standard library.

    1. C++17 or better compiler package (if compiling from source).
    1. For the Raspberry Pi G++ version 8 suffices (C++14)

1. CMake version 3.10 or better (if compiling from source).

## Data Sources

1. [ClearSkyInstitute.Com](http://clearskyinstitute.com/)
1. [NASA](https://www.nasa.gov/)
    1. [NASA SDO Data](https://sdo.gsfc.nasa.gov/data/)
1. [Entypo Font](http://fontello.github.io/entypo/demo.html) was shipped with Nanogui-SDL, I am coninuing to use it.
1. [www.die.net](https://www.die.net) I hope to be able to use the Moon Phases imagery found there. Either way
the site deserves your attention.
1. [CelesTrack](https://www.celestrak.com/NORAD/elements/) NORAD Two-Line Element Sets.

*73*
Richard Buckley
VE3YSH
