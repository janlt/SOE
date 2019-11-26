while true; 
    do 
    echo $1 " " $2
    c_test_soe -x sysprog -o CSUB -z SSUB -c $1 -d $2 -D -n 8 KEY -m 2
    sleep 1
    c_test_soe -x sysprog -o CSUB -z SSUB -c $1 -d $2 -F -k "" -e "" -m 2
    sleep 1
    done
