CLUS=$1
SPAC=$2
STOR=$3
THREADS=$4
integrity_test -x sysprog -o $CLUS -z $SPAC -c $STOR -X 12 -r $THREADS -n 64 -y 80000 -N 200 -k KEYYYYYYYYY -m 2 -p -R 
