zestybench
==========

benchmarking the IPC

Run Local IPC
--------------
1. `make`
- executing `./measure`

Run Remote TCP/UDP
--------------
Remote TCP/UDP experiments are divided into three parts: TCP, UDP latency, UDP throughput

Each experiment should perform `make net` first
### TCP
- on the server, do `./server tcp`, the server will then listen at 32100
- on the client, do `./client tcp THE.IP.OF.SERVER 2`, the client would connect to server and repeat 2 times experiment

### UDP latency
- on the server, do `./server udpl`, the server will then listen at 32100
- on the client, do `./client udpl THE.IP.OF.SERVER 2 64`, the client would connect to server and repeat 2 times latency experiment with 64B send buffer

### UDP throughput
- on the server, do `./server udpt`, the server will then listen at 32100
- on the client, do `./client udpt THE.IP.OF.SERVER 2 64`, the client would connect to server and repeat 2 times latency experiment with 64B send buffer

Notes
--------------
* `gcc -D DEBUG=1 -o measure measure.c utils.c` could print more debug information
* `./server udp` and `./client udp` is another way of conducting both udp latency and throughput experiment, though this method is obsoleted, I would keep it for reference
