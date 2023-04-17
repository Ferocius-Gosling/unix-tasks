lock_sum=0
task_number=$2
pids=()

sigint_handler() {
    echo task$task_number : $lock_sum > task$task_number
    exit
}

trap 'sigint_handler' INT

while true
do
    ./locker $1 1
    if [[ $? -eq 0 ]]
    then
        ((lock_sum=$lock_sum+1))
    fi
done
