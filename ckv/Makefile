obj-m += ckv.o
ckv-objs := ./ckv_src.o ./commands.o ./string_functions.o ../kv/kv.o ../kv/structures.o ../kv/list_interactions.o ../kv/buffer_interactions.o

KDIR = /lib/modules/`uname -r`/build

all:
	make -C $(KDIR) M=$(PWD) modules

 
clean:
	make -C $(KDIR) M=$(PWD) clean
