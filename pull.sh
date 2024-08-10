#!/bin/bash
git pull
wd=$(pwd)
cd lib/blt-gp
git pull
cd lib/blt
git pull
cd $wd
cd lib/blt-graphics
git pull

