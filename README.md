<h1>SOE</h1>
<h2>C++ API Overview</h2>
<p>
<p>Soe is a distributed and persistent key-value store. Soe is
also a NoSQL database providing a mechanism for storage and retrieval of data,
which is modeled in terms other than relational schema.</p>
<p>Soe provides infrastructure to consolidate and unify various
heterogeneous KV stores, RDBMs, memory-only databases, etc., under a
distributed umbrella with a unified set of APIs.</p>
<p></p>
<p>
<p>
<h1>Building and installing</h1>
<p>
<p>The prerequisites for Fedora 28+ before building SOE are the following.</p>

     boost* 1.66+
     json-c 0.13+
     json-glib 1.4+
     jsoncpp 1.8+
     rocksdb 5.7+
   
<p> Then build it</p>
   
     make
     make install
   
<p>  ultimately the install step can be skipped and the environment can be set up instead:</p>

     . envinit.sh
   
<p>  SOE server can be installed as a service using soemetadbsrv/files/usr/lib/systemd/system/soemetadbsrv.service
   as a template.</p>
   
<p>   Steps to run some tests:</p>
   
     Start up soemetadbsrv from the build folder:
     
     soemetadbsrv/bin/soemetadbsrv -u <YOUR_USER_NAME>
     or
     sudo soemetadbsrv/bin/soemetadbsrv -u <ANOTHER_USER_NAME>
       
     Create a store (database) using soe_test utility.
   
     From the build folder run:
     
     soeapi/c_test/bin/soe_test -x <YOUR_USER_NAME> -o <CLUSTER> -z <SPACE> -c <STORE> -A -m 2
       
     then insert a bunch of records in it:
     soeapi/c_test/bin/soe_test -x <YOUR_USER_NAME> -o <CLUSTER> -z <SPACE> -c <STORE> -C -n 100 -N 1000 -k "MY_KEY_" -m 2
       
     finally query it:
     soeapi/c_test/bin/soe_test -x <YOUR_USER_NAME> -o <CLUSTER> -z <SPACE> -c <STORE> -E -k "" -e "" -m 2
     
     The integrity_check utility can be used to exercise some asynchronous APIs:
     
     soeapi/soeintegrity_test/bin/soe_integrity_test -x <YOUR_USER_NAME> -o GG -z GG -c YY -X 8 -n 100 -m 2
     
     The above command inserted 100 records with predefined key in one vector async call. The results can be 
     verified using soe_test:
     
     soeapi/c_test/bin/soe_test -x <YOUR_USER_NAME> -o GG -z GG -c YY -E -k "" -e "" -m 2</p>
<p></p>
<p>
<h3>Asynchronous APIs</h3>

Asynchronous APIs rely on C++ Future class libraries compring a class hierarchy.
Soe futures can be created and destroyed only using session API. Sessions provide context
for future's invocation and handling. Future's constructors and destructors are protected
thus preventing users from creating futures outside of sessions.

There are different future classes, depending on the requested operation.
Single i/o requests, i.e. GetAsync/PutAsync/DeleteAsync/MergeAsync have corresponding future types
as follows:
GetAsync -> GetFuture
PutAsync -> PutFuture
DeletetAsync -> DeleteFuture
MergeAsync -> MergeFuture

For vectored requests, i.e. GetSetAsync/PutSetAsync/DeleteSetAsync/MergeSetAsync the return future
types are the following:
GetSetAsync -> GetSetFuture
PutSetAsync -> PutSetFuture
DeletetSetAsync -> DeleteSetFuture
MergeSetAsync -> MergeSetFuture

Once a future is created via session's async API, it can be used later on to obtain
the status and the result, i.e. key(s) and value(s). Future's API provides Get() method to do that.
Get() method will synchronize future with the return status and result, so potentially it's
a blocking call. Get() may block the caller if a future has not yet received its result.
On vectored requests, Get() will synchronize its future with the results of all the elements of the
vector. For example, if the input vector for PutSetAsync() contains 100 elements, the PutSetFuture
will be synchronized with the statuses of all individual Put requests, i.e. Get() will return
when all of the elements of the input vector have been written and their statuses communicated back
to the future object.
Like in synchronous vectored requests, where an element of the input vector may contain a duplicate
or non-existing key, a vectored future will become available upon encountering the first eDuplicateKey
or eNotFoundKey, provided the boolean flag fail_on_first is set to true.</p>
<h2>Class API Overview</h2>
<p>
<table class=MsoTableGrid border=1 cellspacing=0 cellpadding=0
 style='border-collapse:collapse;border:none'>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal><b>Class name</b></p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border:solid windowtext 1.0pt;
  border-left:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal><b>Description</b></p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Session</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Gives access to {"Cluster", "Space", "Store", "Provider"}
  space. Users can open, create, destroy and manipulate the contents of KV
  stores through Session API.</p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Group</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Allows grouping of multiple operations to the same store
  in one batch, i.e. a Group object holds a sequence of edits to be made to a
  store. Invocation of Write method will write the contents to a store. </p>
  <p class=MsoNormal>Group s primarily used to speed up bulk updates. </p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Transaction</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Transactions are used to write multiple items in a transactional
  fashion. That ensures not only atomicity and isolation but also resolves
  potential conflicts that may occur when two or more transactions try to
  update the same key(s). </p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Duple</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Duple is a wrapper for {Pointer, Size} tuple. Duple's
  storage pointed to by "Pointer" is owned by the caller, so the caller is
  responsible for managing it.</p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>DisjointSubset</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Subsets are used to create lightweight groups within a
  store. DisjointSubset is a group of items within a store with unique keys. </p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>SubsetIterator</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Subsets can be iterated upon using SubsetIterator.
  SubsetIterators are created with a traversal direction and support most
  common iterator methods.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>SessionIterator</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>SessionIterator allows iterating over the contents of the
  entire store. Typically, when creating a SessionIterator user specifies start
  and end key.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Futurable</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Base abstract class for all of the SOE Future class hierarchies.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>SimpleFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Base class for one operation Future classes.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>PutFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Class used to put one key-value pair asynchronously in a store.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>GetFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Class used to get one key-value pair asynchronously from a store.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>MergeFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Class used to merge one key-value pair asynchronously in a store..</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>DeleteFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Class used to delete one key-value pair asynchronously from a store.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>SetFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Base class for vector Future classes, i.e. 
    when a vector<> of key-value pairs is wriiten/read/merged or deleted.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>PutSetFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Class used to put a vector of key-value pairs asynchronously in a store.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>GetSetFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Class used to get a vector of key-value pairs asynchronously from a store.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>MergeSetFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Class used to merge a vector of key-value pairs asynchronously in a store..</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>DeleteSetFuture</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Class used to delete a vector of key-value pairs asynchronously from a store.</p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>BackupEngine</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>BackupEngine is used to create a backup engine, i.e. an
  object that can be used to create or destroy a store backup.</p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>SnapshotEngine</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>SnaphotEngine is used to create a snapshot engine, i.e. an
  object that can be used to create or destroy a store snapshot.</p>
  </td>
 </tr>
</table>
<h2>Test application</h2>
<p>

<p>Example application "soe_test" is located in:
soe/soeapi/c_test</p>


<p>To build test application simply run "make release" in test directory</p>


<p>Test application options:</p>


<pre>Usage bin/c_test_soe [options]
  -o, --cluster-name             Soe cluster name
  -z, --space-name               Soe space name
  -c, --store-name               Soe store name
  -l, --transactional            Soe transactional store (default 0 - non-transactional)
  -n, --num-ops                  Number of ops (default = 1)
  -k, --key                      Key (default key_1)
  -e, --end-key                  End key (default key_1)
  -a, --hex-key                  Key specified in hex format, e.g. 0F45E8CD08C5C5C5
  -t, --iterator-dir             Iterator dir (0 - forward(default), 1 - reverse)
  -u, --default-first-iter-arg   Use iterator's First() args defaults
  -v, --value                    Value (default value_1)
  -d  --snap-backup-subset-name  Snapshot, backup or subset name (default snapshot_1, backup_1, subset_1)
  -f  --snap-back-id             Snapshot or backup id (default 0)
  -i, --sync-type                Sync type (0 - default, 1 - fdatasync, 2 - async WAL)
  -m, --provider                 Store provider (0 - RCSDBEM, 1 - KVS, 2 - METADBCLI, 3 - POSTGRES, 4 - DEFAULT provider)
  -j, --debug                    Debug (default 0)
  -g, --write-group              Do writes as group write (batching) instead of individual writes (1 - Put, 2 - Merge)
  -w, --write-transaction        Do writes as transaction (1,2,4,5 - commit, 3 - rollback, 1,3,4,5 - Put, 2 - Merge, 4 - Get, 5 - Delete)
  -s, --delete-from-group        Do delete from group in the middle of batching
  -A, --create-store             Create cluster/space/store
  -B, --create-subset            Create subset
  -C, --write-store              Write store num_ops (default 1) KV pairs
  -D, --write-subset             Write subset num_ops (default 1) KV pairs
  -O, --merge-store              Merge in store num_ops (default 1) KV pairs
  -P, --merge-subset             Merge in subset num_ops (default 1) KV pairs
  -E, --read-store               Read all KV items from store
  -Q  --repair-store             Repair store
  -S  --write-store-async        Write store async num_ops (default 1) KV pairs
  -W  --read-store-async         Read KV items from store async num_ops (defualt 1)
  -Y  --merge-store-async        Merge in store num_ops (default 1)
  -Z  --delete-store-async       Delete KV items async (defualt 1)
  -R  --traverse-name-space      Traverse name space (print all clusters/spaces/stores/subsets that are there)
  -F, --read-subset              Read all KV items from subset
  -G, --read-kv-item-store       Read one KV item from store
  -H, --read-kv-item-subset      Read one KV item from subset
  -I, --delete-kv-item-store     Delete one KV item from store
  -J, --delete-kv-item-subset    Delete one KV item from subset
  -K, --destroy-store            Destroy store
  -L, --destroy-subset           Destroy subset
  -M, --hex-dump                 Do hex dump of read records (1 - hex print, 2 - pipe to stdout)
  -X, --regression               Regression test
                                 1 - create/destroy stores multi-threaded (default)
                                 2 - create/destroy stores/subsets multi-threaded
                                 3 - create/destroy and write/read different name stores/subsets multi-threaded
                                 4 - create/destroy and write/read same name single store/subset multi-threaded
                                 5 - create/destroy and write/read same name single store multi-threaded
                                 6 - performance test
                                 7 - single session multiple threads performance test
                                 8 - single session loop(open/create/write/read/close)
                                 9 - create/write/destroy different subset same store multi-threaded
                                10 - create open/write/read/close same name
  -N, --data-size                Set data size to an arbitrary value (will override -v)
  -T, --sleep-usecs              Sleep usecs between tests
  -y, --regression-loop-cnt      Regression loop count
  -r, --regression-thread-cnt    Regression thread count
  -p, --no-print                 No printing key/value (default print)
  -x, --user-name                Run unser user-name
  -h, --help                     Print this help</pre>


<pre>Selected option description:

-m (0 - RCSDBEM, 1 - KVS, 2 - METADBCLI, 3 - POSTGRES, 4 - DEFAULT provider)

Specifies store provider for a session. If left out it'll default to value configured in soeprovider.cpp
RCSDBEM - Embedded RocksDB (store access is done directly from the caller's process)
KVS - Optional light-weight embedded key-value store
METADBCLI - Server RocksDB (store access is done by the soemetadbsrv 

-N 
Value size in bytes. 
On the output, only up to first 128 chars of values will be printed. It doesn't mean that the values have been
truncated. -M enables full hex fump. -p turns off printing.

-X [1..6]
These are regression tests used as a quick way to verify the basic functionality.</pre>


<pre>Example invocations:

1. Create a bunch of records in a subset as follows {"l8", "s8", "c8', "sub"}, where
"l8" - cluster name
"s8" - space name
"c8" - container name
"sub" - subset name</pre>


bin/c_test_soe -o l8 -z s8 -c c9 -A
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -B
<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "FIRS 0001" -v "DATA_FIRS_0001"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "FIRS 0002" -v "DATA_FIRS_0002"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "FIRS 0011" -v "DATA_FIRS_0011"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "FIRS 0012" -v "DATA_FIRS_0012"<br>
<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "SECO 0001" -v "DATA_SECO_0001"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "SECO 0002" -v "DATA_SECO_0002"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "SECO 0011" -v "DATA_SECO_0011"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "SECO 0012" -v "DATA_SECO_0012"<br>
<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "THIR 0001" -v "DATA_THIR_0001"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "THIR 0002" -v "DATA_THIR_0002"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "THIR 0011" -v "DATA_THIR_0011"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "THIR 0012" -v "DATA_THIR_0012"<br>
<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "FOUR 0001" -v "DATA_FOUR_0001"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "FOUR 0002" -v "DATA_FOUR_0002"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "FOUR 0011" -v "DATA_FOUR_0011"<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -D -k "FOUR 0012" -v "DATA_FOUR_0012"<br>
<br>
2. Get all the key-value pairs by using an unbounded range<br>
<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -F -k "" -e "" <br>
key(9,FIRS 0001) value(14,DATA_FIRS_0001)<br>
key(9,FIRS 0002) value(14,DATA_FIRS_0002)<br>
key(9,FIRS 0011) value(14,DATA_FIRS_0011)<br>
key(9,FIRS 0012) value(14,DATA_FIRS_0012)<br>
key(9,FOUR 0001) value(14,DATA_FOUR_0001)<br>
key(9,FOUR 0002) value(14,DATA_FOUR_0002)<br>
key(9,FOUR 0011) value(14,DATA_FOUR_0011)<br>
key(9,FOUR 0012) value(14,DATA_FOUR_0012)<br>
key(9,SECO 0001) value(14,DATA_SECO_0001)<br>
key(9,SECO 0002) value(14,DATA_SECO_0002)<br>
key(9,SECO 0011) value(14,DATA_SECO_0011)<br>
key(9,SECO 0012) value(14,DATA_SECO_0012)<br>
key(9,THIR 0001) value(14,DATA_THIR_0001)<br>
key(9,THIR 0002) value(14,DATA_THIR_0002)<br>
key(9,THIR 0011) value(14,DATA_THIR_0011)<br>
key(9,THIR 0012) value(14,DATA_THIR_0012)<br>
<br>
3. The results are in reverse order when using reverse iterator (-t 1)<br>
<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -F -k "" -e "" -t 1<br>
key(9,THIR 0012) value(14,DATA_THIR_0012)<br>
key(9,THIR 0011) value(14,DATA_THIR_0011)<br>
key(9,THIR 0002) value(14,DATA_THIR_0002)<br>
key(9,THIR 0001) value(14,DATA_THIR_0001)<br>
key(9,SECO 0012) value(14,DATA_SECO_0012)<br>
key(9,SECO 0011) value(14,DATA_SECO_0011)<br>
key(9,SECO 0002) value(14,DATA_SECO_0002)<br>
key(9,SECO 0001) value(14,DATA_SECO_0001)<br>
key(9,FOUR 0012) value(14,DATA_FOUR_0012)<br>
key(9,FOUR 0011) value(14,DATA_FOUR_0011)<br>
key(9,FOUR 0002) value(14,DATA_FOUR_0002)<br>
key(9,FOUR 0001) value(14,DATA_FOUR_0001)<br>
key(9,FIRS 0012) value(14,DATA_FIRS_0012)<br>
key(9,FIRS 0011) value(14,DATA_FIRS_0011)<br>
key(9,FIRS 0002) value(14,DATA_FIRS_0002)<br>
key(9,FIRS 0001) value(14,DATA_FIRS_0001)<br>
<br>
4. Query for specific range<br>
<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -F -k "SECO" -e "" <br>
key(9,SECO 0001) value(14,DATA_SECO_0001)<br>
key(9,SECO 0002) value(14,DATA_SECO_0002)<br>
key(9,SECO 0011) value(14,DATA_SECO_0011)<br>
key(9,SECO 0012) value(14,DATA_SECO_0012)<br>
key(9,THIR 0001) value(14,DATA_THIR_0001)<br>
key(9,THIR 0002) value(14,DATA_THIR_0002)<br>
key(9,THIR 0011) value(14,DATA_THIR_0011)<br>
key(9,THIR 0012) value(14,DATA_THIR_0012)<br>
<br>
bin/c_test_soe -o l8 -z s8 -c c9 -d sub -F -k "SECO" -e "THIR 0000" <br>
key(9,SECO 0001) value(14,DATA_SECO_0001)<br>
key(9,SECO 0002) value(14,DATA_SECO_0002)<br>
key(9,SECO 0011) value(14,DATA_SECO_0011)<br>
key(9,SECO 0012) value(14,DATA_SECO_0012)<br>
<br>
<br>
<br>
</p>
<h2>C++ Test application to test data integrity and futures</h2>
<p>

<p>Example application "integrity_test" is located in:
soe/soeapi/integrity_test</p>


<p>To build test application simply run "make release" in integrity_test directory</p>


<p>integrity_test application options:</p>


<pre>Usage integrity_test [options]
  -o, --cluster-name             Soe cluster name
  -z, --space-name               Soe space name
  -c, --store-name               Soe store name
  -l, --transactional            Soe transactional store (default 0 - non-transactional)
  -n, --num-ops                  Number of ops (default = 1)
  -W, --num-loops                Number rof loop count in each regression (default =1)
  -k, --key                      Key (default key_1)
  -e, --end-key                  End key (default key_1)
  -t, --iterator-dir             Iterator dir (0 - forward(default), 1 - reverse)
  -u, --default-first-iter-arg   Use iterator's First() args defaults
  -v, --value                    Value (default value_1)
  -d  --snap-backup-subset-name  Snapshot, backup or subset name (default snapshot_1, backup_1, subset_1)
  -f  --snap-back-id             Snapshot or backup id (default 0)
  -i, --sync-type                Sync type (0 - default, 1 - fdatasync, 2 - async WAL)
  -m, --provider                 Store provider (0 - RCSDBEM, 1 - KVS, 2 - METADBCLI, 3 - POSTGRES, 4 - DEFAULT provider)
  -j, --debug                    Debug (default 0)
  -M, --hex-dump                 Do hex dump of read records (1 - hex print, 2 - pipe to stdout)
  -X, --regression               IntegrityRegression test
                                 1 - IntegrityRegression1 open/create/delete session/subset/sess_it/sub_it (default)
                                 2 - IntegrityRegression2 write/read/verify session/subset records
                                 3 - IntegrityRegression3 not yet defined
                                 4 - AsyncPut() test
                                 5 - AsyncGet() test
                                 6 - AsyncDelete() test
                                 7 - AsyncMerge() test
                                 8 - AsyncPutSet() test
                                 9 - AsyncGetSet() test
                                 10 - AsyncDeleteSet() test
                                 11 - AsyncMergeSet() test
                                 12 - AsyncConcurrentPutSet() test
                                 13 - AsyncConcurrentGetSet() test
                                 14 - AsyncConcurrentDeleteSet() test
                                 15 - AsyncConcurrentMergeSet() test
                                 16 - AsyncSyncMixedSetOpenClose() test
                                 17 - AsyncSyncMixedSet() test
                                 18 - AsyncPrematureCloseSet() test
                                 19 - mixed AsyncPut/AsyncGet() multi-threaded loop test
  -N, --data-size                Set data size to an arbitrary value (will override -v)
  -T, --sleep-usecs              Sleep usecs between tests
  -b, --random-keys              Generate random instead of consecutive keys
  -y, --regression-loop-cnt      Regression loop count
  -r, --regression-thread-cnt    Regression thread count
  -s, --wait_for_key             Wait for key press before exiting out
  -p, --no-print                 No printing key/value (default print)
  -x, --user-name                Run unser user-name
  -h, --help                     Print this help</pre>

<h2>Soecapi C API Overview</h2>
soecapi.hpp<br>
soecapi.cpp<br>
soesessioncapi.cpp<br>
soesessiongroupcapi.cpp<br>
soesessioniteratorcapi.cpp<br>
soesessiontransactioncapi.cpp<br>
soesubsetiteratorcapi.cpp<br>
soesubsetscapi.cpp<br>

<table class=MsoTableGrid border=1 cellspacing=0 cellpadding=0
 style='border-collapse:collapse;border:none'>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal><b>Type name</b></p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border:solid windowtext 1.0pt;
  border-left:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal><b>Description</b></p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>SessionHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Session handle - soecapi.hpp</p>
  <p class=MsoNormal>Gives access to {"Cluster", "Space", "Store", "Provider"}
  space. Users can open, create, destroy and manipulate the contents of KV
  stores through Session API.</p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>GroupHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Group handle - soecapi.hpp</p>
  <p class=MsoNormal>Allows grouping of multiple operations to the same store
  in one batch, i.e. a Group object holds a sequence of edits to be made to a
  store. Invocation of Write method will write the contents to a store. </p>
  <p class=MsoNormal>Group s primarily used to speed up bulk updates. </p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>TransactionHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Transcation handle - soecapi.hpp</p>
  <p class=MsoNormal>Transactions are used to write multiple items in a transactional
  fashion. That ensures not only atomicity and isolation but also resolves
  potential conflicts that may occur when two or more transactions try to
  update the same key(s). </p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>CDuple</p>
  <p class=MsoNormal>CDupleVector</p>
  <p class=MsoNormal>CDuplePair</p>
  <p class=MsoNormal>CDuplePairVector</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>CDuple, CDupleVector, CDuplePair, CDuplePairVector are C
  style counterparts for C++ std containers. Definitions reside in soecapi.hpp</p>
  <p class=MsoNormal>Duple is a wrapper for {Pointer, Size} tuple. Duple's
  storage pointed to by "Pointer" is owned by the caller, so the caller is
  responsible for managing it.</p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>DisjointSubsetHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>DisjointSubset handle - soecapi.hpp</p>
  <p class=MsoNormal>Subsets are used to create lightweight groups within a
  store. DisjointSubset is a group of items within a store with unique keys. </p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>SubsetIteratorHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>SubsetIterator handle - soecapi.hpp</p>
  <p class=MsoNormal>Subsets can be iterated upon using SubsetIterator.
  SubsetIterators are created with a traversal direction and support most
  common iterator methods.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>SessionIteratorHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>SessionIterator handle - soecapi.hpp</p>
  <p class=MsoNormal>SessionIterator allows iterating over the contents of the
  entire store. Typically, when creating a SessionIterator user specifies start
  and end key.</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>FutureHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Future handle for single key-value pair operations - soefuturescapi.hpp</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>SetFutureHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Future handle for vector of key-value pairs operations - soefuturescapi.hpp</p>
  </td>
 </tr>
 <tr style='height:16.1pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>FutureHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:16.1pt'>
  <p class=MsoNormal>Future handle for single key-value pair operations - soefuturescapi.hpp</p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>BackupEngineHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Backup engine handle - soecapi.hpp</p>
  <p class=MsoNormal>BackupEngine is used to create a backup engine, i.e. an
  object that can be used to create or destroy a store backup.</p>
  </td>
 </tr>
 <tr style='height:14.75pt'>
  <td width=127 valign=top style='width:126.9pt;border:solid windowtext 1.0pt;
  border-top:none;padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>SnapshotEngineHND</p>
  </td>
  <td width=293 valign=top style='width:292.5pt;border-top:none;border-left:
  none;border-bottom:solid windowtext 1.0pt;border-right:solid windowtext 1.0pt;
  padding:0in 5.4pt 0in 5.4pt;height:14.75pt'>
  <p class=MsoNormal>Snapshot engine handle - soecapi.hpp</p>
  <p class=MsoNormal>SnaphotEngine is used to create a snapshot engine, i.e. an
  object that can be used to create or destroy a store snapshot.</p>
  </td>
 </tr>
</table>
<h2>Example C++ application</h2>
<p>

<p>Example application "c_test_soe" is located in:
soe/soeapi/c_test</p>


<p>To build test application simply run "make release" in c_test directory</p>


<p>Test application options:</p>

<pre>Usage bin/c_test_soe [options]
  -o, --cluster-name             Soe cluster name
  -z, --space-name               Soe space name
  -c, --store-name               Soe store name
  -l, --transactional            Soe transactional store (default 0 - non-transactional)
  -n, --num-ops                  Number of ops (default = 1)
  -k, --key                      Key (default key_1)
  -e, --end-key                  End key (default key_1)
  -t, --iterator-dir             Iterator dir (0 - forward(default), 1 - reverse)
  -u, --default-first-iter-arg   Use iterator's First() args defaults
  -v, --value                    Value (default value_1)
  -d  --snap-backup-subset-name  Snapshot, backup or subset name (default snapshot_1, backup_1, subset_1)
  -f  --snap-back-id             Snapshot or backup id (default 0)
  -i, --sync-type                Sync type (0 - default, 1 - fdatasync, 2 - async WAL)
  -m, --provider                 Store provider (0 - KVS, 1 - RCSDBEM(default), 2 - METADBCLI, 3 - POSTGRES)
  -j, --debug                    Debug (default 0)
  -A, --create-store             Create cluster/space/store
  -B, --create-subset            Create subset
  -C, --write-store              Write store num_ops (default 1) KV pairs
  -D, --write-subset             Write subset num_ops (default 1) KV pairs
  -E, --read-store               Read all KV items from store
  -F, --read-subset              Read all KV items from subset
  -G, --read-kv-item-store       Read one KV item from store
  -H, --read-kv-item-subset      Read one KV item from subset
  -I, --delete-kv-item-store     Delete one KV item from store
  -J, --delete-kv-item-subset    Delete one KV item from subset
  -K, --destroy-store            Destroy store
  -L, --destroy-subset           Destroy subset
  -M, --hex-dump                 Do hex dump of read records (1 - hex print, 2 - pipe to stdout)
  -X, --regression               Regression test
  -y, --regression-loop-cnt      Regression loop count
  -r, --regression-thread-cnt    Regression thread count
  -p, --wait-for-key             Wait for key pressed (until compaction done)
  -h, --help                     Print this help</pre>

