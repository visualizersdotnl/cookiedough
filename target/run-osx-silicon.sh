#!/bin/bash

echo "*** Arrested Development is about to start! ***"
echo "OSX is a pain when it comes to shipping self-contained Linux-style applications; please permit us to give execution rights to a few harmless and well-known libraries (in '/libs') so the demo can run. Thank you."
echo "If not I understand, then simply press CTRL-C and wait for/check out the Youtube capture or Windows version (available in this same archive)."

macallow () {
    if [[ $# -gt 0 ]]; then
        for f in $@; do
            sudo xattr -d -r com.apple.quarantine $(realpath $f)
        done
    else
        echo syntax: macallow [filename or wildcard]
    fi
}

cd libs
macallow *.dylib
cd ..
./osx/cookiedough-silicon