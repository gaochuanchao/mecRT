#!/bin/bash

# Set environment variables
export DISPLAY="${DISPLAY}"
export LIBGL_ALWAYS_INDIRECT=1

# Change to working directory
cd ${MEC_WORKSPACE}/mecRT/simulations/decentralized

# Run the simulation
# -r 0: run the simulation for 0 iterations
# -u Qtenv: "Qtenv" for GUI mode, use "Cmdenv" for command line mode
# -c Uploading: specify the configuration to use  opp_run
${OMNETPP_ROOT}/bin/opp_run  \
  -r 0 \
  -m \
  -u Qtenv \
  -c Uploading \
  -n "../../src:..:../../../simu5g/emulation:../../../simu5g/simulations:../../../simu5g/src:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials" \
  --image-path "../../images:../../../inet4.5/images:../../../simu5g/images" \
  -l "../../src/mecrt" \
  -l "../../../simu5g/src/simu5g" \
  -l "../../../inet4.5/src/INET" \
  omnetpp.ini \
  --sim-time-limit=100s
