# Sync module


This kernel module provides distributed DataBase functionallity. It uses DLM, and pacemaker to work.

Setup and create cluster with dlm resource.

kv: kernel module - key and value clustered database.
ckv: char character device, that purpose is to provide simple interface to kv. (to learn more read ckv/README).

# Install:

You can read full tutorial from ClusterLabs [here](https://clusterlabs.org/pacemaker/doc/deprecated/en-US/Pacemaker/1.1/html/Clusters_from_Scratch/).

```
apt-get install pcs

```

add node ip's to /etc/hosts (as node-1, node-2)

```
systemctl start pcsd.service

systemctl enable pcsd.service

passwd hacluster                  (this user is created with pcs installation)

pcs host auth node-1 node-2       (use hacluster user)

pcs cluster start                 (on both nodes)

pcs cluster setup mycluster node-1 node-2

crm_attribute -t crm_config -n no-quorum-policy -v ignore

crm_attribute -t crm_config -n stonith-enabled -v false

pcs resource create dlm ocf:pacemaker:controld args="-q0 -f0" allow_stonith_disabled=true op monitor timeout=60 clone interleave=true
```
Cluster is created. You can check its state with `pcs status` command.

Now you can use charecter device to use it (see ckv/README) or use it directly. 

For direct usage of kv you should do one of the folowing:
1) Clone kv and include its headers and build your module with kv.
2) Build kv and load it to the kernel and then used Module.symvers file, produced from kv's build to build your application with it. So, you will be able to use kv's API (exported functions).

Notice that kv will work properly only in correct enviroment (with DLM configured properly).


