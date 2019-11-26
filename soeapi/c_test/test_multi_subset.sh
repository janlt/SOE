STORE=$3
SPACE=$2
CLUSTER=$1
NCOUNT=$4
YCOUNT=$5
APPX=KACZOR
APPX1=PIS
SUBAPPX=MACIOR
COUNTER=120
COUNTER1=120

while true
do
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX$COUNTER1 -A -m 2 
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX1$COUNTER1 -A -m 2 
echo "doing stores ... $STORE$APPX$COUNTER1 $STORE$APPX1$COUNTER1"
    while true
    do
    c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX$COUNTER1 -d $SUBAPPX$STORE$COUNTER -B -m 2 
    c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX1$COUNTER1 -d $SUBAPPX$STORE$COUNTER -B  -m 2 
    c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX$COUNTER1 -d $SUBAPPX$STORE$COUNTER -D -n 100 -k "KULCZYK_DUPCZYK_SRULCZYK" -m 2 
    c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX1$COUNTER1 -d $SUBAPPX$STORE$COUNTER -D -n 100 -k "SRULCZYK_DUPULCZYK" -m 2 
    c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX$COUNTER1 -d $SUBAPPX$STORE$COUNTER -L -m 2 
    c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX1$COUNTER1 -d $SUBAPPX$STORE$COUNTER -L -m 2 
    COUNTER=$((COUNTER+1))
    if [ "$COUNTER" -gt 140 ]; then
        break 
    fi
    echo "doing subsets ... $SUBAPPX$STORE$COUNTER $SUBAPPX$STORE$COUNTER"
    done
COUNTER=120
COUNTER1=$((COUNTER1+1))
sleep 1
done
