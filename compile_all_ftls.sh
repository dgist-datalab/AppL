#!/bin/bash
mkdir ./result
rm ./result/*_driver*

make clean

make TARGET_ALGO=DFTL driver -j 4
cp driver ./result/dftl_driver
mv driver ./result/sftl_driver
make clean

make TARGET_ALGO=Page_ftl driver -j 4
mv driver ./result/oftl_driver
make clean

make TARGET_ALGO=layeredLSM driver -j 4
mv driver ./result/appl_driver
make clean

make TARGET_ALGO=leaFTL driver -j 4
mv driver ./result/leaftl_driver
make clean

ls -al ./result
