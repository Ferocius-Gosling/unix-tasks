#!/bin/bash

make > /dev/null

touch test
./locker test 4 &
sleep 1
rm -f test.lck

wait
echo "removing test"

touch test1
./locker test1 4 &
sleep 1
echo "qwe" > test1.lck

wait
echo "corrupting test"
rm -f test1.lck
rm -f test
rm -f test1
