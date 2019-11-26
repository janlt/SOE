STOREK=$1
CLUSTEREK=$2
SPACEK=$3
ITERS=$4
NUMS=$5
VALSIZE=$6
KEY=KEY5DDDDDXXXOOOOOOOOOOOOOO
VALUE="VALUE___________________VALUE___________________VALUE____"
COUNTER=100
while true 
    do
    COUNTER=$((COUNTER+1))
    C_KEY=$KEY$COUNTER
    C_VALUE=$VALUE$COUNTER
    integrity_test -x sysprog -o $CLUSTEREK -z $SPACEK -c $STOREK -X 12 -r 2 -n $NUMS -y $ITERS -k $C_KEY -v $C_VALUE -N $VALSIZE -m 2 -p
    sleep 1
    done
