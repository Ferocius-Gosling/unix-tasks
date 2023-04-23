#!/bin/bash
make

mkdir /tmp/task3
touch /tmp/task3/in
touch /tmp/task3/out

# создали конфиг
echo /bin/sleep 5 > /tmp/task3/config
echo /bin/sleep 6 >> /tmp/task3/config
echo /bin/sleep 7 >> /tmp/task3/config
echo /tmp/task3/in >> /tmp/task3/config
echo /tmp/task3/out >> /tmp/task3/config

# запустим щас инит
./myinit /tmp/task3/config

# чекаем детей
echo First three tasks: > result.txt
ps -ef | grep /bin/sleep >> result.txt

pkill -f "/bin/sleep 6"

sleep 1
# чекаем детей
echo After kill second task: >> result.txt
ps -ef | grep /bin/sleep >> result.txt

# теперь надо по идее 7 секунд подождать
# чтобы увидеть три завершённых задачи
# а пока ждём перезапишем конфиг
echo /bin/sleep 50 > /tmp/task3/config
echo /tmp/task3/in >> /tmp/task3/config
echo /tmp/task3/out >> /tmp/task3/config
sleep 7

# после сна пошлём процу сигхуп
pkill --signal SIGHUP -f "./myinit /tmp/task3/config"

# он должен зарестартиться
# чекаем детей
echo After sending sighup to daemon: >> result.txt
ps -ef | grep /bin/sleep >> result.txt

# почекаем что написало в лог
echo Logs from file: >> result.txt
cat /tmp/myinit.log >> result.txt

pkill -f "./myinit /tmp/task3/config"