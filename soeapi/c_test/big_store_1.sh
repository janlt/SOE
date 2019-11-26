STORE_NAME=$1
VALUE_SIZE=$2
NUM_OPS=$3
bin/c_test_soe -x sysprog -o KURCZE -z SSSSS -c $STORE_NAME -A -m 2
KEY=KEY5DDDDDXXXOOOOOOOOOOOOOOXXXXX
VALUE="VALUE5__________________________________"
COUNTER=100
while true 
    do
    COUNTER=$((COUNTER+1))
    C_KEY=$KEY$COUNTER
    C_VALUE=$VALUE$COUNTER
    echo $C_KEY " " $C_VALUE " " $COUNTER
    bin/c_test_soe -x sysprog -o KURCZE -z SSSSS -c $STORE_NAME -C -n $NUM_OPS -N $VALUE_SIZE -k $C_KEY -m 2
    sleep 1
    done
