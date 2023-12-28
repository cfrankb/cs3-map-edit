#!/bin/bash
NAME=$1
cmp -l out/${NAME}.mcz out/${NAME}0.mcz | gawk '{printf "%08X %02X %02X\n", $1-1, strtonum(0$2), strtonum(0$3)}'
