#!/bin/bash

echo "Script started... "
# set default values for variables if they are not already defined
MAKE_CMD=${MAKE_CMD-make}

# Compile Controller
pwd=`pwd`
echo $pwd
cd src/hardware_dep/shared/ctrl_plane

gcc -Wall -pthread -std=c99  handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c mac_controller.c -o $pwd/mac_controller
#gcc -Wall -pthread -std=c99  handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c mac_l3_controller.c -o $pwd/mac_l3_controller
#gcc -Wall -pthread -std=c99  handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c mac_l3_controller_ipv6.c -o $pwd/mac_l3_controller_ipv6
#gcc -Wall -pthread -std=c99  handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c mac_nat_dw.c -o $pwd/mac_nat_dw
#gcc -Wall -pthread -std=c99  handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c mac_nat_up.c -o $pwd/mac_nat_up

cd $pwd
echo $(pwd)

# Restart mac controller in background
killall mac_controller
killall mac_l3_controller
killall mac_l3_controller_ipv6
killall mac_nat_dw
killall mac_nat_up

pkill -f mac_controller
pkill -f mac_l3_controller
pkill -f mac_l3_controller_ipv6
pkill -f mac_nat_dw
pkill -f mac_nat_up

./mac_controller traces/trace_trPR_l2_100_random.txt &
#./mac_l3_controller traces/trace_trPR_ipv4_100_random.txt  &
#./mac_l3_controller_ipv6 traces/trace_trPR_ipv6_100_random.txt &
#./mac_nat_dw traces/trace_trPR_100_random_nat_dw.txt &
#./mac_nat_up traces/trace_trPR_100_random_nat_up.txt &

echo "Controller started... "

echo "Creating Datapath Logic from P4 source."
rm -rf build
python src/transpiler.py examples/p4_src/l2_fwd.p4
#python src/transpiler.py examples/p4_src/l3_routing_test.p4
#python src/transpiler.py examples/p4_src/l3_routing_ipv6.p4
#python src/transpiler.py examples/p4_src/nat.p4

ERROR_CODE=$?
if [ "$ERROR_CODE" -ne 0 ]; then
    echo Transpiler failed with error code $ERROR_CODE
    exit 1
fi

# Compile C sources
make clean;${MAKE_CMD} -j4

rm -rf /tmp/odp*
rm -f /mnt/huge/rte*
rm -f /mnt/huge/0/odp*
rm -f /dev/hugepages/rte*
rm -f /dev/hugepages/0/odp*

