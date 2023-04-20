#!/bin/bash

echo "ARRESTED DEVELOPMENT by Bypass featuring TPB"
echo "OSX is a bit of a pain when it comes to shipping self-contained Linux-style applications, please permit us to give execution rights to our main executables and well-established dynamic libraries."
echo "If you do not agree, please simply close this (terminal) window and either check Youtube or run the Windows version (available in this archive)."
echo "Thank you for your patience."

macallow () {
    if [[ $# -gt 0 ]]; then
        for f in $@; do
            sudo xattr -d -r com.apple.quarantine "${f}"
            echo "${f}"
        done
    else
        echo syntax: macallow [filename or wildcard]
    fi
}

cd libs
macallow *.dylib
cd ..
macallow "osx/cookiedough-silicon"
./osx/cookiedough-silicon