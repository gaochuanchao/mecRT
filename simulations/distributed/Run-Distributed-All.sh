#!/bin/bash
cd ${MEC_WORKSPACE}/mecRT/simulations/distributed
mkdir -p errorLog

# Run Distributed configuration
for i in {0..5}; do  # 6 configurations
  echo "=============================="
  echo "Running configuration $i"
  echo "=============================="
  begin_time=$(date +%s)
  log_file="errorLog/Distributed_run_${i}.log"
  tmp_log=$(mktemp)

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
    --sim-time-limit=900s \
    > "$tmp_log" 2>&1

  status=$?

  if [ $status -ne 0 ]; then
    mv "$tmp_log" "$log_file"
    echo "Run $i for Distributed failed with exit code $status. See $log_file for details." >> errorLog.txt
  else
    rm -f "$tmp_log"
  fi

  echo "Finished run $i for Distributed"

  end_time=$(date +%s)
  elapsed_time=$((end_time - begin_time))
  echo "Elapsed time for run $i: ${elapsed_time}s ($((elapsed_time / 60)) minutes)"
  echo "Run $i for Distributed: Elapsed time ${elapsed_time}s ($((elapsed_time / 60)) minutes)" >> runLog.txt
done


# Run DistTestMode configuration
for i in {0..11}; do  # 12 configurations
  echo "=============================="
  echo "Running configuration $i"
  echo "=============================="
  begin_time=$(date +%s)
  log_file="errorLog/DistTestMode_run_${i}.log"
  tmp_log=$(mktemp)

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
    --sim-time-limit=900s \
    > "$tmp_log" 2>&1

  status=$?

  if [ $status -ne 0 ]; then
    mv "$tmp_log" "$log_file"
    echo "Run $i for DistTestMode failed with exit code $status. See $log_file for details." >> errorLog.txt
  else
    rm -f "$tmp_log"
  fi

  echo "Finished run $i for DistTestMode"

  end_time=$(date +%s)
  elapsed_time=$((end_time - begin_time))
  echo "Elapsed time for run $i: ${elapsed_time}s ($((elapsed_time / 60)) minutes)"
  echo "Run $i for DistTestMode: Elapsed time ${elapsed_time}s ($((elapsed_time / 60)) minutes)" >> runLog.txt
done


# Run DistTestModeCapES1 configuration
for i in {0..5}; do  # 6 configurations
  echo "=============================="
  echo "Running configuration $i"
  echo "=============================="
  begin_time=$(date +%s)
  log_file="errorLog/DistributedCapES1_run_${i}.log"
  tmp_log=$(mktemp)

  ${OMNETPP_ROOT}/bin/opp_run \
    -r $i \
    -m -u Cmdenv \
    -c DistributedCapES1 \
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
    echo "Run $i for DistributedCapES1 failed with exit code $status. See $log_file for details." >> errorLog.txt
  else
    rm -f "$tmp_log"
  fi

  echo "Finished run $i for DistributedCapES1"

  end_time=$(date +%s)
  elapsed_time=$((end_time - begin_time))
  echo "Elapsed time for run $i: ${elapsed_time}s ($((elapsed_time / 60)) minutes)"
  echo "Run $i for DistributedCapES1: Elapsed time ${elapsed_time}s ($((elapsed_time / 60)) minutes)" >> runLog.txt
done


# Run DistTestModeCapES2 configuration
for i in {0..2}; do  # 3 configurations
  echo "=============================="
  echo "Running configuration $i"
  echo "=============================="
  begin_time=$(date +%s)
  log_file="errorLog/DistributedCapES2_run_${i}.log"
  tmp_log=$(mktemp)

  ${OMNETPP_ROOT}/bin/opp_run \
    -r $i \
    -m -u Cmdenv \
    -c DistributedCapES2 \
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
    echo "Run $i for DistributedCapES2 failed with exit code $status. See $log_file for details." >> errorLog.txt
  else
    rm -f "$tmp_log"
  fi

  echo "Finished run $i for DistributedCapES2"

  end_time=$(date +%s)
  elapsed_time=$((end_time - begin_time))
  echo "Elapsed time for run $i: ${elapsed_time}s ($((elapsed_time / 60)) minutes)"
  echo "Run $i for DistributedCapES2: Elapsed time ${elapsed_time}s ($((elapsed_time / 60)) minutes)" >> runLog.txt
done
