CLUS=$1
SPAC=$2
STOR=$3
THREADS=$4

while true 
do
COUNTER=$((COUNTER+1))
echo "COUNTER ... " $COUNTER
integrity_test -x sysprog -o $CLUS -z $SPAC -c A1$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 10000 -N 200 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR1.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A2$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 400 -N 2000 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR2.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A3$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 1600 -N 500 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR3.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A4$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR4.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A5$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 40 -N 8000 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR5.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A6$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 800 -N 500 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR6.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A7$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR7.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A8$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR8.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A9$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR9.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A10$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 15000 -N 300 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR10.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A11$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR11.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A12$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR12.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A13$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR13.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A14$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR14.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A15$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 30000 -N 100 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR15.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A16$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 30000 -N 300 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR16.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A17$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR17.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A18$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR18.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A19$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 3000 -N 100 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR19.async.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A20$COUNTER$STOR -X 12 -r $THREADS -n 64 -y 3000 -N 300 -k KEYYYYYYYYY -m 2 -p -R > $COUNTER$STOR20.async.out 2>&1 &
done
