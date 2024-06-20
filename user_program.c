#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define IOCTL_ALLOCATE_MEMORY _IOW('a', 'a', size_t *)
#define IOCTL_STORE_DATA _IOW('a', 'b', char *)
#define IOCTL_LOAD_DATA _IOR('a', 'c', char *)

int main()
{
    int fd;
    size_t memory_size;
    char *buffer;
    char *read_buffer;
    FILE *input_file;
    FILE *output_file;

    fd = open("/dev/mem_device", O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open device");
        return -1;
    }

    // 申请内存空间
    memory_size = 1024; // 示例大小
    if (ioctl(fd, IOCTL_ALLOCATE_MEMORY, &memory_size) < 0)
    {
        perror("Failed to allocate memory");
        close(fd);
        return -1;
    }

    // 读取文件并存储到内核模块的内存中
    buffer = (char *)malloc(memory_size);
    input_file = fopen("input.txt", "r");
    if (!input_file)
    {
        perror("Failed to open input file");
        close(fd);
        return -1;
    }
    fread(buffer, 1, memory_size, input_file);
    fclose(input_file);

    if (ioctl(fd, IOCTL_STORE_DATA, buffer) < 0)
    {
        perror("Failed to store data");
        free(buffer);
        close(fd);
        return -1;
    }

    // 从内核模块的内存中读取数据并写入文件
    read_buffer = (char *)malloc(memory_size);
    if (ioctl(fd, IOCTL_LOAD_DATA, read_buffer) < 0)
    {
        perror("Failed to load data");
        free(buffer);
        free(read_buffer);
        close(fd);
        return -1;
    }

    output_file = fopen("output.txt", "w");
    if (!output_file)
    {
        perror("Failed to open output file");
        free(buffer);
        free(read_buffer);
        close(fd);
        return -1;
    }
    fwrite(read_buffer, 1, memory_size, output_file);
    fclose(output_file);

    // 释放内存
    free(buffer);
    free(read_buffer);
    close(fd);

    return 0;
}
