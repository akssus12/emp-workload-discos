#! /bin/bash

# $1 : target file neme
# $2 : target word
# line : lines of target file

line=$(wc -l < $1)
echo "${line}"

./omp_grep.out $1 $line $2