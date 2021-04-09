### Buzzer - Distributed microblogging platform

Buzzer built on the top of the Qbit technology as DApp.

## Architecture

To start using Buzzer DApp user _must_ own a little amount of qbits. Starting Buzzer client app for the first time she/he can send a request to the network make a little donation to his primary qbit address.

Each microblog starts with the special entity type - transaction TxBuzzer. Buzzer - contains attributes:
 - @name - human-readable buzzer unique name
 - full_name - human-readable buzzer name (could be non-unique)
 - avatar_picture - small picture in png format (120x120)
 - in:
   - qbit-fee
 - out:
   - miner qbit-fee

To start write micro-posts user need use qbits - tx payment for the each post. Each post - special transaction TxBuzz with attributes:
 - body - micro-blog body in multibyte format
 - in:
   - tx-buzzer (ltxo)
   - qbit-fee
 - out:
   - tx-buzzer (utxo)
   - miner qbit-fee

To start receiving buzzers user need to make a special tx - TxBuzzerSubscribe. Attributes:
 - tx-buzzer - original transaction
 - in:
   - qbit-fee
 - out:
   - miner qbit-fee

## Procesing

Mempool - dynamic. TxValidator - subscription lists -> broadcasting to subscribers.
 - IMempoolExtention

TransactionStorage - static. Indexing. (TransactionStorageExtention - register withing TransactionStorage and re-process transactions to build appropriate indexes)
 - ITransactionStoreExtention

BuzzerComposer - making buzz, subscribe, unsubscribe, endorse, mistrust transactions
 - IBuzzerComposer - a-la wallet

Peer - we need peer protocol extention to process dapps requests
 - IPeerExtention

## How to build (Linux Desktop)

1. You need to clone qt-5.15.2
2. Build Qt STATIC version
2.1. Make, for example ~/qt-static-release
2.2. Create ./build.sh file with contents (check your directories before start):

	export PATH=$JAVA_HOME/bin:$PATH
	../qt5/configure -static -release -disable-rpath -nomake tests -nomake examples -no-warnings-are-errors -skip qtdocgallery -prefix /usr/local/Qt-5.15.2-desktop -bundled-xcb-xinput -opengl desktop -qt-libpng -qt-libjpeg

2.3. Execute ./build.sh
2.4. Run ./make (it can take long time)
3. Go to ./qt/app directory
4. Run /usr/local/Qt-5.15.2-desktop/qmake ./buzzer-desktop.pro
5. Run make
6. Executable will be in ./release folder

## How to build (Windows Desktop)

1. See section in root README.MD on how to build for Windows

## Requests

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createbuzzer","params":["<creator|*>", "<buzzer_unique_name>", "<buzzer_alias>", "<description>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"buzz","params":["<creator|*>", "<buzz_multibyte_body>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"buzzersubscribe","params":["<creator|*>", "<publisher_name>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"buzzerunsubscribe","params":["<creator|*>", "<publisher_name>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

## Test

### Start nodes

./qbitd -home .qbit-0 -peers 127.0.0.1:31415,127.0.0.1:31416,127.0.0.1:31417 -port 31415 -threadpool 2 -http 8080 -console -airdrop -roles fullnode,miner -debug info,warn,error,wallet,store,net,bal,pool,cons,http,val,shard -testnet -sparing

./qbitd -home .qbit-1 -peers 127.0.0.1:31415,127.0.0.1:31416,127.0.0.1:31417 -port 31416 -threadpool 2 -http 8081 -console -roles fullnode -debug info,warn,error,wallet,store,net,bal,pool,cons,http,val,shard -testnet -sparing

./qbitd -home .qbit-2 -peers 127.0.0.1:31415,127.0.0.1:31416,127.0.0.1:31417 -port 31417 -threadpool 2 -http 8082 -console -roles fullnode -debug info,warn,error,wallet,store,net,bal,pool,cons,http,val,shard -testnet -sparing

./qbitd -home .qbit-3 -peers 127.0.0.1:31415,127.0.0.1:31416,127.0.0.1:31417,127.0.0.1:31418 -port 31418 -threadpool 2 -http 8083 -console -roles fullnode,miner -debug info,warn,error,wallet,store,net,bal,pool,cons,http,val,shard -testnet -sparing

### Prepare buzzer and cubix dapps

1. Get address for node 0

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"getkey","params":[]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

2. Replace ADDRESS_NODE_0 by "address" and run

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createdapp","params":["ADDRESS_NODE_0", "buzzer", "Buzzer - Decentralized microblogging platform", "4096", "static"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createshard","params":["ADDRESS_NODE_0", "buzzer", "buzzer#00", "Buzzer shard 00"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createshard","params":["ADDRESS_NODE_0", "buzzer", "buzzer#01", "Buzzer shard 01"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createdapp","params":["ADDRESS_NODE_0", "cubix", "Cubix - decentralized media storage", "0", "dynamic"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createshard","params":["ADDRESS_NODE_0", "cubix", "cubix#00", "Cubix shard 00"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createshard","params":["ADDRESS_NODE_0", "cubix", "cubix#01", "Cubix shard 01"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

### Create node-bound buzzer

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createbuzzer","params":["*", "@second", "2-ND", "Second-2"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080


### Start clients

./qbit-cli -peers 127.0.0.1:31415 -debug info,warn,error,wallet,store,net,bal,client

./qbit-cli -home .qbit-cli-1 -peers 127.0.0.1:31415 -debug info,warn,error,wallet,store,net,bal,client

./qbit-cli -home .qbit-cli-2 -peers 127.0.0.1:31415 -debug info,warn,error,wallet,store,net,bal,client

### Create QTT asset

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createasset","params":["ADDRESS_NODE_0", "QTT", "Qbit Technology Token", "10000", "10000", "1", "limited"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createassetemission","params":["ADDRESS_NODE_0", "ASSET_TYPE" ]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080