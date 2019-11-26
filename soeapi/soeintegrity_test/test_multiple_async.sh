CLUS=$1
SPAC=$2
STOR=$3
THREADS=$4
integrity_test -x sysprog -o $CLUS -z $SPAC -c A1$STOR -X 12 -r $THREADS -n 64 -y 10000 -N 200 -k KEYYYYYYYYY -m 2 -p -R > $STOR1.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A2$STOR -X 12 -r $THREADS -n 64 -y 400 -N 2000 -k KEYYYYYYYYY -m 2 -p -R > $STOR2.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A3$STOR -X 12 -r $THREADS -n 64 -y 1600 -N 500 -k KEYYYYYYYYY -m 2 -p -R > $STOR3.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A4$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR4.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A5$STOR -X 12 -r $THREADS -n 64 -y 40 -N 8000 -k KEYYYYYYYYY -m 2 -p -R > $STOR5.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A6$STOR -X 12 -r $THREADS -n 64 -y 800 -N 500 -k KEYYYYYYYYY -m 2 -p -R > $STOR6.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A7$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR7.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A8$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR8.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A9$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR9.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A10$STOR -X 12 -r $THREADS -n 64 -y 15000 -N 300 -k KEYYYYYYYYY -m 2 -p -R > $STOR10.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A11$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR11.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A12$STOR -X 12 -r $THREADS -n 64 -y 4000 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR12.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A13$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR13.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A14$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR14.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A15$STOR -X 12 -r $THREADS -n 64 -y 30000 -N 100 -k KEYYYYYYYYY -m 2 -p -R > $STOR15.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A16$STOR -X 12 -r $THREADS -n 64 -y 30000 -N 300 -k KEYYYYYYYYY -m 2 -p -R > $STOR16.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A17$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR17.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A18$STOR -X 12 -r $THREADS -n 64 -y 800 -N 400 -k KEYYYYYYYYY -m 2 -p -R > $STOR18.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A19$STOR -X 12 -r $THREADS -n 64 -y 3000 -N 100 -k KEYYYYYYYYY -m 2 -p -R > $STOR19.out 2>&1 &
integrity_test -x sysprog -o $CLUS -z $SPAC -c A20$STOR -X 12 -r $THREADS -n 64 -y 3000 -N 300 -k KEYYYYYYYYY -m 2 -p -R > $STOR20.out 2>&1 &
