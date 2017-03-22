#!/bin/bash

# Let's free up some space
rm -Rf ~/contiki-3.0
rm -Rf ~/contiki
apt-get remove --purge libreoffice* openjdk-* firefox thunderbird -y
apt-get clean
apt-get autoremove -y

# Update to Java 8 and install Ant and Maven
add-apt-repository ppa:openjdk-r/ppa -y
apt-get update
apt-get install openjdk-8-jdk ant maven openvswitch-switch -y
update-java-alternatives -s java-1.8.0-openjdk-i386

# Setup environment variables
sudo -u user sed -i '/export JAVA_HOME=\/usr\/lib\/jvm\/java-7-openjdk-i386/c\export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-i386' ~/.bashrc
source ~/.bashrc

# Download sdn-wise-contiki
sudo -u user git clone git://github.com/sdnwiselab/sdn-wise-contiki

# Download sdn-wise-java
sudo -u user git clone git://github.com/sdnwiselab/sdn-wise-java

# Downlad Apache Karaf
sudo -u user mkdir ~/Downloads ~/Applications
sudo -u user wget http://archive.apache.org/dist/karaf/3.0.2/apache-karaf-3.0.2.tar.gz -P ~/Downloads
sudo -u user tar -zxvf ~/Downloads/apache-karaf-3.0.2.tar.gz -C ~/Applications/

# Download ONOS
sudo -u user git clone git://github.com/sdnwiselab/onos

# Download Mininet
sudo -u user git clone git://github.com/mininet/mininet

# Setup environment variables
sudo -u user echo "export ONOS_ROOT=~/onos" >> ~/.bashrc
source ~/.bashrc
source $ONOS_ROOT/tools/dev/bash_profile

# Add ONOS and SDN-WISE to Karaf
sudo -u user sed -i '/featuresRepositories=mvn:org.apache.karaf.features\/standard\/3.0.2\/xml\/features,mvn:org.apache.karaf.features\/enterprise\/3.0.2\/xml\/features,mvn:org.ops4j.pax.web\/pax-web-features\/3.1.2\/xml\/features,mvn:org.apache.karaf.features\/spring\/3.0.2\/xml\/features/c\featuresRepositories=mvn:org.apache.karaf.features\/standard\/3.0.2\/xml\/features,mvn:org.apache.karaf.features\/enterprise\/3.0.2\/xml\/features,mvn:org.ops4j.pax.web\/pax-web-features\/3.1.2\/xml\/features,mvn:org.apache.karaf.features\/spring\/3.0.2\/xml\/features,mvn:org.onosproject\/onos-features\/1.0.2-SNAPSHOT\/xml\/features' /home/user/Applications/apache-karaf-3.0.2/etc/org.apache.karaf.features.cfg
sudo -u user sed -i '/featuresBoot=config,standard,region,package,kar,ssh,management/c\featuresBoot=config,standard,region,package,kar,ssh,management,onos-api,onos-core-trivial,onos-cli,onos-openflow,onos-app-fwd,onos-app-mobility,onos-gui,onos-sdnwise,onos-sdnwise-providers' /home/user/Applications/apache-karaf-3.0.2/etc/org.apache.karaf.features.cfg

# Install Mininet
mininet/util/install.sh -nfv

# Download sdn-wise subprojects
cd sdn-wise-contiki && sudo -u user git submodule update --init && cd contiki/tools/cooja && sudo -u user git submodule update --init && cd
