On macosx, please put the files package_all.sh and package.sh inside the .ofx bundle, for example:
ColorNegInvert-1.0.ofx.bundle/Contents/MacOS/

then copy all boost dependencies inside the MacOS directory,
then run ./package_all.sh

This will create a distribuable OFX package.

