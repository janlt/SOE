KEY=KEYEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
COUNTER=1
STORE_NAME=$1
while true 
    do
    COUNTER=$((COUNTER+1))
    C_KEY=$KEY$COUNTER
    C_STORE_NAME=$STORE_NAME$COUNTER
    echo $C_KEY " " 
    time bin/integrity_test -x sysprog -o A -z A -c $C_STORE_NAME -X 12 -r 1 -n 64 -y 16000 -N 350 -k $C_KEY -m 2 -p
    done
