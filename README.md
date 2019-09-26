### Qbit Distributed Ledger

Qbit is a multi-asset privacy oriented platform based on PoW/DPoS consensus with UTXO principles. Qbit focuses are:

- Entity-based governance
- Various kinds of typed transactions
- Schnorr signatures/multi-signatures by default
- Out-of-the-box privacy transactions for the value transfer
- Scalable architecture
- Atomic swaps support (cross-chain)
- Ultra-fast transaction processing
- Ability to on/off-chain processing and state syncronization
- Multichain support with sharding principles (including different parametrized consensuses for each shard) 
- Smart contract support
- Fast virtual machine (QVM) with asm-based machine codes
- Cuckatoo PoW algorithm and corresponding consensus with 51% attack resistance (first stage - around 3-6 months)
- Local-to-Global DPoS consensus (second stage)
- Full Bitcoin protocol support to archieve atomic swaps and trustless peer-to-peer orders processing
- Flexible integration infrastructure to support alts- and side- chains (gate-keepers)
- DApp making infrastructure
- Built-in support of the "open digital organization" principles
- Built-in support of DEX DApp infrastructure (as one of "oen digital organization" types) (CADEX will be the first implementation)

## Basics, motivation and exchange

There are two basic building blocks: transactions (various types) and blocks. Generic transaction consists of one or more inputs, one or more outputs, including fee. Every input and output contains hash of 'asset' type, thus single transaction can contains several assets to transfer with in&out balance consistence. Value transfer transactions is private transactions with blinded amounts (Pedersen commitment, range proofs). Transactions is not free - you need some "Qbits" to pay for them. 

On the first stage Qbits will be mined. Block reward is variative (from 0.0000 0001 up to 1.0000 0000 q-bit or from 1 q-unit to 1 0000 0000 q-units) and depends on the number of pure Qbit transactions in the past N blocks. If Qbit tx count is relatively small, than reward will be closer to 1 q-unit and if tx cont is relatively high - rewad will be closer to 1 0000 0000 q-units or 1 Qbit.

Block time - 10 secs. Emission - unlimited.

On the second stage we will implement special node role - "validator" and will switch network to the DPoS consensus mode. In that scheme each node with validator role will have relatively equal probability to close the next block. But, there will be some requirements to become validator - computational power and stable wide internet connectivity. That is because (in case of CADEX) each validator will be havy loaded by the tx flow. Reward scheme and block time remains.

Validators will support Global consensus and will produce blocks. But to implement some CADEX features (HFT, for example) we need a bunch of Local consensuses with higest network rates and minimal latency. That is because we planning to implement, in terms of CADEX, another role - market node. So, all exchange markets will be distributed across the community-driven network. And in this case we need some additional control: each active market node will have corresponding, at least, 2 passive market nodes. Active market node will process orders (create, revoke) and generate trades (as a strike result) transactions. And to be convinent, that the active node is not corrupted - corresponding and independent passive market nodes will process the same transactions (orders) and get the independent results. And if active and passive processing results are the same - that mean all goes ok. CADEX Local-to-Global consensus is just an example of such approach. 	

By the way, one of the basic CADEX principles is "zero trust". That is means, that for process orders and generate trades we do not need to trust for the third party. CADEX users will be forced to trade directly with each other. That is achieved by special orders placing and processing architecture, witch laid on atomic swaps principles. And no - you do not need to be online to process something, you just need to place orders at the appropriate price levels into corresponding market (as you do on any CEX) and thats all. Other job will be done by a couple of highly optimized smart-contracts. 

With approach above we suppose, that overall CADEX throughput will be tens of thousands orders per second.

And, of course, market nodes holders will be rewarded. Around 25% of market's fee will be distrubuted across of them. Reward distribution principles will be described in terms of CADEX "zero-administration" and implemented as a bunch of smart contracts. More over we planning to emit digital shares of CADEX. Each share will become participant of CADEX income distribution - up to 50% of market's fee will be distributed ascross CADEX shares holders on the daily basis. Remain market's fee - 25% will be sent to the CADEX R&D wing.

## Bitcoin/Qbit interoperability

In case of zero-trust principle we planning to provide transparent swap scheme. First of all every user need to dock his public Bitcoin address/key (newly generated) through emited smart-contract to the Qbit network. So, when user transfers funds from native Bitcoin network to his docked Bitcoin address, bunch of smart-contracts (including mentioned above) will provide emission of pegged Bitcoin-to-Qbit (qBTC) transaction with the corresponding amounts.

In case of CADEX, user now will be able to place orders to the BTC markets (BTC/USDT, for example). CADEX orders placement is the special transaction type - smart-contract sighned by user. Order-contract transaction's amount will be splitted by lots (1 lot = 1 signed input). So, when other side, for example ask, order-contract intersects with price level of any bid order (matching engine, based on smart-contact's code mixing), trade transaction will be emitted with corresponding sighned inputs and cross-calculated outputs with appropriate couterparties Qbit addresses.

But what about Bitcoin? Bitcoin inputs are also will be placed into corresponding order-contract as meta-data. Thus, during strike processing Bitcoin transaction also will be emitted, as a mirror of assets exchange and will be stored in Qbit blockchain. But this transaction still remains offchain of original - Bitcoin chain - and will/can be apart of future orders/trades. And only when user decides to transfer qBTC to the external BTC address, partial BTC transactions tree will be dropped into Bitcoin network and corresponding qBTC output (amount) will be burned.

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
