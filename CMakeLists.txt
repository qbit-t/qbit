cmake_minimum_required(VERSION 2.8)

project(qbit-test)

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
	./base58.cpp
	./helpers/cleanse.cpp
	./allocator.cpp
)

add_definitions(-DHAVE_CONFIG_H)

add_subdirectory(./jm)

include_directories(./secp256k1/src ./secp256k1 ./secp256k1/include)

add_executable(${PROJECT_NAME} ${SOURCES} ${SECP256K1})
target_link_libraries(${PROJECT_NAME} jm)
