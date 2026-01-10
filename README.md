# cookiedough

This is an experiment I started on a slow Netbook (Intel) back in 2006, if I recall correctly...

... to do some really retro (demoscene) graphical effects. 

Supported platforms as of 09/01/2026:
- Win64/x64 (builds out of the box with MSVC 2019 or higher) <- Currently possibly out of date; please fix!
- OSX/Silicon (should be set up to build and work with VSCode and the required plug-ins for C++, CMake et cetera).
- Linux/x64, courtesy of Erik 'Kusma' Faye-Lund (not very different than OSX, except CMake settings).

Possible with some tinkering (not actively maintained):
- OSX/x64

Important clues/details and credits: read what is on top of 'code/main.cpp' please.
   
For OSX and Linux we use Visual Studio Code (using all the suggested extensions: C++, CMake, CMake tools, Makefile and
perhaps a few more, VSCode will tell you what you need) which in turn through CMake will enable VSCode to build, run, debug and
soforth. 

If VSCode is misbehaving, resort to LLDB on the command line or retain your sanity and use a Windows machine and MSVC
to do your debugging. It's always wise to run the Windows debug build before releasing anything, just to be sure.

Do realize that you need to run a GNU Rocket client locally (or if you've modified '/code/rocket.cpp' perhaps on another
machine); if it can't connect it will time out and quit.

DISCLAIMER: this code has been in tinkered with since 17 years back and I'm trying to straighten it out and modernize
the use of C++ mostly but it will be an 'as we go' process. It started as a pure C 'whatever let's have some fun' project.
