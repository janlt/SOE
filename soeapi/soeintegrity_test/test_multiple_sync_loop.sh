CLUS=$1
SPAC=$2
STOR=$3
THREADS=$4

while true 
do
COUNTER=$((COUNTER+1))
echo "COUNTER ... " $COUNTER
integrity_test -x sysprog -o $CLUS -z $SPAC -c A1$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 10000 -N 200 -k KEYYYYYYYYY -m 2 -p -R > $STOR1.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A2$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 400 -N 2000 -k KEYYYYYYYYY -m 2 -p -R > $STOR2.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A3$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 1600 -N 500 -k KEYYYYYYYYY -m 2 -p -R > $STOR3.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A4$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR4.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A5$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 40 -N 8000 -k KEYYYYYYYYY -m 2 -p -R > $STOR5.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A6$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 800 -N 500 -k KEYYYYYYYYY -m 2 -p -R > $STOR6.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A7$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR7.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A8$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR8.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A9$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR9.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A10$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 15000 -N 300 -k KEYYYYYYYYY -m 2 -p -R > $STOR10.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A11$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR11.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A12$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR12.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A13$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR13.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A14$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR14.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A15$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 30000 -N 100 -k KEYYYYYYYYY -m 2 -p -R > $STOR15.sync.out 2>&1 
integrity_test -x sysprog -o $CLUS -z $SPAC -c A16$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 30000 -N 300 -k KEYYYYYYYYY -m 2 -p -R > $STOR16.sync.out 2>&1 
done
