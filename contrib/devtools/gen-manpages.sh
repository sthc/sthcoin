#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

STHCOIND=${STHCOIND:-$BINDIR/sthcoind}
STHCOINCLI=${STHCOINCLI:-$BINDIR/sthcoin-cli}
STHCOINTX=${STHCOINTX:-$BINDIR/sthcoin-tx}
STHCOINQT=${STHCOINQT:-$BINDIR/qt/sthcoin-qt}

[ ! -x $STHCOIND ] && echo "$STHCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
STHCVER=($($STHCOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for sthcoind if --version-string is not set,
# but has different outcomes for sthcoin-qt and sthcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$STHCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $STHCOIND $STHCOINCLI $STHCOINTX $STHCOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${STHCVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${STHCVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
