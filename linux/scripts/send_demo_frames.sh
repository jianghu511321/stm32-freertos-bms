#!/bin/sh
set -eu

interface="${1:-vcan0}"
sequence="0100"

cansend "$interface" 300#${sequence}420E450E480E
cansend "$interface" 301#${sequence}4B0E4E0E510E
cansend "$interface" 302#${sequence}540E570E5A0E
cansend "$interface" 303#${sequence}5D0E600E630E
cansend "$interface" 304#${sequence}DEAB1EFB0000
cansend "$interface" 305#${sequence}FA00FF000401
cansend "$interface" 306#${sequence}0901420E630E
cansend "$interface" 307#${sequence}102700000000
