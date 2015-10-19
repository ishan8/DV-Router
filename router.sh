#!/bin/bash

pref="details"
pref1="routerOutput"
suf=".txt"
y=(0 A B C D E F G H I J K L M N O P Q R S T U V W X Y Z)
./fileio $1

for ((i = 1; ; i++))
do
	if [ ! -f "$pref$i$suf" ]; then
		break
	else
    ( sleep 0.1; ./router "$pref$i$suf" & )
    fi
done

rm $pref*$suf