#!/bin/bash
./test.sh > result.txt

pids=()

# Файл, который заблокируем
touch testfile


for i in {1..10}
do
    ./task.sh testfile $i & pids+=($!) # пускаем задачу в параллель и запоминаем её pid
done

success_tasks=0
sleep 5m

echo ${pids[@]}

for p in ${pids[@]}
do
	kill -2 $p # убиваем процесс
	if [[ $? -eq 0 ]] # если норм закончился
	then
		((success_tasks=$success_tasks+1)) # число успешных задач +1
	fi
done

wait

echo Success tasks : $success_tasks >> result.txt
for i in {1..10}
do
	cat task$i >> result.txt
	rm task$i
done

rm -f testfile.lck
rm testfile
