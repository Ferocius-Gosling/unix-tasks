#!/bin/bash

for i in {1..100}
do
    # Задержка в 1 миллисекунду
    (./test_client config 1000 $i 0.001 < nums) >> /dev/null & # пускаем задачу в параллель
done

# подождали пока закончат
wait

touch zero
echo 0 > zero

# проверили что сервер с состоянием 0 (инфа в логах сервера)
./client config 1 < zero

rm -f zero