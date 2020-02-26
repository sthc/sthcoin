// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STHCOIN_POW_H
#define STHCOIN_POW_H

#include <consensus/params.h>

#include <stdint.h>
#include <arith_uint256.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&); // Keep for test cases
// + 
unsigned int CalculateNextWorkRequired
    (const CBlockIndex* pindexLast, int64_t nFirstBlockTime, int64_t nCurrentBlockTime, arith_uint256 & diffBits, const Consensus::Params&);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);

#endif // STHCOIN_POW_H
