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
	[ ! -z /var/KVS_cluster ] && mkdir /var/KVS_cluster && chmod 777 /var/KVS_cluster	
	[ ! -z /var/RCSDB_cluster ] && mkdir /var/RCSDB_cluster && chmod 777 /var/RCSDB_cluster	
	cp metadbsrv/bin/soemetadbsrv /usr/bin
