#!/bin/bash
TARGET="z1"

#mkdir cooja_firmwares

#make TARGET=$TARGET DEFINES=COOJA=0,SINK=1 
#make TARGET=$TARGET DEFINE=SINK=1
#mv sdn-wise.$TARGET compiled/sink-sdn-wise.$TARGET 
#mv sdn-wise.$TARGET sdn-wise-sink.$TARGET
#make TARGET=$TARGET DEFINES=COOJA=0,SINK=0
make TARGET=$TARGET DEFINE=SINK=0
mv sdn-wise.$TARGET compiled/node1-sdn-wise.$TARGET 
#mv sdn-wise.$TARGET compiled/node1-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node1-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node1-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node1-sdn-wise.$TARGET
#mv sdn-wise.$TARGET sdn-wise-node.$TARGET
#make TARGET=$TARGET clean
