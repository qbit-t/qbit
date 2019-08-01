cmake_minimum_required(VERSION 2.8)

# This project can use C11, but will gracefully decay down to C89.
set(CMAKE_C_STANDARD 11)

# This project requires C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

# Version
add_definitions(-DCLIENT_VERSION_MAJOR=0)
add_definitions(-DCLIENT_VERSION_MINOR=1)
add_definitions(-DCLIENT_VERSION_REVISION=0)
add_definitions(-DCLIENT_VERSION_BUILD=1)

# Daemon
project(qbitd)

set(SOURCES
	./secp256k1/src/secp256k1.c
	key.cpp
	uint256.cpp
	utilstrencodings.cpp
	context.cpp
	main.cpp
	./crypto/ctaes/ctaes.c
	./crypto/aes.cpp
	./crypto/ripemd160.cpp
	./crypto/sha256.cpp
	./crypto/sha512.cpp
	./crypto/hmac_sha256.cpp
	./crypto/hmac_sha512.cpp
	./base58.cpp
	./helpers/cleanse.cpp
	./allocator.cpp
	./lockedpool.cpp
	./vm/qasm.cpp
	./vm/vm.cpp
	./transaction.cpp
	./hash.cpp
	./uint256.cpp
	./transactionvalidator.cpp
	./block.cpp
	./log/log.cpp
	./db/leveldb.cpp
	./fs.cpp
	./timestamp.cpp
	./transactionactions.cpp
)

set(UNITTESTS
	./secp256k1/src/secp256k1.c
	key.cpp
	uint256.cpp
	utilstrencodings.cpp
	context.cpp
	./crypto/ctaes/ctaes.c
	./crypto/aes.cpp
	./crypto/ripemd160.cpp
	./crypto/sha256.cpp
	./crypto/sha512.cpp
	./crypto/hmac_sha256.cpp
	./crypto/hmac_sha512.cpp
	./base58.cpp
	./helpers/cleanse.cpp
	./hash/cuckatoo.cpp
	./hash/crypto/blake2b-ref.c
	./allocator.cpp
	./lockedpool.cpp
	./vm/qasm.cpp
	./vm/vm.cpp
	./transaction.cpp
	./hash.cpp
	./uint256.cpp
	./transactionvalidator.cpp
	./block.cpp
	./log/log.cpp
	./db/leveldb.cpp
	./fs.cpp
	./timestamp.cpp
	./transactionactions.cpp	

	./tests/testsuit.cpp
	./tests/unittest.cpp
	./tests/keys.cpp
	./tests/vm.cpp
	./tests/transactions.cpp
	./tests/blocks.cpp
	./tests/dbs.cpp
	./tests/cuckoo.cpp
)

add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DJM_MALLOC)

add_subdirectory(./jm)
add_subdirectory(./leveldb)

include_directories(./secp256k1/src ./secp256k1 ./secp256k1/include ./leveldb)

add_executable(${PROJECT_NAME} ${SOURCES} ${SECP256K1})
target_link_libraries(${PROJECT_NAME} jm leveldb boost_system) #gmp

add_executable(qbit-unit-tests ${UNITTESTS} ${SECP256K1})
target_link_libraries(qbit-unit-tests jm leveldb boost_system) #gmp
