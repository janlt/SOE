#!/usr/bin/python

import subprocess
import os
import string
import sys
import select
from datetime import datetime, date, time

# =====================================================================================

# Class encapsulating destroying a store
class DestroyStoreTask:
    cluster = "";
    space = "";
    store = "";
    out_file = "";
    out = "";

    def __init__(self, clu, spe, sto, out_f):
        self.cluster = clu;
        self.space = spe;
        self.store = sto;
        self.out_file = out_f;
        self.out = open(self.out_file, 'w')

    def printArgs(self):
        print "cluster=",self.cluster," space=",self.space," store=",self.store;

    def destroy(self):
        print "destroying store: ",  self.store, " ...";
        command = "c_test_soe -x sysprog " + "-o " + self.cluster + " -z " + self.space + " -c " + self.store + " -K -m 2" + "\n";
        print "command: ", command;
        self.out.write(command);
        proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True);
        stdout_iterator = iter(proc.stdout.readline, b"")
        stderr_iterator = iter(proc.stderr.readline, b"")

        for line in stdout_iterator:
            # Do stuff with line
            self.out.write(line)

        for line in stderr_iterator:
            # Do stuff with line
            self.out.write(line)

        #stdout = []
        #stderr = []
        #readfd = [proc.stdout.fileno(), proc.stderr.fileno()]
        #while True:
            #ret = select.select(readfd, [], [], 10.0) 
            #for fd in ret[0]:
                #if fd == proc.stdout.fileno():
                    #line_o = proc.stdout.readline()
	            #sys.stdout.write(line_o)
	            #self.out.write(line_o)
                #if fd == proc.stderr.fileno():
	            #line_r = proc.stderr.readline()
	            #sys.stdout.write(line_r)
	            #self.out.write(line_r)
            #if proc.poll() != None:
                #proc.stdout.close()
                #proc.stderr.close()
                #break
        
    
# =====================================================================================

destroys_list = [];
try:
    inpfile = 'store_list.txt'
    outfile = 'destroyed_list.txt'
    if len(sys.argv) > 1:
        inpfile = sys.argv[1]
    if len(sys.argv) > 2:
        outfile = sys.argv[2]

    f1 = open(inpfile, 'r')
except IOError as e:
    print "Oops! file error({0}): {1}".format(e.errno, e.strerror)
    
curr_cluster = ""
curr_space = ""
curr_store = ""

for line in f1:
    # print line;
    if len(line.lstrip(' ').rstrip(' ').rstrip('\n')) == 0:
        continue;
    if line.find('subset') >= 0:
        continue;
    if line.find('cluster') >= 0:
        cols1 = line.lstrip(' ').split("(");
        # print "cl1[0] ", cols1[0], " cl[1] ", cols1[1];
        curr_cl = cols1[1].split("/");
        curr_cluster = curr_cl[0];
        #print "c_c ", curr_cluster;
    if line.find('space') >= 0:
        cols2 = line.lstrip(' ').split("(");
        # print "cl2[0] ", cols2[0];
        curr_sp = cols2[1].split("/");
        curr_space = curr_sp[0];
        #print "c_s ", curr_space;
    if line.find('store') >= 0:
        cols3 = line.lstrip(' ').split("(");
        # print "cl3[0] ", cols3[0];
        curr_st = cols3[1].split("/");
        curr_store = curr_st[0];
        #print "c_t ", curr_store;

    if len(curr_cluster) == 0:
        continue;
    if len(curr_space) == 0:
        continue;
    if len(curr_store) == 0:
        continue;

    a_destroy = DestroyStoreTask(curr_cluster, curr_space, curr_store, outfile);
    destroys_list.append(a_destroy);

a = iter(destroys_list)
for i in a:
    #i.printArgs()
    i.destroy()
