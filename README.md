a# cookiedough

This is an experiment I started on a slow Netbook (Intel) back in 2006, if I recall correctly...

... to do some really retro (demoscene) graphical effects. 

Supported platforms as of 09/01/2026:
- Win64/x64 (builds out of the box with MSVC 2019 or higher) <- Currently possibly out of date; please fix!
- OSX/Silicon (should be set up to build and work with VSCode and the required plug-ins for C++, CMake et cetera).
- Linux/x64+ARM64, courtesy of Erik 'Kusma' Faye-Lund & Niels J. de Wit (not very different to get going than OSX).

Possible with some tinkering (not actively maintained):
- OSX/x64

Important clues/details and credits: read what is on top of 'code/main.cpp' please.
   
For OSX and Linux we use Visual Studio Code (using all the suggested extensions: C++, CMake, CMake tools, LLDB et cetera; 
VSCode will tell you what you need) which in turn through CMake+LLDB will enable VSCode to build, run, debug and soforth. 

If VSCode is misbehaving, resort to LLDB on the command line or retain your sanity and use a Windows machine and MSVC
to do your debugging. It's always wise to run the Windows debug build before releasing anything, just to be sure.

Do realize that you need to run a GNU Rocket client locally (or if you've modified '/code/rocket.cpp' perhaps on another
machine); if it can't connect it will time out and quit.

DISCLAIMER: this code has been iterated over for over 18 years now and I try to polish, upgrade and re-style things a little
when I go past them but do remember it started as a "demoscene quick C experiment", not a business software suite.
