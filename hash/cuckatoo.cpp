// Cuck(at)oo Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2019 John Tromp

#include "cuckatoo.h"
#include <inttypes.h> // for SCNx64 macro
#include <stdio.h>    // printf/scanf
#include <stdlib.h>   // exit
#include <unistd.h>   // getopt
#include <assert.h>   // d'uh
#include <sstream>

const char *errstr[] = { "OK", "wrong header length", "edge too big", "edges not ascending", "endpoints don't match up", "branch in cycle", "cycle dead ends", "cycle too short"};

// arbitrary length of header hashed into siphash key
#define HEADERLEN 80

int main_verify(int argc, char **argv, char* input) {
  const char *header = "";
  int nonce = 68;
  int c;
  while ((c = getopt (argc, argv, "h:n:")) != -1) {
    switch (c) {
      case 'h':
        header = optarg;
        break;
      case 'n':
        nonce = atoi(optarg);
        break;
    }
  }
  char headernonce[HEADERLEN];
  u32 hdrlen = strlen(header);
  assert(hdrlen <= HEADERLEN);
  memcpy(headernonce, header, hdrlen);
  memset(headernonce+hdrlen, 0, sizeof(headernonce)-hdrlen);
  ((u32 *)headernonce)[HEADERLEN/sizeof(u32)-1] = htole32(nonce);
  siphash_keys keys;
  setheader(headernonce, sizeof(headernonce), &keys);
  printf("nonce %d k0 k1 k2 k3 %llx %llx %llx %llx\n", nonce, keys.k0, keys.k1, keys.k2, keys.k3);
  printf("Verifying size %d proof for cuckatoo%d(\"%s\",%d)\n",
               PROOFSIZE, EDGEBITS, header, nonce);
  std::stringstream ssin(input);
  word_t nonces[PROOFSIZE];
  for(int i = 0; i < 43; i++)
  {
    std::string data;
    ssin >> data;
    if(i == 0) continue;
    uint64_t nonce;
    int nscan = sscanf(data.c_str(), " %" SCNx64, &nonce);
    assert(nscan == 1);
    nonces[i-1] = nonce;
    printf("'%s' %d\n", data.c_str(), nonce);
  }
  int pow_rc = verify(nonces, &keys);
  if (pow_rc == POW_OK) {
    printf("Verified with cyclehash ");
    unsigned char cyclehash[32];
    blake2b((void *)cyclehash, sizeof(cyclehash), (const void *)nonces, sizeof(nonces), 0, 0);
    for (int i=0; i<32; i++)
      printf("%02x", cyclehash[i]);
    printf("\n");
  } else {
    printf("FAILED due to %s\n", errstr[pow_rc]);
  }
  // for (int nsols=0; scanf(" Solution") == 0; nsols++) {
  //   word_t nonces[PROOFSIZE];
  //   for (int n = 0; n < PROOFSIZE; n++) {
  //     uint64_t nonce;
  //     int nscan = scanf(" %" SCNx64, &nonce);
  //     assert(nscan == 1);
  //     nonces[n] = nonce;
  //   }
  //   int pow_rc = verify(nonces, &keys);
  //   if (pow_rc == POW_OK) {
  //     printf("Verified with cyclehash ");
  //     unsigned char cyclehash[32];
  //     blake2b((void *)cyclehash, sizeof(cyclehash), (const void *)nonces, sizeof(nonces), 0, 0);
  //     for (int i=0; i<32; i++)
  //       printf("%02x", cyclehash[i]);
  //     printf("\n");
  //   } else {
  //     printf("FAILED due to %s\n", errstr[pow_rc]);
  //   }
  // }
  return 0;
}
