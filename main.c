#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "fcntl.h"
#include "unistd.h"
#include "CL/cl.h"

int
main(int argc, char** argv)
{
    cl_int entries = 1, num_platforms = 0;
    char name[128];
    name[127] = '\0';
    cl_platform_id platforms;
    cl_int status = clGetPlatformIDs(entries, &platforms, NULL);
    printf("Status: %d\n", status);
    status = clGetPlatformInfo(platforms, CL_PLATFORM_NAME, 127, &name[0], NULL);
    printf("Platform Name: %s\n", name);
    memset(&name, 0x0, 128);
    status = clGetPlatformInfo(platforms, CL_PLATFORM_VENDOR, 127, &name[0], NULL);
    printf("Vendor: %s\n", name);
    memset(&name, 0x0, 128);
    status = clGetPlatformInfo(platforms, CL_PLATFORM_VERSION, 127, &name[0], NULL);
    printf("Version: %s\n", name);
    memset(&name, 0x0, 128);

    // Get Device Info
    cl_device_id device;
    status = clGetDeviceIDs(platforms, CL_DEVICE_TYPE_GPU, entries, &device, NULL);
    printf("Status: %d\n", status);
    status = clGetDeviceInfo(device, CL_DEVICE_NAME, 128, &name[0], NULL);
    printf("Device Name: %s\n", name);
    memset(&name, 0x0, 128);
    status = clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, 128, &name[0], NULL);
    cl_ulong* test = (cl_ulong*)(&name[0]);
    printf("Global Memory Size: %lu\n", *test);
    memset(&name, 0x0, 128);
    status = clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, 128, &name[0], NULL);
    memset(&name, 0x0, 128);

    // Create Context
    cl_context context;
    cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platforms, 0} ;
    char data[256];
    context = clCreateContext(&properties[0], 1, &device, NULL, &data[0], &status);
    status = clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, 128, &name[0], NULL);
    cl_uint* test1 = (cl_uint*)(&name[0]);
    printf("Context Device Count: %d\n", *test1);
    memset(&name, 0x0, 128);
    memset(&data, 0x0, 256);

    // Create Program
    char line[1024];
    char* text = &line[0];
    const char** strings = &text;
    size_t length = 1024;
    memset(text, 0x0, length);
    int file = open("/home/tomasramos/Repos/OpenCL/kernel.cl", O_RDONLY);
    printf("File: %d\n", file);
    status = read(file, text, length);
    printf("Read Status: %d\n", status);
    const char options[] = "";
    cl_program program = clCreateProgramWithSource(context, 1, strings, &length, &status);
    printf("Create Status: %d\n", status);
    status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 1024, &data[0], NULL);
    printf("Build Status: %d\n", status);
    printf("Log: %s\n", data);

    // Create Queue
    cl_queue_properties queue_properties[] = {CL_QUEUE_PROFILING_ENABLE, 0};
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, &queue_properties[0], &status);
    printf("Queue Creation Status: %d\n", status);

    // Create Kernel
    char* kernel_name = "sum_test\0";
    cl_kernel kernel = clCreateKernel(program, kernel_name, &status);
    printf("Kernel Creation Status: %d\n", status);

    // Create Arguments
    int* a = NULL;
    int* b = NULL;
    int* c = NULL;

    // Device Memory
    cl_mem buffer_a = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int)*1024, NULL, &status);
    cl_mem buffer_b = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int)*1024, NULL, &status);
    cl_mem buffer_c = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int)*1024, NULL, &status);

    // Allocate Host Buffer
    cl_event events[2];
    cl_mem host_a = clCreateBuffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE, sizeof(int)*1024, NULL, &status);
    cl_mem host_b = clCreateBuffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE, sizeof(int)*1024, NULL, &status);
    cl_mem host_c = clCreateBuffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE, sizeof(int)*1024, NULL, &status);

    // Pin Host Memory
    a = clEnqueueMapBuffer(queue, host_a, CL_FALSE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int)*1024, NULL, NULL, NULL, &status);
    printf("\nPinned Memory A Status: %d\n", status);
    for (int i = 0; i < 1024; i++)
        a[i] = 1;
    printf("Pointer to Memory A: %p\n", a);
    printf("First A Item: %d\n", *a);
    status = clEnqueueWriteBuffer(queue, buffer_a, CL_FALSE, 0, sizeof(int)*1024, a, NULL, NULL, &events[0]);
    printf("Write to Device Buffer A Status: %d\n", status);

    b = clEnqueueMapBuffer(queue, host_b, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int)*1024, NULL, NULL, NULL, &status);
    printf("\nPinned Memory B Status: %d\n", status);
    for (int i = 0; i < 1024; i++)
        b[i] = 1;
    printf("Pointer to Memory B: %p\n", b);
    printf("First B Item: %d\n", *b);
    status = clEnqueueWriteBuffer(queue, buffer_b, CL_TRUE, 0, sizeof(int)*1024, b, NULL, NULL, &events[1]);
    printf("Write to Device Buffer B Status: %d\n", status);

    c = clEnqueueMapBuffer(queue, host_c, CL_FALSE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int)*1024, NULL, NULL, NULL, &status);
    printf("\nPinned Memory C Status: %d\n", status);
    memset(c, 0x0, sizeof(int)*1024);

    // Set Kernel Arguments
    clSetKernelArg(kernel, 0, sizeof(buffer_a), &buffer_a);
    clSetKernelArg(kernel, 1, sizeof(buffer_b), &buffer_b);
    clSetKernelArg(kernel, 2, sizeof(buffer_c), &buffer_c);

    // Dispatch Kernel
    size_t global = 1024;
    size_t local = 256;
    cl_event event;
    status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, &local, 2, &events[0], &event);
    printf("\nND Range Kernel Enqueue Status: %d\n", status);

    // Read Output Buffer
    status = clEnqueueReadBuffer(queue, buffer_c, CL_TRUE, 0, sizeof(int)*1024, c, 1, &event, NULL);
    printf("\nRead to Host Buffer C Status: %d\n", status);
    printf("First Item Host Buffer C: %d\n", *c);

    // Count 2s
    int total = 0;
    for (int i = 0; i < 1024; i++)
        if (c[i] == 2)
            total++;

    printf("Count: %d\n", total);

    exit(0);
}
