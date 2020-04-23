# producer_consumer

This is a producer producer -> server -> client programm written in C language. Can handle over 500 requests at one time

## installation

```bash
make all - compiles all files
make clean - deletes compiled files
```

## usage

```bash
./prodcon_server port number_of_max_clients
./producers port number_of_producers
./consumers port number_of_consumers
```
## extra info

passivesock - creates socket that listens for connections
connectsock - connects to passivesock

prodcon.h contains constants
