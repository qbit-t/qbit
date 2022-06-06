export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/qbit/lib
./qbit-cli -home .buzzer-cli -peers 127.0.0.1:31415 -debug info,warn,error,wallet,store,client

