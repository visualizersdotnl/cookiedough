# cookiedough

This is an experiment I started on a slow Netbook (Intel) back in 2006, if I recall correctly, to do some really retro
(demoscene) graphical effects. 

WARNING 1: current project being made is Revision 2023 demo, "Arrested development" by Bypass - clean up planned afterwards.
WARNING 2: OSX build files aren't necessarily easy to get to work; read through 'main.cpp' and follow the crumbs, I'll fix that when I've got time (see issue list).

Supported platforms as of 20/04/2023:
- Win64/x64 (compiles out of the box with MSVC 2019 or higher)
- OSX/x64 (compiles with some effort)
- OSX/Silicon (should compile with little effort if you follow instructions)

For OSX there is a script in '/target/OSX' which will generate two folders in the root, one for a debug build and one
for release. However, it's easier, certainly if you want a non-XCode debugger, to use Visual Studio Code. Install the
C++ extension, the CMake one, CMake tools, Makefile extension and probably a couple more, but Code will tell you what you
need. If you're not going to use Code, please just run the generated make files and launch from the same folder; use
LLDB on command line to debug ;)

Do realize that you need to run a GNU Rocket client locally (or if you've modified '/code/rocket.cpp' perhaps on another
machine); if it can't connect it will time out and quit.

DISCLAIMER: this code has been in tinkered with since 17 years back and I'm trying to straighten it out and modernize
the use of C++ mostly but it will be an 'as we go' process. It started as a pure C project.

Check /code/main.cpp for a list of third party libraries used and some more useful hints (certainly if you want to build it yourself).
