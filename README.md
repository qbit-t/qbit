### QUARK Distributed Ledger

1. git clone https://github.com/ElementsProject/secp256k1-zkp secp256k1
2. git checkout schnorrsig
3. ./secp256k1/autogen.sh
4. ./secp256k1/configure --enable-experimental --enable-module-schnorrsig --enable-module-musig
5. ./secp256k1/make
5. cmake ./
6. make
