Soothing Coin (Sthcoin)
======================

Introduction
------------

Sthcoin is based on [Bitcoin](https://github.com/bitcoin/bitcoin), yet has a few critical improvements, for example,

* It fixed a Bitcoin security loophole. With the loophole, Bitcoin P2SH addresses lose all benefits of multisig and are as weak as normal addresses, which makes it exponentially more likely for coins to be stolen.
* It is CPU-mining only, guaranteed, thanks to our unique DOWS (Dynamic Operations with Wide Sampling) algorithm. As a result, everyone can mine without having to purchase expensive special equipment first. (Well, GPU/ASIC mining can still be done, just won't be cost-effective.)

In addition, Sthcoin is **open**. Everybody, tech-savvy or not, can earn coins for FREE by playing/contributing in one or more of the following ways.

* Referral
* Lottery
* Mining
* Helping Q&A
* Development
* Management

In the future, while keeping track of Bitcoin development, we will continuously improve the security, accessbility and speed of Sthcoin.

Build
-----

Sthcoin needs [Lua](https://www.lua.org/). For now its include files and library need to be added manually. Following the following steps (Sorry for the briefness. We will find time to add more details later).

* Install Lua development files or download the source from [the Lua website](https://www.lua.org/ftp) and made the static library.

* Follow the general build instructions of [Bitcoin](https://github.com/bitcoin/bitcoin/tree/master/doc). Before running 'make', add the include path and library path and library name to CXXFLAGS (-I...), LDFLAGS (-L...) and LIBS (-l..., e.g. -llua53) respectively.

* Run 'make'.

Note: It will take quite some time for the application to initialize. Please be patient.
