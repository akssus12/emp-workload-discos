#! /bin/bash

# $1 : target file neme
# line : lines of target file

line=$(wc -l < $1)
echo "${line}"

./omp_wc.out $1 $line
