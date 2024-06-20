# mem-module


编译内核模块程序
make


编译用户态程序
make -f Makefile.user


加载内核模块并创建设备节点：

sudo insmod mem_module.ko

sudo mknod /dev/mem_device c $(grep mem_device /proc/devices | awk '{print $1}') 0
添加input.txt

运行
./user_program
