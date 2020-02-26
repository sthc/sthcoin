// Copyright (c) 2013-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <crypto/common.h>
#include <crypto/hmac_sha512.h>

// { + 
#include <mutex>

// #define LOG_HASH
#ifdef  LOG_HASH
#include <logging.h> // May fail at cross compilation for Windows
#endif

extern "C"
{
#include <lualib.h>
#include <lauxlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
}

static uint32_t g_hashBase[HASH_BASE_SIZE];  // Written only once at the beginning of the application

#define MAX_STMT_NUM       12
#define MIN_STMT_NUM       8
#define FUNC_COUNT     16  // Number of functions in the hashing code
#define OP_COUNT       10   // total number of operations
#define CALL_WEIGHT    2   // the larger, the more likely function calls

// } + 

inline uint32_t ROTL32(uint32_t x, int8_t r)
{
    return (x << r) | (x >> (32 - r));
}

unsigned int MurmurHash3(unsigned int nHashSeed, const std::vector<unsigned char>& vDataToHash)
{
    // The following is MurmurHash3 (x86_32), see http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
    uint32_t h1 = nHashSeed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = vDataToHash.size() / 4;

    //----------
    // body
    const uint8_t* blocks = vDataToHash.data();

    for (int i = 0; i < nblocks; ++i) {
        uint32_t k1 = ReadLE32(blocks + i*4);

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    //----------
    // tail
    const uint8_t* tail = vDataToHash.data() + nblocks * 4;

    uint32_t k1 = 0;

    switch (vDataToHash.size() & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    }

    //----------
    // finalization
    h1 ^= vDataToHash.size();
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

void BIP32Hash(const ChainCode &chainCode, unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64])
{
    unsigned char num[4];
    num[0] = (nChild >> 24) & 0xFF;
    num[1] = (nChild >> 16) & 0xFF;
    num[2] = (nChild >>  8) & 0xFF;
    num[3] = (nChild >>  0) & 0xFF;
    CHMAC_SHA512(chainCode.begin(), chainCode.size()).Write(&header, 1).Write(data, 32).Write(num, 4).Finalize(output);
}

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define SIPROUND do { \
    v0 += v1; v1 = ROTL(v1, 13); v1 ^= v0; \
    v0 = ROTL(v0, 32); \
    v2 += v3; v3 = ROTL(v3, 16); v3 ^= v2; \
    v0 += v3; v3 = ROTL(v3, 21); v3 ^= v0; \
    v2 += v1; v1 = ROTL(v1, 17); v1 ^= v2; \
    v2 = ROTL(v2, 32); \
} while (0)

CSipHasher::CSipHasher(uint64_t k0, uint64_t k1)
{
    v[0] = 0x736f6d6570736575ULL ^ k0;
    v[1] = 0x646f72616e646f6dULL ^ k1;
    v[2] = 0x6c7967656e657261ULL ^ k0;
    v[3] = 0x7465646279746573ULL ^ k1;
    count = 0;
    tmp = 0;
}

CSipHasher& CSipHasher::Write(uint64_t data)
{
    uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];

    assert(count % 8 == 0);

    v3 ^= data;
    SIPROUND;
    SIPROUND;
    v0 ^= data;

    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;

    count += 8;
    return *this;
}

CSipHasher& CSipHasher::Write(const unsigned char* data, size_t size)
{
    uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];
    uint64_t t = tmp;
    int c = count;

    while (size--) {
        t |= ((uint64_t)(*(data++))) << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            v3 ^= t;
            SIPROUND;
            SIPROUND;
            v0 ^= t;
            t = 0;
        }
    }

    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
    count = c;
    tmp = t;

    return *this;
}

uint64_t CSipHasher::Finalize() const
{
    uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];

    uint64_t t = tmp | (((uint64_t)count) << 56);

    v3 ^= t;
    SIPROUND;
    SIPROUND;
    v0 ^= t;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}

uint64_t SipHashUint256(uint64_t k0, uint64_t k1, const uint256& val)
{
    /* Specialized implementation for efficiency */
    uint64_t d = val.GetUint64(0);

    uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
    uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
    uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
    uint64_t v3 = 0x7465646279746573ULL ^ k1 ^ d;

    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    v3 ^= ((uint64_t)4) << 59;
    SIPROUND;
    SIPROUND;
    v0 ^= ((uint64_t)4) << 59;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}

uint64_t SipHashUint256Extra(uint64_t k0, uint64_t k1, const uint256& val, uint32_t extra)
{
    /* Specialized implementation for efficiency */
    uint64_t d = val.GetUint64(0);

    uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
    uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
    uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
    uint64_t v3 = 0x7465646279746573ULL ^ k1 ^ d;

    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = (((uint64_t)36) << 56) | extra;
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}


// { + 
static inline uint32_t GetUint32FromHashBase (uint32_t i)
{
  i = i % HASH_BASE_SIZE_IN_BYTES;
  return ((uint32_t) ((uint8_t * ) g_hashBase)[i])
         | (((uint32_t) ((uint8_t * ) g_hashBase)[(i + 1) % HASH_BASE_SIZE_IN_BYTES]) << 8)
         | (((uint32_t) ((uint8_t * ) g_hashBase)[(i + 2) % HASH_BASE_SIZE_IN_BYTES]) << 16)
         | (((uint32_t) ((uint8_t * ) g_hashBase)[(i + 3) % HASH_BASE_SIZE_IN_BYTES]) << 24);
}


static int NotShift (lua_State *L)
{
  // Step 1:  extract the parameters from the lua stack:
  uint32_t x =  (uint32_t) lua_tonumber(L,1);
  uint32_t y =  (uint32_t) lua_tonumber(L,2);
  uint32_t m =  (uint32_t) lua_tonumber(L,3);
  uint32_t n =  (uint32_t) lua_tonumber(L,4);

  //  Step 2:  Do the actual calculation.  Normally, this will be more interesting than a single sum!
  x = ~ (x + m);
  x = (((x >> 3) | (x << 29)) - m);
  y = ~ (y - n);
  y = ((y >> 19) | (y << 13)) + n;

  x = x ^ GetUint32FromHashBase(x);
  y = y ^ GetUint32FromHashBase(y);

  // Step 3:  Push the result on the lua stack.
  lua_pushnumber(L,(double) x);
  lua_pushnumber(L,(double) y);

  // Return the number of arguments we pushed onto the stack (that is, the number of return values this
  // function has
  return 2;
}


static int AndXorOr (lua_State *L)
{
  // Step 1:  extract the parameters from the lua stack:
  uint32_t m =  (uint32_t) lua_tonumber(L,3);
  uint32_t n =  (uint32_t) lua_tonumber(L,4);
  uint32_t x =  ((uint32_t) lua_tonumber(L,1)) - m;
  uint32_t y =  ((uint32_t) lua_tonumber(L,2)) + n;

  x = x ^ GetUint32FromHashBase(x);
  y = y ^ GetUint32FromHashBase(y);

  //  Step 2:  Do the actual calculation.  Normally, this will be more interesting than a single sum!
  uint32_t a, b, c, d;
  a = (x >> 16) & 0x0000FFFF;
  b = x & 0x0000FFFF;
  c = (y >> 16) & 0x0000FFFF;
  d = y & 0x0000FFFF;

  x = ((((a & c) ^ (~ a & d)) << 16) | ((b & c) ^ (~ b & d))) + m;
  y = ((((c & a) ^ (~ c & b)) << 16) | ((d & a) ^ (~ d & b))) - n;

  // Step 3:  Push the result on the lua stack.
  lua_pushnumber(L,(double) x);
  lua_pushnumber(L,(double) y);

  // Return the number of arguments we pushed onto the stack (that is, the number of return values this
  // function has
  return 2;
}


static int AndXor (lua_State *L)
{
  // Step 1:  extract the parameters from the lua stack:
  uint32_t m =  (uint32_t) lua_tonumber(L,3);
  uint32_t n =  (uint32_t) lua_tonumber(L,4);
  uint32_t x =  ((uint32_t) lua_tonumber(L,1)) - m;
  uint32_t y =  ((uint32_t) lua_tonumber(L,2)) + n;

  //  Step 2:  Do the actual calculation.  Normally, this will be more interesting than a single sum!
  uint32_t a, b, c, d;
  a = x & 0x0000FFFF;
  b = (x >> 16) & 0x0000FFFF;
  c = y & 0x0000FFFF;
  d = (y >> 16) & 0x0000FFFF;

  x = ((((a & b) ^ (a & c) ^ (b & c)) << 16) |  ((b & c) ^ (b & d) ^ (c & d))) - m;
  y = ((((c & d) ^ (c & a) ^ (d & a)) << 16) |  ((d & a) ^ (d & b) ^ (a & b))) + n;

  x = x ^ GetUint32FromHashBase(x);
  y = y ^ GetUint32FromHashBase(y);

  // Step 3:  Push the result on the lua stack.
  lua_pushnumber(L,(double) x);
  lua_pushnumber(L,(double) y);

  // Return the number of arguments we pushed onto the stack (that is, the number of return values this
  // function has
  return 2;
}

static int ShiftXor (lua_State *L)
{
  // Step 1:  extract the parameters from the lua stack:
  uint32_t m =  (uint32_t) lua_tonumber(L,3);
  uint32_t n =  (uint32_t) lua_tonumber(L,4);
  uint32_t x =  ((uint32_t) lua_tonumber(L,1)) - m;
  uint32_t y =  ((uint32_t) lua_tonumber(L,2)) + n;

  x = x ^ GetUint32FromHashBase(x);
  y = y ^ GetUint32FromHashBase(y);

  //  Step 2:  Do the actual calculation.  Normally, this will be more interesting than a single sum!
  x = (((x >> 2) | (x << 30)) ^ ((x >> 13) | (x << 19)) ^ ((x >> 22) | (x << 10))) + m;
  y = (((y >> 6) | (y << 26)) ^ ((y >> 11) | (y << 21)) ^ ((y >> 25) | (y << 7))) - n;

  // Step 3:  Push the result on the lua stack.
  lua_pushnumber(L,(double) x);
  lua_pushnumber(L,(double) y);

  // Return the number of arguments we pushed onto the stack (that is, the number of return values this
  // function has
  return 2;
}



static int ShiftMix16 (lua_State *L)
{
  // Step 1:  extract the parameters from the lua stack:
  uint32_t x, y;
  uint64_t n;
  uint32_t u =  (uint32_t) lua_tonumber(L,3);
  uint32_t v =  (uint32_t) lua_tonumber(L,4);
  uint64_t a = ((uint64_t) lua_tonumber(L,1)) + (uint64_t) u;
  uint64_t b = ((uint64_t) lua_tonumber(L,2)) - (uint64_t) v;

  //  Step 2:  Do the actual calculation.  Normally, this will be more interesting than a single sum!
  n = (a << 32) | b;
  n = ((n >> 7) | (n << 57)) ^ ((n >> 47) | (n << 17)) ^ ((n >> 53) | (n << 11));
  x = ((uint32_t) ((n & 0x0000FFFF) + ((n >> 16) & 0xFFFF0000))) - u;
  y = ((uint32_t) (((n >> 16) & 0x0000FFFF) + ((n >> 32) & 0xFFFF0000))) + v;

  // Step 3:  Push the result on the lua stack.
  lua_pushnumber(L,(double) x);
  lua_pushnumber(L,(double) y);

  // Return the number of arguments we pushed onto the stack (that is, the number of return values this
  // function has
  return 2;
}


static int ShiftMix8 (lua_State *L)
{
  // Step 1:  extract the parameters from the lua stack:
  uint32_t x, y;
  uint64_t n;
  uint32_t u =  (uint32_t) lua_tonumber(L,3);
  uint32_t v =  (uint32_t) lua_tonumber(L,4);
  uint64_t a = ((uint64_t) lua_tonumber(L,1)) - (uint64_t) u;
  uint64_t b = ((uint64_t) lua_tonumber(L,2)) + (uint64_t) v;

  //  Step 2:  Do the actual calculation.  Normally, this will be more interesting than a single sum!
  n = (b << 32) | a;
  n = ((n >> 19) | (n << 45)) ^ ((n >> 29) | (n << 35)) ^ ((n >> 59) | (n << 5));
  x = ((uint32_t) (((n & 0x000000FF) << 24) | ((n & 0x00FF0000) >> 8) | ((n >> 16) & 0x00FF0000) | ((n >> 48) & 0x000000FF))) + u;
  y = ((uint32_t) (((n & 0x0000FF00) << 16) | ((n & 0xFF000000) >> 8) | ((n >> 32) & 0x0000FF00) | (n >> 56))) - v;

  // Step 3:  Push the result on the lua stack.
  lua_pushnumber(L,(double) x);
  lua_pushnumber(L,(double) y);

  // Return the number of arguments we pushed onto the stack (that is, the number of return values this
  // function has
  return 2;
}


static int SwapShift (lua_State *L)
{
  // Step 1:  extract the parameters from the lua stack:
  uint32_t s =  (uint32_t) lua_tonumber(L,3);
  uint32_t t =  (uint32_t) lua_tonumber(L,4);
  uint32_t a =  (uint32_t) lua_tonumber(L,1) + s;
  uint32_t b =  (uint32_t) lua_tonumber(L,2) - t;
  uint32_t x, y, n;

  //  Step 2:  Do the actual calculation.  Normally, this will be more interesting than a single sum!
  uint32_t c = a % 43;
  uint32_t d = b % 41;

  if (c == 0) {
    n = d;
  }
  else if (d == 0) {
    n = c;
  }
  else {
    n = (c * d) % 37;
  }
  n = (n & 0x0000001F) + 1;

  uint32_t u = 0xFFFFFFFF;
  uint32_t v;
  if (n == 32) u = 0;
  else u >>= n;
  v = ~ u;

  c = 16 - ((b % 43) & 0x0000000F);
  d = ((a % 41) & 0x0000000F) + 1;

  x = ((a & u) | (b & v));
  x = ((x >> c) | (x << (32 - c))) - s;
  y = ((a & v) | (b & u));
  y = ((y << d) | (y >> (32 - d))) + t;

  // Step 3:  Push the result on the lua stack.
  lua_pushnumber(L,(double) x);
  lua_pushnumber(L,(double) y);

  // Return the number of arguments we pushed onto the stack (that is, the number of return values this
  // function has
  return 2;
}


static int PrimeMix (lua_State *L)
{
  // Step 1:  extract the parameters from the lua stack:
  uint32_t x, y;
  uint64_t n;
  uint32_t u =  (uint32_t) lua_tonumber(L,3);
  uint32_t v =  (uint32_t) lua_tonumber(L,4);
  uint64_t a = ((uint64_t) lua_tonumber(L,1)) - (uint64_t) u;
  uint64_t b = ((uint64_t) lua_tonumber(L,2)) + (uint64_t) v;

  //  Step 2:  Do the actual calculation.  Normally, this will be more interesting than a single sum!
  n = (a << 32) | b;
  n = ((n >> 6) | (n << 58)) ^ ((n >> 47) | (n << 17)) ^ ((n >> 54) | (n << 10));

  x = ((uint32_t) (n % 257 + n / 65537 % 257 * 257 + n / 4294967291 % 257 * 65537 + n / 4294967291 / 65537 % 257 * 65537 * 257)) + u;
  n /= 257;
  y = ((uint32_t) (n % 257 + n / 65537 % 257 * 257 + n / 4294967291 % 257 * 65537 + n / 4294967291 / 65537 % 257 * 65537 * 257)) - v;

  // Step 3:  Push the result on the lua stack.
  lua_pushnumber(L,(double) x);
  lua_pushnumber(L,(double) y);

  // Return the number of arguments we pushed onto the stack (that is, the number of return values this
  // function has
  return 2;
}


static int PrimeMix2 (lua_State *L)
{
  // Step 1:  extract the parameters from the lua stack:
  uint32_t x, y;
  uint64_t n;
  uint32_t u =  (uint32_t) lua_tonumber(L,3);
  uint32_t v =  (uint32_t) lua_tonumber(L,4);
  uint64_t a = ((uint64_t) lua_tonumber(L,1)) + (uint64_t) u;
  uint64_t b = ((uint64_t) lua_tonumber(L,2)) - (uint64_t) v;

  //  Step 2:  Do the actual calculation.  Normally, this will be more interesting than a single sum!
  n = (a << 32) | b;
  n = ((n >> 8) | (n << 56)) ^ ((n >> 43) | (n << 21)) ^ ((n >> 52) | (n << 12));

  x = ((uint32_t) (n % 251 * 65539 * 251 + n / 65539 % 251 * 65539 + n / 4294967279 % 251 * 251 + n / 4294967279 / 65539 % 251)) - u;
  n = n / 251;
  y = ((uint32_t) (n % 251 * 65539 * 251 + n / 65539 % 251 * 65539 + n / 4294967279 % 251 * 251 + n / 4294967279 / 65539 % 251)) + v;

  // Step 3:  Push the result on the lua stack.
  lua_pushnumber(L,(double) x);
  lua_pushnumber(L,(double) y);

  // Return the number of arguments we pushed onto the stack (that is, the number of return values this
  // function has
  return 2;
}


void MakeHashCode (uint64_t seed, uint64_t incr, char * code)
{
  char buf[1000];
  char f[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'};
  char call[1000];
  sprintf (call, "\
                    if (r > 0) then \n\
                         if (x %% 23 < 12 and y %% 29 > 14) then\n\
                           z = ((x %% 71) + (y %% 19)) %% %u\n\
                         else\n\
                           z = ((x %% 23) + (y %% 67)) %% %u\n\
                         end\n\
                         y, x = f[z] (x, y, r - 1)\n\
                    end\n", FUNC_COUNT, FUNC_COUNT);

  CPCG32 pcg32 (seed, incr);

  int i, j, n;
  sprintf (code, "f = {}\n");
  strcat (code, "p = {145403341,66068741,2749919,27290089,34185863,37667459,95188969,13833949,67867831,71479897,78736303,55316783,162373177,141650737,149163137,82375961,22182247,126673831,23879353,12195067,108092819,109938481,18815059,60677941,41161511,171834121,177525619,143522779,160481023,62472941,80556551,20495749,10570697,98866763,69672541,25582019,53533379,32452657,84200113,48210583,30723547,75103313,113648273,179424551,91518881,147280787,97026073,46441099,121086289,168048611,7368631,137896123,64268657,8960299,139772119,76918057,122949667,87857347,130408657,104395003,158594087,166158541,29005411,5799961,73289599,154819559,134150869,128541643,106244773,102551369,175628303,117363863,169941001,164262793,111794677,100711231,58885829,93354587,1299553,132276563,57099149,115507703,152935751,15485761,136023631,49979591,39410737,44680193,119226883,86027987,173729729,51754847,156703873,124811003,42919973,89687537,35926171}\n");
  for (i = 0; i < FUNC_COUNT; ++ i) {
    sprintf (buf, "f[%u] = function (x, y, r)\nlocal z\n", i);
    strcat (code, buf);
    n = pcg32.randint(MIN_STMT_NUM, MAX_STMT_NUM);
    for (j = 0; j < n; ++ j) {
      int k = pcg32.pcg32 () % (OP_COUNT + CALL_WEIGHT);
      if (k <= OP_COUNT - 2) {
        sprintf(buf, "y, x = %c (x, y, p[y %% 97 + 1], p[(x + 48) %% 97 + 1])\n", f[k]);
        strcat(code, buf);
        continue;
      }
      strcat (code, call);
    }
    strcat (code, "return y, x\nend\n\n");
  }
}


void InitHashBase () {
  CPCG32 pcg32 (599128178199824553ull, 2055286011627441373ull);

  uint32_t i;

  for (i = 0; i < HASH_BASE_SIZE; ++ i) {
    uint8_t * p = (uint8_t *) &(g_hashBase[i]);
    uint32_t r = pcg32.pcg32();
    p[0] = (uint8_t) (r & 0x000000FF);
    p[1] = (uint8_t) ((r >> 8) & 0x000000FF);
    p[2] = (uint8_t) ((r >> 16) & 0x000000FF);
    p[3] = (uint8_t) ((r >> 24) & 0x000000FF);
  }
}


// Get the 64-bit number from bits starting at byte of index i
inline uint64_t GetUint64 (uint8_t * bits, uint32_t i)
{
  uint64_t x = ((uint64_t) bits[i % 32]) << 56;
  x |= ((uint64_t) bits[(i + 1) % 32]) << 48;
  x |= ((uint64_t) bits[(i + 2) % 32]) << 40;
  x |= ((uint64_t) bits[(i + 3) % 32]) << 32;
  x |= ((uint64_t) bits[(i + 4) % 32]) << 24;
  x |= ((uint64_t) bits[(i + 5) % 32]) << 16;
  x |= ((uint64_t) bits[(i + 6) % 32]) << 8;
  x |= (uint64_t) bits[(i + 7) % 32];

  return x;
}


void ShuffleHash256 (lua_State * L, uint8_t * hash)
{
  const uint32_t byteNum = 32;

  for (uint32_t i = 0; i < 2; ++ i) {
    for (uint32_t m = 0; m < byteNum; m += 4) {
      uint32_t x, y, n, k, f, r, d;
      n = (m + 4) % byteNum;
      k = (n + 4) % byteNum;

      x = (((uint32_t) hash[m]) << 24) + (((uint32_t) hash[(m + 1) % byteNum]) << 16)
          + (((uint32_t) hash[(m + 2) % byteNum]) << 8) + (uint32_t) hash[(m + 3) % byteNum];
      y = (((uint32_t) hash[(n + 3) % byteNum]) << 24) + (((uint32_t) hash[(n + 2) % byteNum]) << 16)
          + (((uint32_t) hash[(n + 1) % byteNum]) << 8) + (uint32_t) hash[n];
      f = (hash[k] >> 4) & 0x0F;
      r = ((hash[k] >> 2) & 0x03) + 1;
      d = (hash[k] & 0x03) + 2;

      for (uint32_t k = 0; k < r; ++k) {
        lua_getglobal(L, "f");
        lua_pushnumber(L, f);
        lua_gettable(L, -2);
        lua_remove(L, -2);
        lua_pushnumber(L, x);
        lua_pushnumber(L, y);
        lua_pushnumber(L, d);
        if (lua_pcall(L, 3, 2, 0) == 0) {
          x = lua_tointeger(L, -2);
          y = lua_tointeger(L, -1);
          lua_pop(L, 2);
        }
        else {
#ifdef  LOG_HASH
          LogPrintf ("LUA error in ShuffleHash256: %s\n", lua_tostring(L, -1));
#endif
          lua_pop(L, 1);
        }
      }

      hash[m] = (uint8_t)((x >> 24) & 0x000000FF);
      hash[(m + 1) % byteNum] = (uint8_t)((x >> 16) & 0x000000FF);
      hash[(m + 2) % byteNum] = (uint8_t)((x >> 8) & 0x000000FF);
      hash[(m + 3) % byteNum] = (uint8_t)(x & 0x000000FF);
      hash[(n + 3) % byteNum] = (uint8_t)((y >> 24) & 0x000000FF);
      hash[(n + 2) % byteNum] = (uint8_t)((y >> 16) & 0x000000FF);
      hash[(n + 1) % byteNum] = (uint8_t)((y >> 8) & 0x000000FF);
      hash[n] = (uint8_t)(y & 0x000000FF);
    }
  }
}


uint256 DowsHash(uint256 result, bool debug)
{
#ifdef  LOG_HASH
  if (debug) {
        LogPrint(BCLog::HASH, "Plain hash = %s\n", result.ToString());
    }
#endif

  uint8_t h[32];
  char code[100000];
  int i;

  memcpy (h, result.begin(), 32);
  lua_State* L = luaL_newstate();

  // load Lua base libraries (print / math / etc)
  luaL_openlibs(L);
  lua_register(L, "A", NotShift);
  lua_register(L, "B", AndXor);
  lua_register(L, "C", AndXorOr);
  lua_register(L, "D", ShiftMix8);
  lua_register(L, "E", ShiftMix16);
  lua_register(L, "F", ShiftXor);
  lua_register(L, "G", SwapShift);
  lua_register(L, "H", PrimeMix);
  lua_register(L, "I", PrimeMix2);


  uint64_t seed = 0, incr = 0;
  for (i = 0; i < 32; i += 4) {
    seed += GetUint64(h, i);
    incr += GetUint64(h, 31 - i);
    CPCG32 pcg32 (seed, incr);
    seed += (uint64_t) pcg32.pcg32();
  }

  MakeHashCode(seed, incr, code);
  luaL_loadstring(L, code);
  if (lua_pcall (L, 0, 0, 0)) {
#ifdef  LOG_HASH
    LogPrintf ("LUA error: %s\n", lua_tostring(L, -1));
#endif
    lua_pop(L, 1);
  }

  ShuffleHash256(L, (uint8_t *) h);
  lua_close (L);
  CHash256().Write (result.begin(), 32).Write (h, 32).Finalize(result.begin());
  memcpy (h, result.begin(), 32);
  for (i = 0; i < 32; i += 4) {
    seed += GetUint64(h, i);
    incr += GetUint64(h, 31 - i);
    CPCG32 pcg32 (seed, incr);
    seed += (uint64_t) pcg32.pcg32();
  }

  CHash256 & h256 = CHash256().Write (result.begin(), 32);
  CPCG32 pcg32 (seed, incr);
  for (i = 0; i < HASH_BASE_USE_COUNT; ++ i) {
    uint32_t n = pcg32.pcg32() % HASH_BASE_SIZE_IN_BYTES;

    if (n <= HASH_BASE_SIZE_IN_BYTES - 32) {
      h256.Write(((uint8_t *) g_hashBase) + n, 32);
      continue;
    }

    uint8_t b[32];
    for (uint32_t j = 0; j < 32; ++ j) {
      b[j] = ((uint8_t *) g_hashBase)[(n + j) % HASH_BASE_SIZE_IN_BYTES];
    }
    h256.Write(b, 32);
  }
  h256.Finalize(result.begin());

#ifdef  LOG_HASH
  if (debug) {
        uint256 check;
        CHash256().Write ((unsigned char *) code, strlen (code)).Finalize(check.begin());
        LogPrint(BCLog::HASH, "Code hash = %s\n", check.ToString());
        LogPrint(BCLog::HASH, "LUA hash = %s\n", result.ToString());
    }
#endif
  return result;
}

// } + 