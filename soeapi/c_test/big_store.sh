bin/c_test_soe -x sysprog -o KURCZE -z SSSSS -c STOREK2 -A -m 2
KEY=KEY5DDDDDXXXOOOOOOOOOOOOOO
VALUE="VALUE5__________________________________"
COUNTER=100
while true 
    do
    COUNTER=$((COUNTER+1))
    C_KEY=$KEY$COUNTER
    C_VALUE=$VALUE$COUNTER
    echo $C_KEY " " $C_VALUE " " $COUNTER
    bin/c_test_soe -x sysprog -o KURCZE -z SSSSS -c STOREK2 -C -n 200000 -N 5000 -k $C_KEY -v $C_VALUE -m 2
    done
