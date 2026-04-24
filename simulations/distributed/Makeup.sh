#!/bin/bash
cd ${MEC_WORKSPACE}/mecRT/simulations/distributed

# change both start and end to the same number to run makeup for a specific configuration index
# change the configuration name (e.g., -c DistributedCapES2) to run makeup for target configuration

i=7 # specify configuration index
echo "=============================="
echo "Running configuration $i"
echo "=============================="
begin_time=$(date +%s)

${OMNETPP_ROOT}/bin/opp_run \
  -r $i \
  -m -u Cmdenv \
  -c DistTestMode \
  -n "../../src:..:../../../simu5g/emulation:../../../simu5g/simulations:../../../simu5g/src:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials" \
  --image-path "../../images:../../../inet4.5/images:../../../simu5g/images" \
  -l "../../src/mecrt" \
  -l "../../../simu5g/src/simu5g" \
  -l "../../../inet4.5/src/INET" \
  omnetpp.ini \
  --sim-time-limit=900s

echo "Finished run $i for DistTestMode"

end_time=$(date +%s)
elapsed_time=$((end_time - begin_time))
echo "Elapsed time for run $i: ${elapsed_time}s ($((elapsed_time / 60)) minutes)"
# echo "Run $i for DistTestMode: Elapsed time ${elapsed_time}s ($((elapsed_time / 60)) minutes)" >> runLog.txt
# # report to runLog if quit with error
# if [ $? -ne 0 ]; then
#   echo "Run $i for DistTestMode: Error occurred" >> errorLog.txt
# fi

