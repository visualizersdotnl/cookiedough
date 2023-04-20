# requires https://github.com/SCG82/macdylibbundler (in lieu of making an actual OSX packaged application)
mv cookiedough cookiedough-silicon
dylibbundler -x cookiedough-silicon -cd -of
rm -rf ../libs
mv libs ../libs
