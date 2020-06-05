// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>

#include <tinyformat.h>
#include <util.h>
#include <utilstrencodings.h>

#include <assert.h>

#include <chainparamsseeds.h>

#include <arith_uint256.h>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";

        consensus.nSubsidyHalvingInterval = 92160; // every 128 days
        consensus.BIP16Exception = uint256S("0x0");
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0");
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.powLimit = uint256S("000fffff00000000000000000000000000000000000000000000000000000000");
        consensus.nPowTargetTimespan = 6 * 60 * 60; // 6 hours
        consensus.nPowTargetSpacing = 2 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 171; // 95% of 2016
        consensus.nMinerConfirmationWindow = 180; // nPowTargetTimespan / nPowTargetSpacing

        /* 
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256S("0x00000000000002dc756eebf4f49723ed8d30cc28a5f108eb94b1ba88ac4f9c22");
        consensus.BIP34Height = 227931;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.BIP65Height = 388381; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 363725; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
         */

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1557171322; // May 6, 2019 // 
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1620243322; // 2 years later

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1557171322; // January ?, 2019 // 
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1620243322; // 2 years later

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        // Refere to AbstractThresholdConditionChecker::GetStateFor in validation.cpp for Consensus::BIP9Deployment::ALWAYS_ACTIVE
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE; // 
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        //  consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000028822fef1c230963535a90d");
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000fff");

        // By default assume that the signatures in ancestors of this block are valid.
        //  consensus.defaultAssumeValid = uint256S("0x0000000000000000002e63058c023a9a1de233554f28c7b21380b6c9003f36a8"); //534292
        consensus.defaultAssumeValid = uint256S("0x0");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        /* 
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xd9;
        nDefaultPort = 8333;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1231006505, 2083236893, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));
        */

        pchMessageStart[0] = 0xfb;
        pchMessageStart[1] = 0xbb;
        pchMessageStart[2] = 0xcc;
        pchMessageStart[3] = 0xd8;
        nDefaultPort = 9099;
        nPruneAfterHeight = 100000;

        //  {
        /*
        uint32_t i = 0;
        arith_uint256 pol = UintToArith256 (consensus.powLimit);
        do {
            genesis = CreateGenesisBlock(1557171322, i, 0x1f0fffff, 1, 20000 * COIN);
            ++ i;
            consensus.hashGenesisBlock = genesis.GetHash();
            // std::cout << consensus.hashGenesisBlock.ToString () << "\n";
            if (i % 1000 == 0) printf ("Checking nonce %u ...\n", i);
            // std::cout << UintToArith256 (consensus.hashGenesisBlock).ToString() << "\n";
            // std::cout << pol.ToString() << "\n";
        } while (UintToArith256 (consensus.hashGenesisBlock) > pol);
        printf ("Nonce = %u\n", i - 1);
        */
        //  }

        genesis = CreateGenesisBlock(1557171322, 618, 0x1f0fffff, 1, 20000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        // std::cout << "genesis block hash = " << consensus.hashGenesisBlock.ToString () << "\n";
        // std::cout << "genesis block merkle root = " << genesis.hashMerkleRoot.ToString () << "\n";
        assert(consensus.hashGenesisBlock == uint256S("0x0000b277bd61e047d5f32fbb93839be8ef2b5927443665cfa32ba5033e431c67"));
        assert(genesis.hashMerkleRoot == uint256S("0x1961c39fd7ce4c2ca210f39d71a91d723506bc052d6e408e329bf1613cc6931d"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        // vSeeds.emplace_back("seed.freecoins.city"); // Pieter Wuille, only supports x1, x5, x9, and xd

        /* 
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
         */

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,100);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,105);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,228);
        base58Prefixes[EXT_PUBLIC_KEY] = {0xA4, 0xB8, 0xAD, 0xE4};
        base58Prefixes[EXT_SECRET_KEY] = {0xA4, 0xB8, 0xB2, 0x1E};

        bech32_hrp = "bc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));
        vSeeds.emplace_back("seed.soothing.center");

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = {
            {
                {0, uint256S("0x0000b277bd61e047d5f32fbb93839be8ef2b5927443665cfa32ba5033e431c67")},
                {100, uint256S("0x00043a3c99a755162b227e94796dfb2a0a961e9b7235e880f970f5246f8e6a3c")},
                {200, uint256S("0x00036e030bd0ac1f9c041afa9bf8ffbc66580dfef7642e298de1f8ef29e8d86e")},
                {500, uint256S("0x00042671d087f0389c2b22a727e5de065cad1952ed38c5a556fad7d78c4d190d")},
                {1000, uint256S("0x0000dfb4d6d09a5f4852090547e123051f1a7d439469e05dbef81b035100229a")},
                {1500, uint256S("0x00041fd718e599300b3d128166ff26ee516905cf9f34dbd2216ed64be10b274e")},
                {3000, uint256S("0x0003f73b50527b3575b67376b001bcb297ae787d1a785f43a7950f72b0e494b5")},
                {5000, uint256S("0x0001c574ab405f9f1082c5058f6f9c18073776c9b698b7e51b7956765be134a2")},
                {10000, uint256S("0x0006b6158a0d61a14a2a5869b9bd7ecae36329d0c34951c70ad5cb39b266a57f")},
                {14900, uint256S("0x00002a4783041889d72a0d62bbedcf460560d3cf5298a68af4e3cf9ac88c6997")},
                {20000, uint256S("0x00017208be74aa817e2f5c67de53157fb6a603ab959bc976fe88f0605ce21660")},
                {30000, uint256S("0x0001d79a25191f995bf3d122f1c55bad27567db50dc6b46e1c94eec32f2dc917")},
                {40000, uint256S("0x00001931bfc3934e51f1f02447adc12b63a08981db035dcdd3ccafc6afddd8c0")},
                {50000, uint256S("0x00013b1092102b3d90f084d061b4e24bdf179abcc76370c4e067bfce4e5dc984")},
                {100000, uint256S("0x0002ce49316904584e654983851805b65724f29814ddc4afa9aea649c44465fc")},
                {200000, uint256S("0x000004bc653af293284ce8b4d588f4f8d2c4d46f0ec30622c49c260e8dc128a5")}
            }
        };

        /* 
        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 0000000000000000002e63058c023a9a1de233554f28c7b21380b6c9003f36a8
            1532884444, // nTime
            331282217, // nTxCount
            2.4 // dTxRate
        };
        */
        chainTxData = ChainTxData{
            /* nTime    */ 1557171322,
            /* nTxCount */ 1,
            /* dTxRate  */ 0.011
        };

        /* enable fallback fee on mainnet until it has operated for a while for statistics to be available */
        m_fallback_fee_enabled = true;
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 92160;
        consensus.BIP16Exception = uint256S("0x0");
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x0");
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.powLimit = uint256S("000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 6 * 60 * 60; // 6 hours
        consensus.nPowTargetSpacing = 2 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 135; // 75% for testchains
        consensus.nMinerConfirmationWindow = 180; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1557171323;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1620243323;

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1557171323;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1620243323;

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 0;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000fff");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0"); //1354312

        pchMessageStart[0] = 0xAb;
        pchMessageStart[1] = 0xA1;
        pchMessageStart[2] = 0xA9;
        pchMessageStart[3] = 0xA7;
        nDefaultPort = 19099;
        nPruneAfterHeight = 1000;

        //  {
        /*
        uint32_t i = 0;
        arith_uint256 pol = UintToArith256 (consensus.powLimit);
        do {
            genesis = CreateGenesisBlock(1557171323, i, 0x1f0fffff, 1, 20000 * COIN);
            ++ i;
            consensus.hashGenesisBlock = genesis.GetHash();
            // std::cout << consensus.hashGenesisBlock.ToString () << "\n";
            if (i % 1000 == 0) printf ("Checking test nonce %u ...\n", i);
            // std::cout << UintToArith256 (consensus.hashGenesisBlock).ToString() << "\n";
            // std::cout << pol.ToString() << "\n";
        } while (UintToArith256 (consensus.hashGenesisBlock) > pol);
        printf ("Nonce = %u\n", i - 1);
        */
        //  }

        genesis = CreateGenesisBlock(1557171323, 13672, 0x1f0fffff, 1, 20000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        // std::cout << "genesis block hash = " << consensus.hashGenesisBlock.ToString () << "\n";
        // std::cout << "genesis block merkle root = " << genesis.hashMerkleRoot.ToString () << "\n";
        assert(consensus.hashGenesisBlock == uint256S("0x000d2b44ed3d75acbe0d5676d6653794bc0890f733657ad185e2ba34ddc0ecad"));
        assert(genesis.hashMerkleRoot == uint256S("0x1961c39fd7ce4c2ca210f39d71a91d723506bc052d6e408e329bf1613cc6931d"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;

        checkpointData = {
            {
                {0, uint256S("0x000d2b44ed3d75acbe0d5676d6653794bc0890f733657ad185e2ba34ddc0ecad")},
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 0000000000000037a8cd3e06cd5edbfe9dd1dbcc5dacab279376ef7cfc2b4c75
            /* nTime    */ 1557171323,
            /* nTxCount */ 1,
            /* dTxRate  */ 0.011
        };

        /* enable fallback fee on testnet */
        m_fallback_fee_enabled = true;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Exception = uint256();
        consensus.BIP34Height = 0; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 0; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 0; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 6 * 60 * 60; // 6 hours
        consensus.nPowTargetSpacing = 2 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xff;
        pchMessageStart[2] = 0xf5;
        pchMessageStart[3] = 0xfa;
        nDefaultPort = 29099;
        nPruneAfterHeight = 1000;

        //  {
        /*
        uint32_t i = 0;
        arith_uint256 pol = UintToArith256 (consensus.powLimit);
        do {
            genesis = CreateGenesisBlock(1557171326, i, 0x2000ffff, 1, 20000 * COIN);
            ++ i;
            consensus.hashGenesisBlock = genesis.GetHash();
            // std::cout << consensus.hashGenesisBlock.ToString () << "\n";
            if (i % 1000 == 0) printf ("Checking reg nonce %u ...\n", i);
            // std::cout << UintToArith256 (consensus.hashGenesisBlock).ToString() << "\n";
            // std::cout << pol.ToString() << "\n";
        } while (UintToArith256 (consensus.hashGenesisBlock) > pol);
        printf ("Nonce = %u\n", i - 1);
        */
        //  }

        genesis = CreateGenesisBlock(1557171326, 1053, 0x2000ffff, 1, 20000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        // std::cout << "genesis block hash = " << consensus.hashGenesisBlock.ToString () << "\n";
        // std::cout << "genesis block merkle root = " << genesis.hashMerkleRoot.ToString () << "\n";
        assert(consensus.hashGenesisBlock == uint256S("0x00cdd47e31f84f0c162fce696d892ac8656fd46fbd1810ba488d003586ad9dfd"));
        assert(genesis.hashMerkleRoot == uint256S("0x1961c39fd7ce4c2ca210f39d71a91d723506bc052d6e408e329bf1613cc6931d"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {
                {0, uint256S("0x00cdd47e31f84f0c162fce696d892ac8656fd46fbd1810ba488d003586ad9dfd")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "bcrt";

        /* enable fallback fee on regtest */
        m_fallback_fee_enabled = true;
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
