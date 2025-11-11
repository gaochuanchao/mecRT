#!/bin/bash
cd ${MEC_WORKSPACE}/mecRT/simulations/decentralized

echo "============================================="
echo "Running configuration NodeFailureExample"
echo "============================================="

# Qtenv for visualization, Cmdenv for command line
${OMNETPP_ROOT}/bin/opp_run \
  -r 0 \
  -m -u Cmdenv \
  -c FailureTest \
  -n "../../src:..:../../../simu5g/emulation:../../../simu5g/simulations:../../../simu5g/src:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials" \
  --image-path "../../images:../../../inet4.5/images:../../../simu5g/images" \
  -l "../../src/mecrt" \
  -l "../../../simu5g/src/simu5g" \
  -l "../../../inet4.5/src/INET" \
  omnetpp-fault.ini \
  --sim-time-limit=900s

echo "Finished run 0 for FailureTest"

