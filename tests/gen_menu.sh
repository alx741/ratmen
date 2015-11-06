#!/bin/sh

# Usage: $0 prefix n
# Generate a printing menu (-pd:) of N elements and PREFIX

RATMEN="../ratmen"

if [[ $1 == "" ]] || [[ $2 == "" ]]
then
    echo "Usage: $0 prefix n"
    echo
    exit 0
fi


if [[ ! -x "$RATMEN" ]]
then
    echo "[!] Please compile ratmen first"
    echo
    exit 0
fi



command="$RATMEN -pd: "
for i in `seq "$2"`
do
    command+="$1_$i "
done

eval "$command"
