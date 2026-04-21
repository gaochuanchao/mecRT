#!/bin/bash
cd ${MEC_WORKSPACE}/mecRT/simulations/distributed

for i in {0..5}; do  # 6 configurations
  echo "=============================="
  echo "Running configuration $i"
  echo "=============================="
  begin_time=$(date +%s)

  ${OMNETPP_ROOT}/bin/opp_run \
    -r $i \
    -m -u Cmdenv \
    -c Distributed \
    -n "../../src:..:../../../simu5g/emulation:../../../simu5g/simulations:../../../simu5g/src:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials" \
    --image-path "../../images:../../../inet4.5/images:../../../simu5g/images" \
    -l "../../src/mecrt" \
    -l "../../../simu5g/src/simu5g" \
    -l "../../../inet4.5/src/INET" \
    omnetpp.ini \
    --sim-time-limit=900s

  if [ $? -ne 0 ]; then
    echo "Run $i for Distributed: Error occurred" >> errorLog.txt
  fi

  echo "Finished run $i for Distributed"

  end_time=$(date +%s)
  elapsed_time=$((end_time - begin_time))
  echo "Elapsed time for run $i: ${elapsed_time}s ($((elapsed_time / 60)) minutes)"
  echo "Run $i for Distributed: Elapsed time ${elapsed_time}s ($((elapsed_time / 60)) minutes)" >> runLog.txt
done

