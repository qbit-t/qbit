### Qbit Distributed Ledger

Qbit is a multi-asset privacy oriented platform based on PoW/DPoS consensus with UTXO principles. Qbit focuses are:

	- Entity-based governance
	- Various kinds of typed transactions
	- Schnorr signatures/multi-signatures by default
	- Support of different signature schemes
	- Out-of-the-box privacy transactions for the value transfer
	- Scalable architecture
	- Atomic swaps support
	- Ultra-fast transaction processing
	- Multichain support with sharding principles (including different parametrized consensuses for each shard) 
	- Smart contract support
	- Fast virtual machine (QVM) with asm-based machine codes
	- Cuckatoo PoW algorithm and corresponding consensus with 51% attack resistance
	- Local-to-Global DPoS consensus
	- Full Bitcoin protocol support to archieve atomic swaps and trustless peer-to-peer orders processing
	- Flexible integration infrastructure to support alts- and side- chains (gate-keepers)
	- DApp making infrastructure
	- Built-in support of the "open digital organization" principles
	- Built-in support of DEX DApp infrastructure (as one of "open digital organization" types)

## Basics

There are two basic building blocks: transactions (various types) and blocks. Generic transaction consists of one or more inputs, one or more outputs, including fee. Every input and output contains hash of 'asset' type, thus single transaction can contains several assets to transfer with in&out balance consistence. Value transfer transactions is private transactions with blinded amounts (Pedersen commitment, range proofs). Transactions is not free - you need some "Qbits" to pay for them. 

Block time - 5 secs. Emission ~ 66M.

## Bitcoin/Qbit interoperability

In case of zero-trust principle we planning to provide transparent swap scheme. First of all every user need to dock his public Bitcoin address/key (newly generated) through emited smart-contract to the Qbit network. So, when user transfers funds from native Bitcoin network to his docked Bitcoin address, bunch of smart-contracts will provide emission of pegged Bitcoin-to-Qbit (qBTC) transaction with the corresponding amounts.

## Smart contracts

Qbit Smart contracts based on the following principles:

- Smart-contract - entity with some methods (or methods' table) and byte-code embedded into entity body. Entity body is a special transaction with contract data and qasm code.

- Qasm is a assembler-like byte-code/machine-code level language. It operates four groups of registers and a couple of macro-instructions. With simple and understandable logic. Turing-complete.

- Virtual Machine - operates by qasm byte-code. Fast, secured enough. 

- Contract called by special transaction type (call-transaction) with contract address, method hash and couple of parameters to call. Contract method calls processed by all miners/validators in the same way.

## How to build

Prerequisites:
1. [optionally] sudo apt-get update && sudo apt-get install build-essential
2. [optionally] sudo apt-get install autoconf
3. [optionally] sudo apt-get install libtool
4. [optionally] sudo apt-get install cmake
5. sudo apt-get install libjpeg-dev
6. sudo apt-get install libpng-dev

Build Linux:
1. cd ./secp256k1
2. ./autogen.sh
3. ./configure --enable-experimental --enable-module-schnorrsig --enable-module-musig --enable-module-ecdh --enable-module-generator --enable-module-rangeproof --with-bignum=no
4. cd ..
5. cd ./boost
6. ./bootstrap.sh --with-libraries=system,thread,chrono,random,filesystem --prefix=../
7. ./b2
8. cd ..
9. cmake -DCMAKE_BUILD_TYPE=Release
10. make

Please, note - if make produces linking errors, do the following:
1. cmake ./
2. make

Build Windows (on Linux with special toolset - https://mxe.cc):
1. git clone https://github.com/mxe/mxe ~/mxe
2. cd ~/mxe
3. edit ./settings.mk and add MXE_TARGETS := x86_64-w64-mingw32.static
4. make
5. add path to MXE binaries to your PATH (edit .bashrc, for example)
6. cd ./secp256k1
7. ./autogen.sh
8. ./configure --enable-experimental --enable-module-schnorrsig --enable-module-musig --enable-module-ecdh --enable-module-generator --enable-module-rangeproof --with-bignum=no --host=x86_64-w64-mingw32.static
9. cd ..
10. x86_64-w64-mingw32.static-cmake -DCMAKE_BUILD_TYPE=Release
11. x86_64-w64-mingw32.static-cmake ./
12. make

Please, note - (4) it can take long time, very long and not all from the packages you need. During compilation you may experience building errors, consider to add to the EXCLUDE_PKGS and rerun make

## Requests

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"getkey","params":[]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"getbalance","params":[]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"sendtoaddress","params":["<asset|*>", "<to>", "<amount>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"gettransaction","params":["<tx_id>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createdapp","params":["<owner>", "<short_name>", "<long_name>", "<tx_dapp_instance>", "<static|dynamic>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"createshard","params":["<creator>", "<dapp_name>", "<unique_short_name>", "<description>"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"getpeerinfo","params":[]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080

curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"mallocstats","params":["[thread_id]", "[alloc_class]", "[path_to_dump]"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080
