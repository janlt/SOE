STOREK=$1
CLUSTEREK=$2
SPACEK=$3
ITERS=$4
NUMS=$5
LOOPS=$6
VALSIZE=$7
KEY=KEY5DDDDDXXXOOOOOOOOOOOOOO
VALUE="VALUE"
COUNTER=100
while true 
    do
    COUNTER=$((COUNTER+1))
    C_KEY=$KEY$COUNTER
    C_VALUE=$VALUE$COUNTER
    integrity_test -x sysprog -o $CLUSTEREK -z $SPACEK -c $STOREK -X 13 -r 4 -W $LOOPS -y $ITERS -n $NUMS -k $C_KEY -v $C_VALUE -N $VALSIZE -m 2 -p
    done
