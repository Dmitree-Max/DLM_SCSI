# Char device driver


This driver supposed to store keys and thier locks.

# Install:

```
make 
insmod ckv.ko

...interaction...

rmmod ckv
```

-`make`: compile and link

-`insmod ckv.ko`: to include new module into kernel, you will have `/dev/ckv` device

-`rmmod ckv`:  to remove module

# Possible interaction:

- `cat /dev/ckv`: to print whole state
- `echo 'add-key key value' > /dev/ckv`: to add key 'key' with value 'value' or update 'key' with new value 'value'
- `echo 'remove-key key' > /dev/ckv`:  to remove key
- `echo 'lock-key key' > /dev/ckv`: to lock key (will be locked according to locking machine id)
- `echo 'unlock-key key' > /dev/ckv`: to unlock key 


Usage logic:
1) Locked key can be updated or unlocked only by node that had locked it.
2) One key can have 1 or zero locks.
3) Wrong commands will have no effect.
4) State on all nodes of cluster is always the same.
