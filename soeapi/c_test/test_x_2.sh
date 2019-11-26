STORE=$3
SPACE=$2
CLUSTER=$1
NCOUNT=$4
YCOUNT=$5
APPX=KACZOR
APPX1=PIS

while true
do
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE -X 2 -n $NCOUNT -r 4 -y $YCOUNT -k "AAAKKKKK" -N 1000 -m 2 -p
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX -A -m 2 
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX1 -A -m 2 
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX -C -n 1000 -k "KULCZYK_DUPCZYK_SRULCZYK" -m 2 
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX1 -C -n 10000 -k "SRULCZYK_DUPULCZYK" -m 2 
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX -K -m 2 
c_test_soe -x sysprog -o $CLUSTER -z $SPACE -c $STORE$APPX1 -K -m 2 
done
