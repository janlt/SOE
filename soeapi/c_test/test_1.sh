SSSS=$1
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -A -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -K -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -A -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -C -n 16 -k "KEY" -v "VALUE" -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -E -k "" -e "" -m 2 
bin/c_test_soe -x sysprog -o KURCZE -z ZZZZZ -c $SSSS -K -m 2 
