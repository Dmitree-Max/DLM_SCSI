# Sync module


This kernel module provides distributed DataBase functionallity. It uses DLM, and pacemaker to work.


Setup and create cluster with dlm resource.


# Install:

```
apt-get install pcs

```

add node ip's to /etc/hosts (as node-1, node-2)

```
systemctl start pcsd.service

systemctl enable pcsd.service

pcs host auth pcmk-1 pcmk-2

pcs cluster setup mycluster node-1 node-2

crm_attribute -t crm_config -n no-quorum-policy -v ignore

crm_attribute -t crm_config -n stonith-enabled -v false

pcs resource create dlm ocf:pacemaker:controld args="-q0 -f0" allow_stonith_disabled=true op monitor timeout=60 clone interleave=true
```

Then run `pcs cluster start` on each node.


Now you can use char device to use it (see ckv/README) or use it directly. For that you should build your module with kv's Module.symvers. 

