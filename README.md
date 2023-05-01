# cookiedough

This is an experiment I started on a slow Netbook (Intel) back in 2006, if I recall correctly, to do some really retro (demoscene) graphical effects. 

WARNING 1: last project being made was our Revision 2023 PC entry "Arrested Development" by Bypass - cleanup session planned after I branch off.
WARNING 2: OSX build files aren't necessarily easy to get to work; read through 'code/main.cpp' and follow the bread crumbs; I'll fix that when I've got time (see issue list).

Supported platforms as of 20/04/2023:
- Win64/x64 (builds out of the box with MSVC 2019 or higher)
- OSX/x64 (builds with some effort, might be fixed soon)
- OSX/Silicon (should build with little effort if you follow instructions)
- Linux, courtesy of Erik 'Kusma' Faye-Lund

As of 26/04/2023 Linux should also be supported, thank you Erik Faye-Lund.

For OSX preferably something like Visual Studio Code (using all the suggested extensions: C++, CMake, CMake tools, Makefile and
perhaps a few more, Code will tell you what you need) which in turn through CMake will enable VSCode to build, run, debug and
soforth. If VSCode is misbehaving, resort to LLDB on the command line or retain your sanity and use a Windows machine and MSVC
to do your debugging. It's always wise to run the Windows debug build before releasing anything, just to be sure.

Do realize that you need to run a GNU Rocket client locally (or if you've modified '/code/rocket.cpp' perhaps on another
machine); if it can't connect it will time out and quit.

DISCLAIMER: this code has been in tinkered with since 17 years back and I'm trying to straighten it out and modernize
the use of C++ mostly but it will be an 'as we go' process. It started as a pure C 'whatever let's have some fun' project.

Check '/code/main.cpp' for a list of third party libraries used and some more useful hints (certainly if you want to build it yourself).
