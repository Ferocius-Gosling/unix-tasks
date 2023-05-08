#!/bin/bash

# первый аргумент - число клиентов
# второй аргумент - задержка
num_clients=$1
delay=$2


rm -f /tmp/client.log
touch /tmp/client.log # сюда будут складываться просто числа - время работы клиента.
SECONDS=0 # встроенная переменная. означает что мы только сделали первый запрос.
start_time="$(date -u +%s)"

for ((i=1; i <= $num_clients; i++));
do
    (./test_client config 20 $i $delay < nums) >> /tmp/client.log &
done

wait
# замерили сколько вычислялись.
echo "Results for $num_clients clients and delay $delay :" >> result.txt
# найдём клиента, который дольше всех работал
longest_client=$(grep -Eo '[0-9]+' /tmp/client.log | sort -rn | head -n 1)
echo "Longest client working for $longest_client seconds" >> result.txt

end_time="$(date -u +%s)"
elapsed="$(($end_time-$start_time))"
echo "Total of $elapsed seconds elapsed for server" >> result.txt

duration=$SECONDS
efficiency=$((duration - longest_client))
echo "Total efficiency $efficiency seconds elapsed" >> result.txt
# здесь бы вычислять уже что-нибудь