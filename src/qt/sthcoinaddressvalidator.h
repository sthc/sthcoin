// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STHCOIN_QT_STHCOINADDRESSVALIDATOR_H
#define STHCOIN_QT_STHCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class SthcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit SthcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Sthcoin address widget validator, checks for a valid sthcoin address.
 */
class SthcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit SthcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // STHCOIN_QT_STHCOINADDRESSVALIDATOR_H
