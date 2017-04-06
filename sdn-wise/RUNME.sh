#!/bin/bash
TARGET="z1"

#make TARGET=$TARGET DEFINES=COOJA=0,SINK=1 
make TARGET=$TARGET DEFINE=SINK=1
mv sdn-wise.$TARGET compiled/sink-sdn-wise.$TARGET 
#make TARGET=$TARGET DEFINES=COOJA=0,SINK=0
#make TARGET=$TARGET DEFINE=SINK=0
#mv sdn-wise.$TARGET compiled/node1-sdn-wise.$TARGET 
#mv sdn-wise.$TARGET compiled/node2-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node3-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node4-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node5-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node6-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node7-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node8-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node9-sdn-wise.$TARGET
#mv sdn-wise.$TARGET compiled/node10-sdn-wise.$TARGET
#make TARGET=$TARGET clean
