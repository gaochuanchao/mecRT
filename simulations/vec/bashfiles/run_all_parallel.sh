#!/bin/bash
cd /home/gao/vec_simulator/vec/simulations/vec

max_parallel=2   # Maximum parallel jobs
job_count=0

for i in {0..59}; do
  echo "Running configuration $i"
  /home/gao/omnetpp-6.0.3/bin/opp_run \
    -r $i \
    -m -u Cmdenv \
    -c Full-Test \
    -n ../../src:..:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials:../../../simu5g/emulation:../../../simu5g/simulations:../../../simu5g/src \
    -x "inet.common.selfdoc;inet.linklayer.configurator.gatescheduling.z3;inet.emulation;inet.showcases.visualizer.osg;inet.examples.emulation;inet.showcases.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.visualizer.osg;inet.examples.voipstream;simu5g.simulations.LTE.cars;simu5g.simulations.NR.cars;simu5g.nodes.cars" \
    --image-path=../../images:../../../inet4.5/images:../../../simu5g/images \
    -l ../../src/vec \
    -l ../../../inet4.5/src/INET \
    -l ../../../simu5g/src/simu5g \
    --sim-time-limit=900s \
    VEComnetpp.ini > run_$i.log 2>&1 &

  ((job_count++))
  if (( job_count % max_parallel == 0 )); then
    wait  # Wait for this batch of jobs to finish before continuing
  fi
done

wait  # Wait for any remaining jobs
echo "All 60 runs completed!"
