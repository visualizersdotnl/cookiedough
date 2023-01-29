# cookiedough

This is a little testbed I set up to experiment with some retro (demoscene) graphics effects, back in 2007-2009.
It shall henceforth be reffered to as 'cocktails with Kurt Bevacqua' (https://en.wikipedia.org/wiki/Kurt_Bevacqua).

Was also used as testbed for FM. BISON, a hybrid/FM software synthesizer, but I deleted (almost?) all traces of that!

It's built using Visual Studio 2017, or rather, last tested with Visual Studio 2019.

There is a CMake script (in '/code') to build for OSX, please read top of 'main.cpp' to get your dependencies right.
Then navigate to '/target/OSX', run 'gen-builds.sh', use make to build either and run them from their respective directories.

- 29/01/2023: OSX version runs!
- 25/07/2018: got it running, back in production.
- Use one of the supplied Rocket editors (Kusma's or Emoon's).
- Set your startup directory to '/target'.

Disclaimer: I've worked this over 10 years in varying states of mental clarity, so I know there's enough low hanging fruit.
Check /code/main.cpp for a list of third party libraries used.

This is old-school C/C++, I'm sticking with it (mostly) for consistency.

** Has content from Comanche (videogame by Novalogic in the early 1990s).  
** Has content from The Non-stop Ibiza Experience (demoscene, Orange).

Said content will not be spread in the form of a release.
