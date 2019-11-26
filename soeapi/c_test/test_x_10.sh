STORE=$3
SPACE=$2
CLUSTER=$1
NCOUNT=$4
YCOUNT=$5
KEY=KEY5DDDDDXXXOOOOOOOOOOOOOOXXXXX
VALUE="VALUE5__________________________________"
COUNTER=100

while true
    do
    COUNTER=$((COUNTER+1))
    C_KEY=$KEY$COUNTER
    C_VALUE=$VALUE$COUNTER
    c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE -X 10 -n $NCOUNT -r 1 -y $YCOUNT -k $C_KEY -v $C_VALUE -m 2 -T 1000000
    done
