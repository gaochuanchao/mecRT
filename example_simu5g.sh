#!/bin/bash

# Set environment variables
export DISPLAY="${DISPLAY}"
export LIBGL_ALWAYS_INDIRECT=1

# Change to working directory
cd ~/simulator/simu5g/simulations/NR/tutorial

# Run the simulation
~/omnetpp-6.0.3/bin/opp_run \
  -m \
  -u Qtenv \
  -c Single-UE \
  -n "../../../emulation:../..:../../../src:../../../../inet4.5/examples:../../../../inet4.5/showcases:../../../../inet4.5/src:../../../../inet4.5/tests/validation:../../../../inet4.5/tests/networks:../../../../inet4.5/tutorials" \
  --image-path "../../../images:../../../../inet4.5/images" \
  -l "../../../src/simu5g" \
  -l "../../../../inet4.5/src/INET" \
  omnetpp.ini

