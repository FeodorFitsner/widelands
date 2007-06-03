This document describes steps needed to compile Widelands for different
systems using different compilers. If you have problems, please also have a
look at our website http://www.widelands.org, especially the FAQ.



================
= Dependencies =
================
These are the libraries you need. You also need the headers and link libraries
(on some distributions these come in separate packages, e.g. 'libpng-dev'),
for Widelands makes direct use of them:
   - SDL >= 1.2.8
   - SDL_mixer >= 1.2.6
   - SDL_image
   - SDL_net
   - SDL_ttf >= 2.0.0
   - gettext (look at FAQ if you have problems with -lintl)
   - libpng
   - zlib
   - libiconv (only needed under win32)
   - libintl (only needed under win32)

Make sure you have them all. If you encounter library versions that do not work,
please tell us.

For compiling, you will also need
   - Python >= 2.4
   - scons >= 0.97 (optional but recommended, see below)
If you have a desparate need to use older Python versions then tell us. It'd be
possible, but the inconvenience seems not to be worthwile so far.



============
=   Unix   =
============

scons
------------------
Using scons for building is the preferred way starting with Widelands-build10.
We still support make, but the motivation to do so is dwindling rapidly.

If you already have scons installed on your machine, change to the Widelands
directory and execute "scons". That's it.

This will use the default build type, which is "release" for published releases
(who'd have guessed? *g*) and "debug" anytime else. If you want to change the
build type, execute e.g. "scons build=debug". To see all available types, do
"scons -h"

If you don't have scons installed, you can still build Widelands: just replace
"scons" with "./build-widelands.sh" in the above instructions. This is a
wrapper around a minimal version of scons that we deliver together with
widelands. You can even use this for development, but we recommend a full
install anyway.

Several other build targets are available :-) but not documented yet :-(


make
------------------
Edit src/config.h.default to your liking and check the Makefile for more
user definable variables. If everything is good, simply run GNU make in the
widelands directory.

Once again: there's a strong possibility that make support is on it's way out.



=============
=  Windows  =
=============
If you're searching for a good SVN tool for windows, we recommend Tortoise
SVN.
Check http://tortoisesvn.sourceforge.net.

mingw and msys
------------------
This describes the steps needed to set up a free development enviroment
under Windows and compiling Widelands.
 - get the latest MSYS snapshot from http://sourceforge.net/projects/mingw
 - install it
 - get the latest complete mingw tarball from
   http://sourceforge.net/projects/mingw
 - unpack it under the MSYS sh-shell in the dir /mingw
 - get all library source tarballs which are mentioned in DEPENDENCIES and STDPort from http://www.stlport.com
 - compile and install all stuff
 - check out a widelands SVN version or get a build source release
 - unpack it, edit the makefile user variables and run make
 - if there were no problems, you're done. start developing and commit your
   changes
   
Bloodshed's DevCpp
------------------
Since Build10, we support a DevCpp-Project file for Bloodshed's free GPLed IDE.
DevCpp uses mingw and can be set up to use an allready installed mingw-environment
(like the one you can set up via the instructions above).
You can get the newest version of DevCpp at http://www.bloodshed.net

The mingw-version, that comes with DevCpp does not include all needed librarys so you
have to install few manually. For few librarys dev-packages are available, so you 
might check http://devpaks.org/ for the librarys you need.

If everything is set up correctly, you can open the [Widelands]/build/win32/Widelands.dev,
can change and save anything and of course can compile Widelands.

InnoSetup
------------------
Since Build10 we support a Innosetup file, which can be used for compiling a Setup
(like the official Widelands-Setup available on sourceforge.net-mirrors).
Innosetup can be downloaded from http://www.jrsoftware.org 

If you've installed InnoSetup, you just need to open [Widelands]/build/win32/Widelands.iss.
You might change few settings or directly start packing/compiling the setup.

ATTENTION!
Please check if all needed *.dll-files are in [Widelands]-directory during Setup packing/compile.
Else your setup might be useless :-?

