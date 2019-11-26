SSSS=$1
COUNTER=1
while true 
do
COUNTER=$((COUNTER+1))
echo "COUNTER ... " $COUNTER
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -A -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -K -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -A -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -C -n 16 -k "KEY" -v "VALUE" -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -E -k "" -e "" -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -K -m 2 
done
