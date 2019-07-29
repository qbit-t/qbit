### Qbit Distributed Ledger

Qbit is a multi-asset privacy oriented platform based on PoW/DPoS consensus with UTXO principles. Qbit focuses are:

- Entity-based governance
- Various kinds of typed transactions
- Schnorr signatures/multi-signatures by default
- Out-of-the-box privacy transactions for the value transfer
- Scalable architecture
- Ultra-fast transaction processing
- Ability to on/off-chain processing and state syncronization
- Smart contract support
- Fast virtual machine (QVM) with asm-based machine codes
- Cuckatoo PoW algorithm
- Local-to-Global DPoS consensus
- DApp flexible infrastructure
- Built-in support for "digital organization" principles
- Built-in support for DEX DApp infrastructure (as one of "digital organization" types)

## Basics

There are two basic building blocks: transactions (various types) and blocks. Generic transaction consists of one or more inputs, one or more outputs and special inputs and outputs for fee processing. Every input and output contains hash of 'asset' type, thus single transaction can contains several assets to transfer with in/out balance consistence. 

## Smart contracts

Qbit Smart contracts based on the following principles:

- Smart-contract - entity with some methods (or methods' table) and byte-code embedded into entity body. Entity body is a special transaction with contract data and qasm code.

- Qasm is a assembler-like byte-code/machine-code level language. It operates four groups of registers and a couple of macro-instructions. With simple and understandable logic. Turing-complete.

- Virtual Machine - operates by qasm byte-code. Fast, secured enough. 

- Contract called by special transaction type (call-transaction) with contract address, method hash and couple of parameters to call. Contract method calls processed by all miners/validators in the same way.

## How to build

1. cd ./secp256k1
2. ./autogen.sh
3. ./configure --enable-experimental --enable-module-schnorrsig --enable-module-musig --enable-module-ecdh --enable-module-generator --enable-module-rangeproof --with-bignum=no
4. cd ..
5. cmake -DCMAKE_BUILD_TYPE=Release
