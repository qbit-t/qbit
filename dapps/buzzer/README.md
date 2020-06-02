### Buzz - Distributed microblogging platform

Buzz built on the top of the Qbit technology as DApp.

## Architecture

To start using Buzz DApp user _must_ own a little amount of qbits. Starting Buzz client app for the first time she/he can send a request to the network make a little donation to his primary qbit address.

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

BuzzComposer - making buzz, subscribe, unsubscribe, endorse, mistrust transactions
 - IBuzzComposer - a-la wallet

Peer - we need peer protocol extention to process dapps requests
 - IPeerExtention

## Requests

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createbuzzer","params":["<creator|*>", "<buzzer_unique_name>", "<buzzer_alias>", "<description>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"buzz","params":["<creator|*>", "<buzz_multibyte_body>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"buzzersubscribe","params":["<creator|*>", "<publisher_name>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"buzzerunsubscribe","params":["<creator|*>", "<publisher_name>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

## Test

### Start nodes

./qbitd -home .qbit-0 -peers 127.0.0.1:31415,127.0.0.1:31416,127.0.0.1:31417 -port 31415 -threadpool 2 -http 8080 -console -airdrop -roles fullnode,miner -debug info,warn,error,wallet,store,net,bal,pool,cons,http,val,shard

./qbitd -home .qbit-1 -peers 127.0.0.1:31415,127.0.0.1:31416,127.0.0.1:31417 -port 31416 -threadpool 2 -http 8081 -console -roles fullnode -debug info,warn,error,wallet,store,net,bal,pool,cons,http,val,shard

./qbitd -home .qbit-2 -peers 127.0.0.1:31415,127.0.0.1:31416,127.0.0.1:31417 -port 31417 -threadpool 2 -http 8082 -console -roles fullnode -debug info,warn,error,wallet,store,net,bal,pool,cons,http,val,shard

./qbitd -home .qbit-3 -peers 127.0.0.1:31415,127.0.0.1:31416,127.0.0.1:31417,127.0.0.1:31418 -port 31418 -threadpool 2 -http 8083 -console -roles fullnode,miner -debug info,warn,error,wallet,store,net,bal,pool,cons,http,val,shard

### Prepare buzzer and cubix dapps

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createdapp","params":["76PvbNsTT84VvksyZcAsu2AaRn4W2g7a47fVnC4ZoHwT196K6N", "buzzer", "Buzzer - Decentralized microblogging platform", "4096", "static"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createshard","params":["76PvbNsTT84VvksyZcAsu2AaRn4W2g7a47fVnC4ZoHwT196K6N", "buzzer", "buzzer#00", "Buzzer shard 00"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createshard","params":["76PvbNsTT84VvksyZcAsu2AaRn4W2g7a47fVnC4ZoHwT196K6N", "buzzer", "buzzer#01", "Buzzer shard 01"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createdapp","params":["76PvbNsTT84VvksyZcAsu2AaRn4W2g7a47fVnC4ZoHwT196K6N", "cubix", "Cubix - decentralized media storage", "0", "dynamic"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createshard","params":["76PvbNsTT84VvksyZcAsu2AaRn4W2g7a47fVnC4ZoHwT196K6N", "cubix", "cubix#00", "Cubix shard 00"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createshard","params":["76PvbNsTT84VvksyZcAsu2AaRn4W2g7a47fVnC4ZoHwT196K6N", "cubix", "cubix#01", "Cubix shard 01"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

### Create node-bound buzzer

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createbuzzer","params":["*", "@second", "2-ND", "Second-2"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080


### Start clients

./qbit-cli -peers 127.0.0.1:31415 -debug info,warn,error,wallet,store,net,bal,client

./qbit-cli -home .qbit-cli-1 -peers 127.0.0.1:31415 -debug info,warn,error,wallet,store,net,bal,client

./qbit-cli -home .qbit-cli-2 -peers 127.0.0.1:31415 -debug info,warn,error,wallet,store,net,bal,client
