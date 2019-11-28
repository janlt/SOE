ALL_LIBS := soeutil soemempools soebind soercsdb soelogger soekvs soemsgs soemsgnet soercsdbem soemetadbcli soeapi soemonitors
ALL_SRVS := soemetadbsrv soeapi/c_test soeapi/soeintegrity_test soeapi/soe_backup

PACKAGE := README.all

.PHONY: all $(PACKAGE) $(ALL_LIBS) $(ALL_SRVS)

all: $(ALL_LIBS) $(ALL_SRVS) 

$(PACKAGE) $(ALL_LIBS):
	$(MAKE) --directory=$@
	touch README.all

$(ALL_SRVS): 
	$(MAKE) --directory=$@

clean:
	for d in $(ALL_LIBS); \
	do                               \
	    $(MAKE) clean --directory=$$d;       \
	done
	for d in $(ALL_SRVS); \
	do                               \
	    $(MAKE) clean --directory=$$d;       \
	done
	rm README.all

install:
	if [ ! -d /var/KVS_cluster ]; then mkdir /var/KVS_cluster; chmod 777 /var/KVS_cluster; else echo "/var/KVS_cluster already exists..."; fi
	if [ ! -d /var/RCSDB_cluster ]; then mkdir /var/RCSDB_cluster; chmod 777 /var/RCSDB_cluster; else echo "/var/RCSDB_cluster already exists...";  fi
	cp soemetadbsrv/bin/soemetadbsrv /usr/bin
	cp `find . -name "libsoe*.so" -print` /usr/lib64
