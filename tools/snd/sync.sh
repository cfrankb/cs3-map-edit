#!/bin/bash

TARGET_EDITOR=/disks/salmon/cfrankb/toolkit/qt/mapedit

make && build/sndx
cp -v out/sounds.h $TARGET_EDITOR
cp -v out/sounds.dat $TARGET_EDITOR/data
