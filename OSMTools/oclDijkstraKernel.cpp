//
// Book:      OpenCL(R) Programming Guide
// Authors:   Aaftab Munshi, Benedict Gaster, Timothy Mattson, James Fung, Dan Ginsburg
// ISBN-10:   0-321-74964-2
// ISBN-13:   978-0-321-74964-2
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780132488006/
//            http://www.openclprogrammingguide.com
//

//
//
//  Description:
//      Implementation of Dijkstra's Single-Source Shortest Path (SSSP) algorithm on the GPU.
//      The basis of this implementation is the paper:
//
//          "Accelerating large graph algorithms on the GPU using CUDA" by
//          Parwan Harish and P.J. Narayanan
//
//
//  Author:
//      Dan Ginsburg
//      <daniel.ginsburg@childrens.harvard.edu>
//
//  Children's Hospital Boston
//
#include <float.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <pthread.h>
#include <boost/thread.hpp>
#include <sstream>
#include <iostream>
#include <fstream>
#include "oclDijkstraKernel.h"

#define MAX_SOURCE_SIZE (0x100000)

///
//  Macros
//
#define checkError(a, b) checkErrorFileLine(a, b, __FILE__ , __LINE__)

///
//  Macro Options
//
#define NUM_ASYNCHRONOUS_ITERATIONS 10  // Number of async loop iterations before attempting to read results back

///



///
//  Namespaces
//
using namespace std;


///
//  Globals
//
//pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
boost::mutex mtx;

///////////////////////////////////////////////////////////////////////////////
//
//  Private Functions
//
//

///
/// Load and build an OpenCL program from source file
/// \param gpuContext GPU context on which to load and build the program
/// \param fileName File name of source file that holds the kernels
/// \return Handle to the program
///
cl_program loadAndBuildProgram( cl_context gpuContext, const char *fileName )
{
    //pthread_mutex_lock(&mutex1);
    mtx.lock();

    cl_int errNum;
    cl_program program;

    // Load the OpenCL source code from the .cl file
    char file_path[1024];
    strncpy(file_path, cl_dir, 1024);
    strcat(file_path, fileName);
    std::ifstream kernelFile(file_path, std::ios::in);
    if (!kernelFile.is_open())
    {
        std::cerr << "Failed to open file for reading: " << fileName << std::endl;
        return NULL;
    }

    std::ostringstream oss;
    oss << kernelFile.rdbuf();

    std::string srcStdStr = oss.str();
    const char *source = srcStdStr.c_str();

    checkError(source != NULL, true);

    // Create the program for all GPUs in the context
    program = clCreateProgramWithSource(gpuContext, 1, (const char **)&source, NULL, &errNum);
    checkError(errNum, CL_SUCCESS);
    // build the program for all devices on the context
    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        char cBuildLog[10240];
        clGetProgramBuildInfo(program, getFirstDev(gpuContext), CL_PROGRAM_BUILD_LOG,
                              sizeof(cBuildLog), cBuildLog, NULL );

        cerr << cBuildLog << endl;
        checkError(errNum, CL_SUCCESS);
    }

    //pthread_mutex_unlock(&mutex1);
    mtx.unlock();
    return program;
}

///
///  Allocate memory for input CUDA buffers and copy the data into device memory
///
void allocateOCLBuffers(cl_context gpuContext, cl_command_queue commandQueue, GraphData *graph,
                        cl_mem *vertexArrayDevice, cl_mem *edgeArrayDevice, cl_mem *weightArrayDevice,
                        cl_mem *maskArrayDevice, cl_mem *costArrayDevice, cl_mem *updatingCostArrayDevice,
                        size_t globalWorkSize)
{
    cl_int errNum;
    cl_mem hostVertexArrayBuffer;
    cl_mem hostEdgeArrayBuffer;
    cl_mem hostWeightArrayBuffer;

    // First, need to create OpenCL Host buffers that can be copied to device buffers
    hostVertexArrayBuffer = clCreateBuffer(gpuContext, CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR,
                                           sizeof(int) * graph->vertexCount, graph->vertexArray, &errNum);
    checkError(errNum, CL_SUCCESS);

    hostEdgeArrayBuffer = clCreateBuffer(gpuContext, CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR,
                                           sizeof(int) * graph->edgeCount, graph->edgeArray, &errNum);
    checkError(errNum, CL_SUCCESS);

    hostWeightArrayBuffer = clCreateBuffer(gpuContext, CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR,
                                           sizeof(int) * graph->edgeCount, graph->weightArray, &errNum);
    checkError(errNum, CL_SUCCESS);

    // Now create all of the GPU buffers
    *vertexArrayDevice = clCreateBuffer(gpuContext, CL_MEM_READ_ONLY, sizeof(int) * globalWorkSize, NULL, &errNum);
    checkError(errNum, CL_SUCCESS);
    *edgeArrayDevice = clCreateBuffer(gpuContext, CL_MEM_READ_ONLY, sizeof(int) * graph->edgeCount, NULL, &errNum);
    checkError(errNum, CL_SUCCESS);
    *weightArrayDevice = clCreateBuffer(gpuContext, CL_MEM_READ_ONLY, sizeof(int) * graph->edgeCount, NULL, &errNum);
    checkError(errNum, CL_SUCCESS);
    *maskArrayDevice = clCreateBuffer(gpuContext, CL_MEM_READ_WRITE, sizeof(int) * globalWorkSize, NULL, &errNum);
    checkError(errNum, CL_SUCCESS);
    *costArrayDevice = clCreateBuffer(gpuContext, CL_MEM_READ_WRITE, sizeof(int) * globalWorkSize, NULL, &errNum);
    checkError(errNum, CL_SUCCESS);
    *updatingCostArrayDevice = clCreateBuffer(gpuContext, CL_MEM_READ_WRITE, sizeof(int) * globalWorkSize, NULL, &errNum);
    checkError(errNum, CL_SUCCESS);

    // Now queue up the data to be copied to the device
    errNum = clEnqueueCopyBuffer(commandQueue, hostVertexArrayBuffer, *vertexArrayDevice, 0, 0,
                                 sizeof(int) * graph->vertexCount, 0, NULL, NULL);
    checkError(errNum, CL_SUCCESS);

    errNum = clEnqueueCopyBuffer(commandQueue, hostEdgeArrayBuffer, *edgeArrayDevice, 0, 0,
                                 sizeof(int) * graph->edgeCount, 0, NULL, NULL);
    checkError(errNum, CL_SUCCESS);

    errNum = clEnqueueCopyBuffer(commandQueue, hostWeightArrayBuffer, *weightArrayDevice, 0, 0,
                                 sizeof(int) * graph->edgeCount, 0, NULL, NULL);
    checkError(errNum, CL_SUCCESS);

    clReleaseMemObject(hostVertexArrayBuffer);
    clReleaseMemObject(hostEdgeArrayBuffer);
    clReleaseMemObject(hostWeightArrayBuffer);
}

///
/// Initialize OpenCL buffers for single run of Dijkstra
///
void initializeOCLBuffers(cl_command_queue commandQueue,
                          cl_kernel initializeKernel, GraphData *graph,
                          size_t maxWorkGroupSize)
{
    cl_int errNum;
    // Set # of work items in work group and total in 1 dimensional range
    size_t localWorkSize = maxWorkGroupSize;
    size_t globalWorkSize = roundWorkSizeUp(localWorkSize, graph->vertexCount);

    errNum = clEnqueueNDRangeKernel(commandQueue, initializeKernel, 1,
                                    NULL, &globalWorkSize, &localWorkSize,
                                    0, NULL, NULL);
    checkError(errNum, CL_SUCCESS);
}

///
/// Gets the id of the nth device from the context (from the NVIDIA SDK)
///
cl_device_id getDev(cl_context cxGPUContext, unsigned int nr)
{
    size_t szParmDataBytes;
    cl_device_id* cdDevices;

    // get the list of GPU devices associated with context
    clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, 0, NULL,
                     &szParmDataBytes);

    if( szParmDataBytes / sizeof(cl_device_id) < nr )
    {
        return (cl_device_id)-1;
    }

    cdDevices = (cl_device_id*) malloc(szParmDataBytes);

    clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, szParmDataBytes,
                     cdDevices, NULL);

    cl_device_id device = cdDevices[nr];
    free(cdDevices);

    return device;
}


///
/// Gets the id of the first device from the context (from the NVIDIA SDK)
///
cl_device_id getFirstDev(cl_context cxGPUContext)
{
    size_t szParmDataBytes;
    cl_device_id* cdDevices;

    // get the list of GPU devices associated with context
    clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, 0, NULL,
                     &szParmDataBytes);
    cdDevices = (cl_device_id*) malloc(szParmDataBytes);

    clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, szParmDataBytes,
                     cdDevices, NULL);

    cl_device_id first = cdDevices[0];
    free(cdDevices);

    return first;
}

///
/// Gets the id of device with maximal FLOPS from the context (from NVIDIA SDK)
///
cl_device_id getMaxFlopsDev(cl_context cxGPUContext)
{
    size_t szParmDataBytes;
    cl_device_id* cdDevices;

    // get the list of GPU devices associated with context
    clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, 0, NULL, &szParmDataBytes);
    cdDevices = (cl_device_id*) malloc(szParmDataBytes);
    size_t device_count = szParmDataBytes / sizeof(cl_device_id);

    clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, szParmDataBytes, cdDevices, NULL);

    cl_device_id max_flops_device = cdDevices[0];
	int max_flops = 0;

	size_t current_device = 0;

    // CL_DEVICE_MAX_COMPUTE_UNITS
    cl_uint compute_units;
    clGetDeviceInfo(cdDevices[current_device], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);

    // CL_DEVICE_MAX_CLOCK_FREQUENCY
    cl_uint clock_frequency;
    clGetDeviceInfo(cdDevices[current_device], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL);

	max_flops = compute_units * clock_frequency;
	++current_device;

	while( current_device < device_count )
	{
        // CL_DEVICE_MAX_COMPUTE_UNITS
        cl_uint compute_units;
        clGetDeviceInfo(cdDevices[current_device], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);

        // CL_DEVICE_MAX_CLOCK_FREQUENCY
        cl_uint clock_frequency;
        clGetDeviceInfo(cdDevices[current_device], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL);

        int flops = compute_units * clock_frequency;
		if( flops > max_flops )
		{
			max_flops        = flops;
			max_flops_device = cdDevices[current_device];
		}
		++current_device;
	}

    free(cdDevices);

	return max_flops_device;
}

///
/// Check for error condition and exit if found.  Print file and line number
/// of error. (from NVIDIA SDK)
///
void checkErrorFileLine(int errNum, int expected, const char* file,
                        const int lineNumber)
{
    if (errNum != expected)
    {
        cerr << "Line " << lineNumber << " in File " << file << endl;
        exit(1);
    }
}


///
/// Round the local work size up to the next multiple of the size
///
int roundWorkSizeUp(int groupSize, int globalSize)
{
    int remainder = globalSize % groupSize;
    if (remainder == 0)
    {
        return globalSize;
    }
    else
    {
        return globalSize + groupSize - remainder;
    }
}

void runDijkstra( cl_context context, cl_device_id deviceId, GraphData* graph,
                  int *sourceVertices, int *results, int numResults,
                  const std::vector<std::pair<int, int> >& query_to_node,
                  boost::unordered_map<int, std::vector<int> >& node_to_query)
{
    // Create command queue
    cl_int errNum;
    cl_command_queue commandQueue;
    commandQueue = clCreateCommandQueue( context, deviceId, 0, &errNum );
    checkError(errNum, CL_SUCCESS);

    // Program handle
    cl_program program = loadAndBuildProgram( context, "dijkstra.cl" );

    // Get the max workgroup size
    size_t maxWorkGroupSize;
    clGetDeviceInfo(deviceId, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t),
                    &maxWorkGroupSize, NULL);

    cl_device_type dev_type;
    clGetDeviceInfo(deviceId, CL_DEVICE_TYPE, sizeof(dev_type), &dev_type, NULL);
    if (dev_type == CL_DEVICE_TYPE_CPU) {
        maxWorkGroupSize = 128;
    }
    checkError(errNum, CL_SUCCESS);
    cout << "MAX_WORKGROUP_SIZE: " << maxWorkGroupSize << endl;
    cout << "Computing '" << numResults << "' results." << endl;
    // Set # of work items in work group and total in 1 dimensional range
    size_t localWorkSize = maxWorkGroupSize;
    size_t globalWorkSize = roundWorkSizeUp(localWorkSize, graph->vertexCount);

    cl_mem vertexArrayDevice;
    cl_mem edgeArrayDevice;
    cl_mem weightArrayDevice;
    cl_mem maskArrayDevice;
    cl_mem costArrayDevice;
    cl_mem updatingCostArrayDevice;

    // Allocate buffers in Device memory
    allocateOCLBuffers( context, commandQueue, graph, &vertexArrayDevice,
                        &edgeArrayDevice, &weightArrayDevice,
                        &maskArrayDevice, &costArrayDevice,
                        &updatingCostArrayDevice, globalWorkSize);


    // Create the Kernels
    cl_kernel initializeBuffersKernel;
    initializeBuffersKernel = clCreateKernel(program, "initializeBuffers",
                                             &errNum);
    checkError(errNum, CL_SUCCESS);

    // Set the args values and check for errors
    errNum |= clSetKernelArg(initializeBuffersKernel, 0, sizeof(cl_mem),
                             &maskArrayDevice);
    errNum |= clSetKernelArg(initializeBuffersKernel, 1, sizeof(cl_mem),
                             &costArrayDevice);
    errNum |= clSetKernelArg(initializeBuffersKernel, 2, sizeof(cl_mem), &updatingCostArrayDevice);

    // 3 set below in loop
    errNum |= clSetKernelArg(initializeBuffersKernel, 4, sizeof(int),
                             &graph->vertexCount);
    checkError(errNum, CL_SUCCESS);

    // Kernel 1
    cl_kernel ssspKernel1;
    ssspKernel1 = clCreateKernel(program, "OCL_SSSP_KERNEL1", &errNum);
    checkError(errNum, CL_SUCCESS);
    errNum |= clSetKernelArg(ssspKernel1, 0, sizeof(cl_mem), &vertexArrayDevice);
    errNum |= clSetKernelArg(ssspKernel1, 1, sizeof(cl_mem), &edgeArrayDevice);
    errNum |= clSetKernelArg(ssspKernel1, 2, sizeof(cl_mem), &weightArrayDevice);
    errNum |= clSetKernelArg(ssspKernel1, 3, sizeof(cl_mem), &maskArrayDevice);
    errNum |= clSetKernelArg(ssspKernel1, 4, sizeof(cl_mem), &costArrayDevice);
    errNum |= clSetKernelArg(ssspKernel1, 5, sizeof(cl_mem), &updatingCostArrayDevice);
    errNum |= clSetKernelArg(ssspKernel1, 6, sizeof(int), &graph->vertexCount);
    errNum |= clSetKernelArg(ssspKernel1, 7, sizeof(int), &graph->edgeCount);
    checkError(errNum, CL_SUCCESS);

    // Kernel 2
    cl_kernel ssspKernel2;
    ssspKernel2 = clCreateKernel(program, "OCL_SSSP_KERNEL2", &errNum);
    checkError(errNum, CL_SUCCESS);
    errNum |= clSetKernelArg(ssspKernel2, 0, sizeof(cl_mem), &vertexArrayDevice);
    errNum |= clSetKernelArg(ssspKernel2, 1, sizeof(cl_mem), &edgeArrayDevice);
    errNum |= clSetKernelArg(ssspKernel2, 2, sizeof(cl_mem), &weightArrayDevice);
    errNum |= clSetKernelArg(ssspKernel2, 3, sizeof(cl_mem), &maskArrayDevice);
    errNum |= clSetKernelArg(ssspKernel2, 4, sizeof(cl_mem), &costArrayDevice);
    errNum |= clSetKernelArg(ssspKernel2, 5, sizeof(cl_mem), &updatingCostArrayDevice);
    errNum |= clSetKernelArg(ssspKernel2, 6, sizeof(int), &graph->vertexCount);

    checkError(errNum, CL_SUCCESS);

    int *maskArrayHost = (int*) malloc(sizeof(int) * graph->vertexCount);
    int *costsArray = (int*) malloc(sizeof(int) * graph->vertexCount);
    for ( size_t i = 0 ; i < numResults; i++ ) {

        errNum |= clSetKernelArg(initializeBuffersKernel, 3, sizeof(int),
                                 &sourceVertices[i]);
        checkError(errNum, CL_SUCCESS);

        // Initialize mask array to false, C and U to infiniti
        initializeOCLBuffers( commandQueue, initializeBuffersKernel, graph,
                             maxWorkGroupSize );

        // Read mask array from device -> host
        cl_event readDone;
        errNum = clEnqueueReadBuffer( commandQueue, maskArrayDevice, CL_FALSE,
                                      0, sizeof(int) * graph->vertexCount,
                                      maskArrayHost, 0, NULL, &readDone);
        checkError(errNum, CL_SUCCESS);
        clWaitForEvents(1, &readDone);

        while(!maskArrayEmpty(maskArrayHost, graph->vertexCount))
        {

            // In order to improve performance, we run some number of iterations
            // without reading the results.  This might result in running more iterations
            // than necessary at times, but it will in most cases be faster because
            // we are doing less stalling of the GPU waiting for results.
            for(int asyncIter = 0; asyncIter < NUM_ASYNCHRONOUS_ITERATIONS;
                asyncIter++)
            {
                size_t localWorkSize = maxWorkGroupSize;
                size_t globalWorkSize = roundWorkSizeUp(localWorkSize,
                                                        graph->vertexCount);

                // execute the kernel
                errNum = clEnqueueNDRangeKernel(commandQueue, ssspKernel1, 1, 0,
                                                &globalWorkSize, &localWorkSize,
                                                0, NULL, NULL);
                checkError(errNum, CL_SUCCESS);

                errNum = clEnqueueNDRangeKernel(commandQueue, ssspKernel2, 1, 0,
                                                &globalWorkSize, &localWorkSize,
                                                0, NULL, NULL);
                checkError(errNum, CL_SUCCESS);
            }
            errNum = clEnqueueReadBuffer(commandQueue, maskArrayDevice,
                                         CL_FALSE, 0,
                                         sizeof(int) * graph->vertexCount,
                                         maskArrayHost, 0, NULL, &readDone);
            checkError(errNum, CL_SUCCESS);
            clWaitForEvents(1, &readDone);
        }

        // Copy the result back
        errNum = clEnqueueReadBuffer(commandQueue, costArrayDevice, CL_FALSE, 0,
                                     sizeof(int) * graph->vertexCount,
                                     costsArray, 0, NULL, &readDone);
        checkError(errNum, CL_SUCCESS);
        clWaitForEvents(1, &readDone);
        
        size_t j, k, offset, n_query;
        int node_idx, cost, src = sourceVertices[i];
        std::vector<int>& query_idx = node_to_query[src];
        n_query = query_to_node.size();
        
        for (j=0; j<query_idx.size(); ++j) {
            offset = (size_t)query_idx[j] * n_query;
            for (k=0; k<n_query; ++k) {
                if (query_idx[j] == k) {
                    results[offset+k] = 0;
                } else {
                    node_idx = query_to_node[k].first;
                    cost = costsArray[node_idx];
                    cost += query_to_node[k].second;
                    cost += query_to_node[query_idx[j]].second;
                    results[offset+k] = cost;
                }
            }
        }
    }

    free (maskArrayHost);
    free (costsArray);

    clReleaseMemObject(vertexArrayDevice);
    clReleaseMemObject(edgeArrayDevice);
    clReleaseMemObject(weightArrayDevice);
    clReleaseMemObject(maskArrayDevice);
    clReleaseMemObject(costArrayDevice);
    clReleaseMemObject(updatingCostArrayDevice);

    clReleaseKernel(initializeBuffersKernel);
    clReleaseKernel(ssspKernel1);
    clReleaseKernel(ssspKernel2);

    clReleaseCommandQueue(commandQueue);
    clReleaseProgram(program);
    cout << "Computed '" << numResults << "' results" << endl;

}


void runDijkstraMultiGPU( const char* dir, cl_context gpuContext,
                          GraphData* graph, int *sourceVertices,
                          int *outResultCosts, int numResults,
                    const std::vector<std::pair<int, int> >& query_to_node,
                    boost::unordered_map<int, std::vector<int> >& node_to_query)
{
    strcpy(cl_dir, dir);
    
    // Find out how many GPU's to compute on all available GPUs
    cl_int errNum;
    size_t deviceBytes;
    cl_uint deviceCount;

    errNum = clGetContextInfo(gpuContext, CL_CONTEXT_DEVICES, 0, NULL, &deviceBytes);
    checkError(errNum, CL_SUCCESS);
    deviceCount = (cl_uint)deviceBytes/sizeof(cl_device_id);

    if (deviceCount == 0) {
        cerr << "ERROR: no GPUs present!" << endl;
        return;
    }

    DevicePlan *devicePlans = (DevicePlan*) malloc(sizeof(DevicePlan) * deviceCount);
    boost::thread* bthread[deviceCount];

    // Divide the workload out per device
    int resultsPerDevice = numResults / deviceCount;

    size_t offset = 0;

    for (unsigned int i = 0; i < deviceCount; i++) {
        devicePlans[i].context = gpuContext;
        devicePlans[i].deviceId = getDev(gpuContext, i);
        devicePlans[i].graph = graph;
        devicePlans[i].sourceVertices = &sourceVertices[offset];
        devicePlans[i].outResultCosts = outResultCosts;
        devicePlans[i].numResults = resultsPerDevice;

        offset += resultsPerDevice;
    }

    // Add any remaining work to the last GPU
    if (offset < numResults) {
        devicePlans[deviceCount - 1].numResults += (numResults - offset);
    }

    // Launch all the threads
    for (unsigned int i = 0; i < deviceCount; i++) {
        bthread[i] = new boost::thread(&runDijkstra,
                                       (cl_context) devicePlans[i].context,
                                       (cl_device_id) devicePlans[i].deviceId,
                                       (GraphData*) devicePlans[i].graph,
                                       (int*) devicePlans[i].sourceVertices,
                                       (int*) devicePlans[i].outResultCosts,
                                       devicePlans[i].numResults,
                                       boost::ref(query_to_node),
                                       boost::ref(node_to_query));
    }

    // Wait for the results from all threads
    for (unsigned int i = 0; i < deviceCount; i++) {
        bthread[i]->join();
    }

    for (unsigned int i = 0; i < deviceCount; i++) {
        delete bthread[i];
    }

    free (devicePlans);
}


///
/// Check whether the mask array is empty.  This tells the algorithm whether
/// it needs to continue running or not.
///
bool maskArrayEmpty(int *maskArray, int count)
{
    for(int i = 0; i < count; i++ )
    {
        if (maskArray[i] == 1)
        {
            return false;
        }
    }

    return true;
}


void runDijkstraRef( GraphData* graph, int *sourceVertices,
                     int *outResultCosts, int start, int end)
{

    // Create the arrays needed for processing the algorithm
    int *costArray = new int [graph->vertexCount];
    int *updatingCostArray = new int[graph->vertexCount];
    int *maskArray = new int[graph->vertexCount];

    for (size_t i = start; i <= end; i++)
    {
        // Initialize the buffer for this run
        for (int v = 0; v < graph->vertexCount; v++)
        {
            if (v == sourceVertices[i])
            {
                maskArray[v] = 1;
                costArray[v] = 0;
                updatingCostArray[v] = 0;
            }
            else
            {
                maskArray[v] = 0;
                costArray[v] = INT_MAX;
                updatingCostArray[v] = INT_MAX;
            }
        }

        while(!maskArrayEmpty(maskArray, graph->vertexCount))
        {
            // Equivalent of OCL_SSSP_KERNEL1()
            for (int tid = 0; tid < graph->vertexCount; tid++)
            {
                if ( maskArray[tid] != 0 )
                {
                    maskArray[tid] = 0;

                    int edgeStart = graph->vertexArray[tid];
                    int edgeEnd;
                    if (tid + 1 < (graph->vertexCount))
                    {
                        edgeEnd = graph->vertexArray[tid + 1];
                    }
                    else
                    {
                        edgeEnd = graph->edgeCount;
                    }

                    for(int edge = edgeStart; edge < edgeEnd; edge++)
                    {
                        int nid = graph->edgeArray[edge];

                        // One note here: whereas the paper specified weightArray[nid], I
                        //  found that the correct thing to do was weightArray[edge].  I think
                        //  this was a typo in the paper.  Either that, or I misunderstood
                        //  the data structure.
                        if (updatingCostArray[nid] > (costArray[tid] + graph->weightArray[edge]))
                        {
                            updatingCostArray[nid] = (costArray[tid] + graph->weightArray[edge]);
                        }
                    }
                }
            }

            // Equivalent of OCL_SSSP_KERNEL2()
            for (int tid = 0; tid < graph->vertexCount; tid++)
            {
                if (costArray[tid] > updatingCostArray[tid])
                {
                    costArray[tid] = updatingCostArray[tid];
                    maskArray[tid] = 1;
                }

                updatingCostArray[tid] = costArray[tid];
            }
        }

        // Copy the result back
        memcpy(&outResultCosts[i * (size_t)graph->vertexCount], costArray, sizeof(int) * graph->vertexCount);
    }

    // Free temporary computation buffers
    delete [] costArray;
    delete [] updatingCostArray;
    delete [] maskArray;
}




//////////////////

// A utility function to create a new adjacency list node
struct AdjListNode* newAdjListNode(int dest, int weight)
{
    struct AdjListNode* newNode =
    (struct AdjListNode*) malloc(sizeof(struct AdjListNode));
    newNode->dest = dest;
    newNode->weight = weight;
    newNode->next = NULL;
    return newNode;
}

// A utility function that creates a graph of V vertices
struct CPUGraph* createGraph(int V)
{
    struct CPUGraph* graph = (struct CPUGraph*) malloc(sizeof(struct CPUGraph));
    graph->V = V;

    // Create an array of adjacency lists.  Size of array will be V
    graph->array = (struct AdjList*) malloc(V * sizeof(struct AdjList));

    // Initialize each adjacency list as empty by making head as NULL
    for (int i = 0; i < V; ++i)
        graph->array[i].head = NULL;

    return graph;
}

// Adds an edge to an undirected graph
void addEdgeToGraph(struct CPUGraph* graph, int src, int dest, int weight)
{
    // Add an edge from src to dest.  A new node is added to the adjacency
    // list of src.  The node is added at the begining
    struct AdjListNode* newNode = newAdjListNode(dest, weight);
    newNode->next = graph->array[src].head;
    graph->array[src].head = newNode;

    // Since graph is undirected, add an edge from dest to src also
    newNode = newAdjListNode(src, weight);
    newNode->next = graph->array[dest].head;
    graph->array[dest].head = newNode;
}

// A utility function to create a new Min Heap Node
struct MinHeapNode* newMinHeapNode(int v, int dist)
{
    MinHeapNode* minHeapNode = new MinHeapNode();
        //(struct MinHeapNode*) malloc(sizeof(struct MinHeapNode));
    minHeapNode->v = v;
    minHeapNode->dist = dist;
    return minHeapNode;
}

// A utility function to create a Min Heap
struct MinHeap* createMinHeap(int capacity)
{
    MinHeap* minHeap = new MinHeap();
    minHeap->pos = new int[capacity];
    minHeap->size = 0;
    minHeap->capacity = capacity;
    minHeap->array = new MinHeapNode*[capacity];
        //(struct MinHeapNode**) malloc(capacity * sizeof(struct MinHeapNode*));
    return minHeap;
}

/* free a minHeap struct*/
void freeMinheap(MinHeap* minHeap) {
    free(minHeap->pos);
    free(minHeap->array);
    free(minHeap);
}

// A utility function to swap two nodes of min heap. Needed for min heapify
void swapMinHeapNode(struct MinHeapNode** a, struct MinHeapNode** b)
{
    struct MinHeapNode t;
    t.v = (*a)->v;
    t.dist = (*a)->dist;

    (*a)->v = (*b)->v;
    (*a)->dist = (*b)->dist;

    (*b)->v = t.v;
    (*b)->dist = t.dist;
}

// A standard function to heapify at given idx
// This function also updates position of nodes when they are swapped.
// Position is needed for decreaseKey()
void minHeapify(struct MinHeap* minHeap, int idx)
{
    int smallest, left, right;
    smallest = idx;
    left = 2 * idx + 1;
    right = 2 * idx + 2;

    if (left < minHeap->size &&
        minHeap->array[left]->dist < minHeap->array[smallest]->dist )
        smallest = left;

    if (right < minHeap->size &&
        minHeap->array[right]->dist < minHeap->array[smallest]->dist )
        smallest = right;

    if (smallest != idx)
        {
        // The nodes to be swapped in min heap
        MinHeapNode *smallestNode = minHeap->array[smallest];
        MinHeapNode *idxNode = minHeap->array[idx];

        // Swap positions
        minHeap->pos[smallestNode->v] = idx;
        minHeap->pos[idxNode->v] = smallest;

        // Swap nodes
        swapMinHeapNode(&minHeap->array[smallest], &minHeap->array[idx]);

        minHeapify(minHeap, smallest);
        }
}
// A utility function to check if the given minHeap is ampty or not
int isEmpty(struct MinHeap* minHeap)
{
    return minHeap->size == 0;
}

// Standard function to extract minimum node from heap
struct MinHeapNode* extractMin(struct MinHeap* minHeap)
{
    if (isEmpty(minHeap))
        return NULL;

    // Store the root node
    struct MinHeapNode* root = minHeap->array[0];

    // Replace root node with last node
    struct MinHeapNode* lastNode = minHeap->array[minHeap->size - 1];
    minHeap->array[0] = lastNode;

    // Update position of last node
    minHeap->pos[root->v] = minHeap->size-1;
    minHeap->pos[lastNode->v] = 0;

    // Reduce heap size and heapify root
    --minHeap->size;
    minHeapify(minHeap, 0);

    return root;
}

// Function to decreasy dist value of a given vertex v. This function
// uses pos[] of min heap to get the current index of node in min heap
void decreaseKey(struct MinHeap* minHeap, int v, int dist)
{
    // Get the index of v in  heap array
    int i = minHeap->pos[v];

    // Get the node and update its dist value
    minHeap->array[i]->dist = dist;

    // Travel up while the complete tree is not hepified.
    // This is a O(Logn) loop
    while (i && minHeap->array[i]->dist < minHeap->array[(i - 1) / 2]->dist)
        {
        // Swap this node with its parent
        minHeap->pos[minHeap->array[i]->v] = (i-1)/2;
        minHeap->pos[minHeap->array[(i-1)/2]->v] = i;
        swapMinHeapNode(&minHeap->array[i],  &minHeap->array[(i - 1) / 2]);

        // move to parent index
        i = (i - 1) / 2;
        }
}

// A utility function to check if a given vertex
// 'v' is in min heap or not
bool isInMinHeap(struct MinHeap *minHeap, int v)
{
    if (minHeap->pos[v] < minHeap->size)
        return true;
    return false;
}

// A utility function used to print the solution
void printArr(int dist[], int n)
{
    printf("Vertex   Distance from Source\n");
    for (int i = 0; i < n; ++i)
        printf("%d \t\t %d\n", i, dist[i]);
}

// The main function that calulates distances of shortest paths from src to all
// vertices. It is a O(ELogV) function
void dijkstra(struct CPUGraph* graph, int src, int* results,
              const std::vector<std::pair<int, int> >& query_to_node,
              boost::unordered_map<int, std::vector<int> >& node_to_query,
              int* path)
{
    int V = graph->V;// Get the number of vertices in graph
    int* dist = new int[V];      // dist values used to pick minimum weight edge in cut

    // minHeap represents set E
    struct MinHeap* minHeap = createMinHeap(V);

    // Initialize min heap with all vertices. dist value of all vertices
    for (int v = 0; v < V; ++v) {
        if (path) path[v] = -1;
        dist[v] = INT_MAX;
        minHeap->array[v] = newMinHeapNode(v, dist[v]);
        minHeap->pos[v] = v;
    }

    // Make dist value of src vertex as 0 so that it is extracted first
    minHeap->array[src] = newMinHeapNode(src, dist[src]);
    minHeap->pos[src]   = src;
    dist[src] = 0;
    decreaseKey(minHeap, src, dist[src]);

    // Initially size of min heap is equal to V
    minHeap->size = V;

    // In the followin loop, min heap contains all nodes
    // whose shortest distance is not yet finalized.
    while (!isEmpty(minHeap)) {
        // Extract the vertex with minimum distance value
        struct MinHeapNode* minHeapNode = extractMin(minHeap);
        int u = minHeapNode->v; // Store the extracted vertex number

        // Traverse through all adjacent vertices of u (the extracted
        // vertex) and update their distance values
        struct AdjListNode* pCrawl = graph->array[u].head;
        while (pCrawl != NULL) {
            int v = pCrawl->dest;

            // If shortest distance to v is not finalized yet, and distance to v
            // through u is less than its previously calculated distance
            if (isInMinHeap(minHeap, v) && dist[u] != INT_MAX &&
                pCrawl->weight + dist[u] < dist[v])
            {
                if (path) path[v] = u;
                dist[v] = dist[u] + pCrawl->weight;

                // update distance value in min heap also
                decreaseKey(minHeap, v, dist[v]);
            }
            pCrawl = pCrawl->next;
        }
    }

    if (path) {
        // return path
        memcpy(results, dist, sizeof(int) * V);
        
    } else {
        if (node_to_query.empty() && query_to_node.empty()) {
            // return dist as results
            memcpy(results, dist, sizeof(int) * V);

        }  else {
            // map query points to node points
            size_t i, j, offset, n_query;
            int node_idx, cost;

            std::vector<int>& query_idx = node_to_query[src];
            n_query = query_to_node.size();

            for (i=0; i<query_idx.size(); ++i) {
                offset = (size_t)query_idx[i] * n_query;
                for (j=0; j<n_query; ++j) {
                    if (query_idx[i] == j) {
                        results[offset+j] = 0;
                    } else {
                        node_idx = query_to_node[j].first;
                        cost = dist[node_idx];
                        cost += query_to_node[j].second;
                        cost += query_to_node[query_idx[i]].second;
                        results[offset+j] = cost;
                    }
                }
            }
        }
    }
    
    //free allocated memory
    delete[] minHeap->pos;
    for (int i=1; i<minHeap->capacity; i++) delete minHeap->array[i];
    delete[] minHeap->array;
    delete minHeap;
    delete[] dist;
}

void freeGraph(CPUGraph* graph)
{
    struct AdjListNode *AdjListNode;
    struct AdjListNode *temp;
    for (int i = 0; i < graph->V; i++) {
        if (graph->array[i].head) {
            AdjListNode = graph->array[i].head;
            while (AdjListNode) {
                temp = AdjListNode->next;
                free(AdjListNode);
                AdjListNode = temp;
            }

        }
    }
    free(graph->array);
    free(graph);
}
