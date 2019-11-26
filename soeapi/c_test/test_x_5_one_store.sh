STORE=$3
SPACE=$2
CLUSTER=$1
NCOUNT=$4
YCOUNT=$5
APPX=KACZOR
APPX1=PIS

while true
do
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE -X 5 -n $NCOUNT -r 1 -y $YCOUNT -k "AAAKKKKK" -N 10000 -m 2 -p -T 1000000
sleep 1
done
