#!/bin/bash
cd ${MEC_WORKSPACE}/mecRT/simulations/distributed
mkdir -p errorLog

for i in {0..29}; do  # 30 configurations
  echo "=============================="
  echo "Running configuration $i"
  echo "=============================="
  begin_time=$(date +%s)
  log_file="errorLog/Centralized_run_${i}.log"
  tmp_log=$(mktemp)

  ${OMNETPP_ROOT}/bin/opp_run \
    -r $i \
    -m -u Cmdenv \
    -c Centralized \
    -n "../../src:..:../../../simu5g/emulation:../../../simu5g/simulations:../../../simu5g/src:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials" \
    --image-path "../../images:../../../inet4.5/images:../../../simu5g/images" \
    -l "../../src/mecrt" \
    -l "../../../simu5g/src/simu5g" \
    -l "../../../inet4.5/src/INET" \
    omnetpp.ini \
    --sim-time-limit=900s \
    > "$tmp_log" 2>&1

  status=$?

  if [ $status -ne 0 ]; then
    mv "$tmp_log" "$log_file"
    echo "Run $i for Centralized failed with exit code $status. See $log_file for details." >> errorLog.txt
  else
    rm -f "$tmp_log"
  fi

  echo "Finished run $i for Centralized"

  end_time=$(date +%s)
  elapsed_time=$((end_time - begin_time))
  echo "Elapsed time for run $i: ${elapsed_time}s ($((elapsed_time / 60)) minutes)"
  echo "Run $i for Centralized: Elapsed time ${elapsed_time}s ($((elapsed_time / 60)) minutes)" >> runLog.txt
done
