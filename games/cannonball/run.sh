#!/bin/sh

mydir=`dirname "$0"`

export HOME=$mydir

cd $mydir && ./cannonball

pid record $!
wait $!
pid erase
