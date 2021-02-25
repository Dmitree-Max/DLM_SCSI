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

- `cat /dev/ckv`: to see all info
- `echo 'add-key key value' > /dev/ckv`: to add key 'key' with value 'value'
- `echo 'lock-key key' > /dev/ckv`: to lock key (will be locked according to locking machine id)
- `echo 'unlock-key key' > /dev/ckv`: to unlock key 
