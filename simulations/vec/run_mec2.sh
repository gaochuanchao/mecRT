#!/bin/bash
cd /home/gao/vec_simulator/vec/simulations/vec

# for i in {0..31}; do  # 32 configurations
#   echo "=============================="
#   echo "Running configuration $i"
#   echo "=============================="

#   /home/gao/omnetpp-6.0.3/bin/opp_run \
#     -r $i \
#     -m -u Cmdenv \
#     -c Full-Test-MEC-2 \
#     -n ../../src:..:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials:../../../simu5g/emulation:../../../simu5g/simulations:../../../simu5g/src \
#     -x "inet.common.selfdoc;inet.linklayer.configurator.gatescheduling.z3;inet.emulation;inet.showcases.visualizer.osg;inet.examples.emulation;inet.showcases.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.visualizer.osg;inet.examples.voipstream;simu5g.simulations.LTE.cars;simu5g.simulations.NR.cars;simu5g.nodes.cars" \
#     --image-path=../../images:../../../inet4.5/images:../../../simu5g/images \
#     -l ../../src/vec \
#     -l ../../../inet4.5/src/INET \
#     -l ../../../simu5g/src/simu5g \
#     --sim-time-limit=900s \
#     VEComnetpp.ini

#   echo "Finished run $i for Full-Test-MEC-2"
# done

# for i in {0..15}; do  # 16 configurations
#   echo "=============================="
#   echo "Running configuration $i"
#   echo "=============================="

#   /home/gao/omnetpp-6.0.3/bin/opp_run \
#     -r $i \
#     -m -u Cmdenv \
#     -c Full-Test-MEC-2-05 \
#     -n ../../src:..:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials:../../../simu5g/emulation:../../../simu5g/simulations:../../../simu5g/src \
#     -x "inet.common.selfdoc;inet.linklayer.configurator.gatescheduling.z3;inet.emulation;inet.showcases.visualizer.osg;inet.examples.emulation;inet.showcases.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.visualizer.osg;inet.examples.voipstream;simu5g.simulations.LTE.cars;simu5g.simulations.NR.cars;simu5g.nodes.cars" \
#     --image-path=../../images:../../../inet4.5/images:../../../simu5g/images \
#     -l ../../src/vec \
#     -l ../../../inet4.5/src/INET \
#     -l ../../../simu5g/src/simu5g \
#     --sim-time-limit=900s \
#     VEComnetpp.ini

#   echo "Finished run $i for Full-Test-MEC-2"
# done

for i in {0..3}; do  # 12 configurations
  echo "=============================="
  echo "Running configuration $i"
  echo "=============================="

  /home/gao/omnetpp-6.0.3/bin/opp_run \
    -r $i \
    -m -u Cmdenv \
    -c AppCount-Test-MEC-2 \
    -n ../../src:..:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials:../../../simu5g/emulation:../../../simu5g/simulations:../../../simu5g/src \
    -x "inet.common.selfdoc;inet.linklayer.configurator.gatescheduling.z3;inet.emulation;inet.showcases.visualizer.osg;inet.examples.emulation;inet.showcases.emulation;inet.transportlayer.tcp_lwip;inet.applications.voipstream;inet.visualizer.osg;inet.examples.voipstream;simu5g.simulations.LTE.cars;simu5g.simulations.NR.cars;simu5g.nodes.cars" \
    --image-path=../../images:../../../inet4.5/images:../../../simu5g/images \
    -l ../../src/vec \
    -l ../../../inet4.5/src/INET \
    -l ../../../simu5g/src/simu5g \
    --sim-time-limit=900s \
    VEComnetpp.ini

  echo "Finished run $i for AppCount-Test-MEC-2"
done

echo "All runs completed!"
