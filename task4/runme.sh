#!/bin/bash
clients=(5 25 50 75 100)
delays=(0 0.2 0.4 0.6 0.8 1)

make

python3 gen.py # генерируем числа
./server config & # стартуем сервак

sleep 1 # немного ждём, чтобы клиенты успели подключиться

# тест на работоспособность в принципе
./test1.sh 

echo First attempt completed > result.txt

# проверка на то что можно так делать много раз
# + не используются лишние дескрипторы и не растёт использование памяти
./test1.sh
./test1.sh

# первая и последняя запись из лога
echo Three attempts completed >> result.txt
echo There is first and last logs in /tmp/server.log >> result.txt
grep -in 'Accept socket 6. ' /tmp/server.log | grep -i -e '^4:' -e '600310:' >> result.txt

# эксперимент с задержками и разным числом клиентов.
# + вычисление эффективности работы сервера.

for c in ${clients[@]}
do
    echo "clients $c"
    for d in ${delays[@]}
    do
        echo "delays $d"
        # передаём число клиентов и задержку
        ./test2.sh $c $d
    done
done

pkill -f "./server config"