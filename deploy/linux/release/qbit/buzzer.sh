export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/qbit/lib
./qbitd -home .buzzer -port 31415 -threadpool 2 -http 8080 -roles fullnode,miner -debug info,warn,error,store -daemon


