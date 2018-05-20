#include <stdio.h>
#include <iostream>
#include <math.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MAX_SOURCE_SIZE (0x100000)

#include "../ShapeOperations/GalWeight.h"

using namespace std;

char * toArray(int number)
{
    int n = log10(number) + 1;
    int i;
    char *numberArray = (char*)calloc(n, sizeof(char));
    for ( i = 0; i < n; ++i, number /= 10 )
    {
        numberArray[i] = number % 10;
    }
    return numberArray;
}

char *replace_str(char *str, char *orig, char *rep, int start)
{
    static char temp[4096];
    static char buffer[4096];
    char *p;
    
    strcpy(temp, str + start);
    
    if(!(p = strstr(temp, orig)))  // Is 'orig' even in 'temp'?
        return temp;
    
    strncpy(buffer, temp, p-temp); // Copy characters from 'temp' start to 'orig' str
    buffer[p-temp] = '\0';
    
    sprintf(buffer + (p - temp), "%s%s", rep, p + strlen(orig));
    sprintf(str + start, "%s", buffer);
    
    return str;
}

void gpu_lisa(const char* cl_path, int rows, int permutations, unsigned long long last_seed_used, double* values, double* local_moran, GalElement* w, double* p)
{
    int max_n_nbrs = 0;
    int* num_nbrs = new int[rows];
    int total_nbrs = 0;
    
    for (size_t i=0; i<rows; i++) {
        int nnbrs = w[i].Size();
        if (nnbrs > max_n_nbrs) {
            max_n_nbrs = nnbrs;
        }
        num_nbrs[i] = nnbrs;
        total_nbrs += nnbrs;
    }
    
    int* nbr_idx = new int[total_nbrs];
    size_t idx = 0;
    
    for (size_t i=0; i<rows; i++) {
        int nnbrs = w[i].Size();
        for (size_t j=0; j<nnbrs; j++) {
            nbr_idx[idx++] = w[i][j];
        }
    }
    
    // Load the kernel source code into the array source_str
    FILE *fp;
    char *source_str;
    size_t source_size;
    
    fp = fopen(cl_path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose( fp );
    
    // replace 12345 with max_n_nbrs
    //replace_str(source_str, "12345", toArray(max_n_nbrs), 0);
    
    // Get platform and device information
    cl_platform_id platform_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    
    cl_uint maxDevices = 10;
    cl_device_id* devices = new cl_device_id[maxDevices];
    cl_uint nrDevices;
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, maxDevices, devices, &ret_num_devices);
    cl_device_id device_id = devices[0];
    if (ret_num_devices==2) {
        device_id = devices[1];
    }
    //ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_ALL, 1, &device_id, &ret_num_devices);
    
    // Create an OpenCL context
    cl_context context = clCreateContext( NULL, ret_num_devices, devices, NULL, NULL, &ret);
    
    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    
    // Create memory buffers on the device for each vector
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                      sizeof(double)*rows, NULL, &ret);
    cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                      sizeof(double)*rows, NULL, &ret);
    cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                      sizeof(int)*rows, NULL, &ret);
    cl_mem d_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                      sizeof(int)*total_nbrs, NULL, &ret);
    cl_mem p_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                      sizeof(double)*rows, NULL, &ret);
    
    // Copy the lists A and B to their respective memory buffers
    ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0, sizeof(double)*rows,
                               values, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0, sizeof(double)*rows,
                               local_moran, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, c_mem_obj, CL_TRUE, 0, sizeof(int)*rows,
                               num_nbrs, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, d_mem_obj, CL_TRUE, 0, sizeof(int)*total_nbrs,
                               nbr_idx, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, p_mem_obj, CL_TRUE, 0, sizeof(double)*rows,
                               p, 0, NULL, NULL);
    
    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
                                                   (const char **)&source_str, (const size_t *)&source_size, &ret);
    
    // Build the program
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "lisa", &ret);
    
    // Set the arguments of the kernel
    ret = clSetKernelArg(kernel, 0, sizeof(cl_int), (void *)&rows);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&permutations);
    ret = clSetKernelArg(kernel, 2, sizeof(cl_ulong), (void *)&last_seed_used);
    ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&a_mem_obj);
    ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&b_mem_obj);
    ret = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&c_mem_obj);
    ret = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&d_mem_obj);
    ret = clSetKernelArg(kernel, 7, sizeof(cl_mem), (void *)&p_mem_obj);
    
    // Execute the OpenCL kernel on the list
    size_t global_item_size = 256 * ceil(rows/256.0); // Process the entire lists
    size_t local_item_size = 256; // Process in groups of 64
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
                                 &global_item_size, &local_item_size, 0, NULL, NULL);
    
    // Read the memory buffer C on the device to the local variable C
    ret = clEnqueueReadBuffer(command_queue, p_mem_obj, CL_TRUE, 0,
                              sizeof(double) * rows, p, 0, NULL, NULL);
    
    // Display the result to the screen
    for(size_t i = 0; i < 20; i++)
        printf("%f\n", p[i]);
    
    // Clean up
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(a_mem_obj);
    ret = clReleaseMemObject(b_mem_obj);
    ret = clReleaseMemObject(c_mem_obj);
    ret = clReleaseMemObject(d_mem_obj);
    ret = clReleaseMemObject(p_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);

    delete[] num_nbrs;
    delete[] nbr_idx;
}