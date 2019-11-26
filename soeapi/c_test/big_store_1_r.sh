STORE_NAME=$1
VALUE_SIZE=$2
NUM_OPS=$3
NUM_THREADS=$4
c_test_soe -x sysprog -o KURCZE -z SSSSS -c $STORE_NAME -A -m 2
KEY=MUKAKEY5DDDDDXXXOOOOOOOOOOOOOOXXXXX
VALUE="VALUE5__________________________________"
COUNTER=100
export DATE=`date '+%Y-%m-%d-%H:%M:%S:%N'`
while true 
    do
    COUNTER=$((COUNTER+1))
    C_KEY=$KEY$COUNTER$DATE
    C_VALUE=$VALUE$COUNTER
    echo $C_KEY " " $C_VALUE " " $COUNTER
    c_test_soe -x sysprog -o KURCZE -z SSSSS -c $STORE_NAME -r $NUM_THREADS -C -n $NUM_OPS -N $VALUE_SIZE -k $C_KEY -m 2 -T 1000000
    sleep 1
    done
