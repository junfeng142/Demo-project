#!/bin/bash

cp ./sdlpal  _opk_paladin/

/home/javonca/libs/mksquashfs ./_opk_paladin/* paladin_funkey-s.opk -all-root -no-xattrs -noappend -no-exports
