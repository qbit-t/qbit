cmake_minimum_required(VERSION 2.8)

# This project can use C11, but will gracefully decay down to C89.
set(CMAKE_C_STANDARD 11)

# This project requires C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -Wno-nonnull")

# Version
add_definitions(-DQBIT_VERSION_MAJOR=0)
add_definitions(-DQBIT_VERSION_MINOR=1)
add_definitions(-DQBIT_VERSION_REVISION=6)
add_definitions(-DQBIT_VERSION_BUILD=99)

# Daemon
project(qbitd)

# Sources
set(SOURCES
	./secp256k1/src/secp256k1.c
	key.cpp
	uint256.cpp
	arith_uint256.cpp
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
	./hash/blake2/blake2b-ref.c
	./hash/cuckoo.cpp
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
	./wallet.cpp
	./mkpath.cpp
	./memorypool.cpp
	./transactionstore.cpp
	./peer.cpp
	./httpconnection.cpp
	./httpconnectionmanager.cpp
	./httpreply.cpp
	./httprequesthandler.cpp
	./httprequestparser.cpp
	./json.cpp
	./httpendpoints.cpp
	./blockcontext.cpp
	./validator.cpp
	./consensusmanager.cpp
	./dapps/buzzer/buzzer.cpp
	./dapps/buzzer/composer.cpp
	./dapps/buzzer/httpendpoints.cpp
	./dapps/buzzer/transactionstoreextension.cpp
	./dapps/buzzer/transactionactions.cpp
	./dapps/buzzer/peerextension.cpp
	./dapps/buzzer/buzzfeed.cpp
	./dapps/buzzer/eventsfeed.cpp
	./dapps/buzzer/conversationsfeed.cpp
	./itransactionstoreextension.cpp
	./seedwords.cpp
	./message.cpp
	./pow.cpp
)

set(UNITTESTS
	./secp256k1/src/secp256k1.c
	key.cpp
	uint256.cpp
	arith_uint256.cpp
	utilstrencodings.cpp
	context.cpp
	pow.cpp
	./crypto/ctaes/ctaes.c
	./crypto/aes.cpp
	./crypto/ripemd160.cpp
	./crypto/sha256.cpp
	./crypto/sha512.cpp
	./crypto/hmac_sha256.cpp
	./crypto/hmac_sha512.cpp
	./hash/blake2/blake2b-ref.c
	./hash/cuckoo.cpp
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
	./wallet.cpp
	./mkpath.cpp
	./memorypool.cpp
	./transactionstore.cpp
	./peer.cpp
	./httpconnection.cpp
	./httpconnectionmanager.cpp
	./httpreply.cpp
	./httprequesthandler.cpp
	./httprequestparser.cpp
	./json.cpp
	./httpendpoints.cpp
	./blockcontext.cpp
	./validator.cpp
	./consensusmanager.cpp
	./dapps/buzzer/buzzer.cpp
	./dapps/buzzer/composer.cpp
	./dapps/buzzer/httpendpoints.cpp
	./dapps/buzzer/transactionstoreextension.cpp
	./dapps/buzzer/transactionactions.cpp
	./dapps/buzzer/peerextension.cpp
	./dapps/buzzer/buzzfeed.cpp
	./dapps/buzzer/eventsfeed.cpp
	./dapps/buzzer/conversationsfeed.cpp
	./itransactionstoreextension.cpp
	./seedwords.cpp
	./message.cpp

	./tests/testsuit.cpp
	./tests/unittest.cpp
	./tests/keys.cpp
	./tests/vm.cpp
	./tests/transactions.cpp
	./tests/blocks.cpp
	./tests/dbs.cpp
	./tests/assets.cpp
	./tests/wallet.cpp
	./tests/memorypool.cpp
	./tests/transactionstore.cpp
	./tests/server.cpp
	./tests/httpserver.cpp
	./tests/hash.cpp
	./tests/pow.cpp
)

set(CLIENT
	./secp256k1/src/secp256k1.c
	key.cpp
	uint256.cpp
	arith_uint256.cpp
	utilstrencodings.cpp
	context.cpp
	./crypto/ctaes/ctaes.c
	./crypto/aes.cpp
	./crypto/ripemd160.cpp
	./crypto/sha256.cpp
	./crypto/sha512.cpp
	./crypto/hmac_sha256.cpp
	./crypto/hmac_sha512.cpp
	./hash/blake2/blake2b-ref.c
	./hash/cuckoo.cpp
	./base58.cpp
	./helpers/cleanse.cpp
	./allocator.cpp
	./lockedpool.cpp
	./vm/qasm.cpp
	./vm/vm.cpp
	./transaction.cpp
	./hash.cpp
	./uint256.cpp
	./block.cpp
	./log/log.cpp
	./db/leveldb.cpp
	./fs.cpp
	./timestamp.cpp
	./mkpath.cpp
	./peer.cpp
	./seedwords.cpp
	./message.cpp
	./lightwallet.cpp
	./pow.cpp
	./json.cpp
	./dapps/buzzer/buzzer.cpp
	./dapps/buzzer/buzzfeed.cpp
	./dapps/buzzer/eventsfeed.cpp
	./dapps/buzzer/conversationsfeed.cpp
	./dapps/buzzer/peerextension.cpp
	./dapps/buzzer/transactionstoreextension.cpp
	./dapps/buzzer/transactionactions.cpp
	./client/dapps/buzzer/buzzercommands.cpp
	./client/dapps/buzzer/buzzercomposer.cpp
	./client/dapps/cubix/cubixcommands.cpp
	./client/dapps/cubix/cubixcomposer.cpp
	./client/dapps/cubix/exif.cpp
	./client/commands.cpp
	./client/commandshandler.cpp
	./client/main.cpp
)

add_definitions(-DHAVE_CONFIG_H)

add_definitions(-DBUZZER_MOD)
add_definitions(-DCUBIX_MOD)
#
# uncomment and rebuild after initial deployment
add_definitions(-DPRODUCTION_MOD)

add_subdirectory(./leveldb)

#
# MinGW & Windows
if(WIN32)
	include_directories(./secp256k1/src ./secp256k1 ./secp256k1/include ./leveldb)

	add_executable(${PROJECT_NAME} ${SOURCES} ${SECP256K1})
	target_link_libraries(${PROJECT_NAME} leveldb boost_system-mt-x64 boost_thread-mt-x64 boost_chrono-mt-x64 boost_random-mt-x64 boost_filesystem-mt-x64 ws2_32 wsock32 jpeg png z) #gmp

	add_executable(qbit-cli ${CLIENT} ${SECP256K1})
	target_link_libraries(qbit-cli leveldb boost_system-mt-x64 boost_thread-mt-x64 boost_chrono-mt-x64 boost_random-mt-x64 boost_filesystem-mt-x64 ws2_32 wsock32 jpeg png z) #gmp

	#add_executable(qbit-unit-tests ${UNITTESTS} ${SECP256K1})
	#target_link_libraries(qbit-unit-tests leveldb boost_system-mt-x64 boost_thread-mt-x64 boost_chrono-mt-x64 boost_random-mt-x64 boost_filesystem-mt-x64 ws2_32 wsock32 jpeg png z) #gmp
elseif(APPLE)
	include_directories(./secp256k1/src ./secp256k1 ./secp256k1/include ./leveldb ./boost)
	link_directories(./boost/stage/lib)

	add_executable(${PROJECT_NAME} ${SOURCES} ${SECP256K1})
	target_link_libraries(${PROJECT_NAME} leveldb boost_system boost_thread boost_chrono boost_random boost_filesystem jpeg png) #gmp

	add_executable(qbit-cli ${CLIENT} ${SECP256K1})
	target_link_libraries(qbit-cli leveldb boost_system boost_thread boost_chrono boost_random boost_filesystem jpeg png) #gmp

	#add_executable(qbit-unit-tests ${UNITTESTS} ${SECP256K1})
	#target_link_libraries(qbit-unit-tests leveldb boost_system boost_thread boost_chrono boost_random boost_filesystem jpeg png) #gmp
else()
	add_subdirectory(./jm)
	add_definitions(-DJM_MALLOC)

	include_directories(./secp256k1/src ./secp256k1 ./secp256k1/include ./leveldb ./boost)
	link_directories(./boost/stage/lib)

	add_executable(${PROJECT_NAME} ${SOURCES} ${SECP256K1})
	target_link_libraries(${PROJECT_NAME} jm leveldb boost_system boost_thread boost_chrono boost_random boost_filesystem jpeg png) #gmp

	add_executable(qbit-cli ${CLIENT} ${SECP256K1})
	target_link_libraries(qbit-cli jm leveldb boost_system boost_thread boost_chrono boost_random boost_filesystem jpeg png) #gmp

	#add_executable(qbit-unit-tests ${UNITTESTS} ${SECP256K1})
	#target_link_libraries(qbit-unit-tests jm leveldb boost_system boost_thread boost_chrono boost_random boost_filesystem jpeg png) #gmp
endif()
