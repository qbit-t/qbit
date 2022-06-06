export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/qbit/lib
./qbitd -home .buzzer -endpoint 0.0.0.0 -port 31415 -threadpool 6 -clients 1000 -http 8080 -roles fullnode,miner -debug info,warn,error,store,cons,val -daemon
