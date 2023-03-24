#!/bin/bash

make

python3 gen.py
./create_sparse fileA fileB
gzip -k fileA fileB
gzip -cd fileB.gz | ./create_sparse fileC
./create_sparse fileA fileD 100
stat "%n %s" file*
