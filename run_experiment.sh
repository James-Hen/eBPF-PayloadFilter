echo "--- Checking make status ---"
make

echo "--- Injecting xdp object ---"
ip link set dev lo xdp obj filter.o
echo "Done"

echo "--- Capturing ---"
touch result.pcapng
chmod o=rw result.pcapng # Due to the stupid tshark permission bug
tshark -i lo -f "tcp port 1145" -w "./result.pcapng" -P -a duration:10 &

TSHARK_PID=$!

echo "--- Running host A and B ---"
sleep 1
./experiment

wait $TSHARK_PID
echo "--- Uninstalling xdp object ---"
ip link set dev lo xdp off
echo "Done"