#!/bin/bash

x=$1
y=$2
z=$3
((n=x*y*z))
echo Spawning $n processes with dimensions \($x, $y, $z\).
echo mpirun -n $n ./demo $x $y $z $4 $5 $6
mpirun -n $n ./demo $x $y $z $4 $5 $6
