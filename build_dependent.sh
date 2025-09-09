#!/bin/bash
source ~/omnetpp-6.0.3/setenv
cd ~/simulator

echo "Building INET..."
make -C inet4.5 MODE=release -j$(nproc)
make -C inet4.5 MODE=debug -j$(nproc)

echo "Building Simu5G..."
make -C simu5g MODE=release -j$(nproc)
make -C simu5g MODE=debug -j$(nproc)
