#!/bin/bash
TARGET="z1"

#mkdir cooja_firmwares

make TARGET=$TARGET DEFINES=COOJA=0,SINK=1 
#mv sdn-wise.$TARGET cooja_firmwares/sink.$TARGET 
mv sdn-wise.$TARGET sdn-wise-sink.$TARGET
make TARGET=$TARGET DEFINES=COOJA=0,SINK=0
#mv sdn-wise.$TARGET cooja_firmwares/node.$TARGET 
mv sdn-wise.$TARGET sdn-wise-node.$TARGET
#make TARGET=$TARGET clean
