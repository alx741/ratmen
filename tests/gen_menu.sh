#!/bin/sh

# Usage: $0 prefix n
# Generate a printing menu (-pd:) of N elements and PREFIX

VMENU="../vmenu"

if [[ $1 == "" ]] || [[ $2 == "" ]]
then
    echo "Usage: $0 prefix n"
    echo
    exit 0
fi


if [[ ! -x "$VMENU" ]]
then
    echo "[!] Please compile vmenu first"
    echo
    exit 0
fi



command="$VMENU -pd: "
for i in `seq "$2"`
do
    command+="$1_$i "
done

eval "$command"
