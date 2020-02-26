// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <logging.h>

// & 

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    int nHeightFirst;
    const CBlockIndex* pindexFirst;
    arith_uint256 diffBits;

    // { + 
    if (pindexLast->nHeight < params.DifficultyAdjustmentInterval()) {
        return nProofOfWorkLimit;
    }
    // } + 

    // Only change once per difficulty adjustment interval
    if (pindexLast->nHeight % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }

        // { + 
        // Go back by what we want to be 6 hours worth of blocks
        int64_t blockTime;
        if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + (params.nPowTargetSpacing >> 1)) {
            nHeightFirst = pindexLast->nHeight - params.DifficultyAdjustmentInterval() + 1;
            blockTime = pblock->GetBlockTime();
        }
        else {
            nHeightFirst = pindexLast->nHeight - params.DifficultyAdjustmentInterval();
            blockTime = pindexLast->GetBlockTime();
        }
        assert(nHeightFirst >= 0);
        pindexFirst = pindexLast->GetAncestorAndAverageDifficulty(nHeightFirst, diffBits);
        assert(pindexFirst);

        // Adjust difficulty if having been waiting for too long for one block to be generated
        if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 5) {
            LogPrint (BCLog::DIFFICULTY, "Adjusting difficulty - too hard\n");
            return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), blockTime, diffBits, params);
        }

        int32_t d = pblock->GetBlockTime() - pindexFirst->GetBlockTime();
        if (d - params.nPowTargetTimespan > (params.nPowTargetTimespan >> 2)) {
            LogPrint (BCLog::DIFFICULTY, "Adjusting difficulty - too hard on average\n");
            return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), blockTime, diffBits, params);
        }

        if ( d < ((params.nPowTargetTimespan >> 2) * 3)) {
            LogPrint (BCLog::DIFFICULTY, "Adjusting difficulty - too easy\n");
            return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), blockTime, diffBits, params);
        }
        // } + 
        return pindexLast->nBits;
    }

    // Go back by what we want to be 6 hours worth of blocks
    nHeightFirst = pindexLast->nHeight - params.DifficultyAdjustmentInterval(); // & 
    assert(nHeightFirst >= 0);
    pindexFirst = pindexLast->GetAncestorAndAverageDifficulty(nHeightFirst, diffBits); // & 
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), pindexLast->GetBlockTime(), diffBits, params); // & 
}

// ( & 
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, int64_t nCurrentBlockTime,
                                       arith_uint256 & bnNew, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = nCurrentBlockTime - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew2 = bnNew;

    LogPrint (BCLog::DIFFICULTY,"Old target: %s, target time span: %llu\n", bnNew.ToString(), params.nPowTargetTimespan);
    bnNew /= params.nPowTargetTimespan;
    LogPrint (BCLog::DIFFICULTY, "Intermed. target: %s, actual time span: %llu\n", bnNew.ToString(), nActualTimespan);
    bnNew *= nActualTimespan;
    if (bnNew == arith_uint256 (0)) {
        bnNew = bnNew2;
        bnNew *= nActualTimespan;
        bnNew /= params.nPowTargetTimespan;
    }
    LogPrint (BCLog::DIFFICULTY, "New target: %s\n", bnNew.ToString());

    if (bnNew > bnPowLimit) {
        bnNew = bnPowLimit;
        LogPrint (BCLog::DIFFICULTY, "Adjusted new target: %s\n", bnNew.ToString());
    }

    return bnNew.GetCompact();
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params) {
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    return CalculateNextWorkRequired(pindexLast, nFirstBlockTime, pindexLast->GetBlockTime(), bnNew, params);
}
// & 

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
