### Qbit Distributed Ledger

## Smart contracts

Qbit Smart contracts based on the following principles:

- Smart-contract - entity with ignition function and byte-code embedded into entity body. Entity body is a special transaction with contract data and qasm code.

- Qasm is a assembler-like byte-code/machine-code level language. It operates four groups of registers and a couple of macro-instructions. With simple and understandable logic. Turing-complete.

- Virtual Machine - operates by qasm byte-code. Fast, secured enough. 

- Every miner/validator кor each transaction that enters the block that is created, which arrives at the address of the smart contract, starts the virtual machine with the body of the smart contract and the corresponding transaction. The result of execution may be many new transactions.

## How to build

1. cd ./secp256k1
2. ./autogen.sh
3. ./configure --enable-experimental --enable-module-schnorrsig --enable-module-musig --with-bignum=no
4. ./make
5. cd ..
6. cmake -DCMAKE_BUILD_TYPE=Release
