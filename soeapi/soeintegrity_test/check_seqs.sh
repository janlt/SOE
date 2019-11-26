for line in `cat c.J.seqs`
  do 
    grep $line c.J.soa.out | awk '{ print $5}' > tmp_seq.s.out
    grep $line c.J.rao.out | awk '{ print $5}' > tmp_seq.r.out
    DIFF=`diff tmp_seq.s.out tmp_seq.r.out`
    if [ -n "$DIFF" ] ; then
      grep $line c.J.soa.out > tmp_seq.s.err
      grep $line c.J.rao.out > tmp_seq.r.err
      echo " pos " $line 
      cat tmp_seq.s.err
      cat tmp_seq.r.err
    fi
    grep $line c.J.soa.out | awk '{ print $6}' > tmp_seq.s.out
    grep $line c.J.rao.out | awk '{ print $6}' > tmp_seq.r.out
    DIFF=`diff tmp_seq.s.out tmp_seq.r.out`
    if [ -n "$DIFF" ] ; then
      grep $line c.J.soa.out > tmp_seq.s.err
      grep $line c.J.rao.out > tmp_seq.r.err
      echo " th " $line 
      cat tmp_seq.s.err
      cat tmp_seq.r.err
    fi
    grep $line c.J.soa.out | awk '{ print $7}' > tmp_seq.s.out
    grep $line c.J.rao.out | awk '{ print $7}' > tmp_seq.r.out
    DIFF=`diff tmp_seq.s.out tmp_seq.r.out`
    if [ -n "$DIFF" ] ; then
      grep $line c.J.soa.out > tmp_seq.s.err
      grep $line c.J.rao.out > tmp_seq.r.err
      echo " e_p " $line 
      cat tmp_seq.s.err
      cat tmp_seq.r.err
    fi
  done
