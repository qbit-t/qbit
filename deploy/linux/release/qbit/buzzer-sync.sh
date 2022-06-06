export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/qbit/lib
./qbitd -home .buzzer -peers 212.42.43.24:31415,212.42.43.9:31415,212.42.43.8:31415 -port 31415 -threadpool 6 -http 8080 -roles fullnode -debug info,warn,error,store,cons,val -daemon -no-config
