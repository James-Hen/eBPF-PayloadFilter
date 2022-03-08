# eBPF-PayloadFilter

A payload filter designed for running experiments that includes dropping packets.

## Build the filter

```bash
make
```

## How was it loaded

```bash
ip link set dev eth0 xdp obj filter.o sec .text
```

## How was it unloaded

```bash
ip link set dev eth0 xdp off
```

## How was it tested

Using the terminal version of WireShark: TShark

Check the loopback interface first using `sudo tshark -D`; assuming we have `l0` as loopback

```bash
sudo tshark -i l0 -f "tcp port 1145"
```