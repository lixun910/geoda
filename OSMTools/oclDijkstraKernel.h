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
#ifndef DIJKSTRA_KERNEL_H
#define DIJKSTRA_KERNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <boost/unordered_map.hpp>

#ifdef __APPLE__
    #include <OpenCL/cl.h>
#else
    #include <CL/cl.h>
#endif

static char cl_dir[1024];

//  Function prototypes
//
bool maskArrayEmpty(int *maskArray, int count);

///
//  Types
//

///
//  Utility functions adapted from NVIDIA GPU Computing SDK
//
void checkErrorFileLine(int errNum, int expected, const char* file, const int lineNumber);
cl_device_id getDev(cl_context cxGPUContext, unsigned int nr);
cl_device_id getFirstDev(cl_context cxGPUContext);
void checkErrorFileLine(int errNum, int expected, const char* file, const int lineNumber);
int roundWorkSizeUp(int groupSize, int globalSize);

///
//  Types
//
//
//  This data structure and algorithm implementation is based on
//  Accelerating large graph algorithms on the GPU using CUDA by
//  Parwan Harish and P.J. Narayanan
//
typedef struct
{
    // (V) This contains a pointer to the edge list for each vertex
    int *vertexArray;

    // Vertex count
    int vertexCount;

    // (E) This contains pointers to the vertices that each edge is attached to
    int *edgeArray;

    // Edge count
    int edgeCount;

    // (W) Weight array
    int *weightArray;

} GraphData;

// This structure is used in the multi-GPU implementation of the algorithm.
// This structure defines the workload for each GPU.  The code chunks up
// the work on a per-GPU basis.
typedef struct
{
    // Context
    cl_context context;
    
    // Device number to run algorithm on
    cl_device_id deviceId;
    
    // Pointer to graph data
    GraphData *graph;
    
    // Source vertex indices to process
    int *sourceVertices;
    
    // End vertex indices to process
    int *endVertices;
    
    // Results of processing
    int *outResultCosts;
    
    // Number of results
    int numResults;
    
} DevicePlan;


///
/// Run Dijkstra's shortest path on the GraphData provided to this function.  This
/// function will compute the shortest path distance from sourceVertices[n] ->
/// endVertices[n] and store the cost in outResultCosts[n].  The number of results
/// it will compute is given by numResults.
///
/// This function will run the algorithm on a single GPU.
///
/// \param gpuContext Current context, must be created by caller
/// \param deviceId The device ID on which to run the kernel.  This can
///                 be determined externally by the caller or the multi
///                 GPU version will automatically split the work across
///                 devices
/// \param graph Structure containing the vertex, edge, and weight arra
///              for the input graph
/// \param startVertices Indices into the vertex array from which to
///                      start the search
/// \param outResultsCosts A pre-allocated array where the results for
///                        each shortest path search will be written.
///                        This must be sized numResults * graph->numVertices.
/// \param numResults Should be the size of all three passed inarrays
///
void runDijkstra( cl_context context, cl_device_id deviceId, GraphData* graph,
                  int *sourceVertices, int *results, int numResults,
                  const std::vector<std::pair<int, int> >& query_to_node,
                  boost::unordered_map<int, std::vector<int> >& node_to_query);


///
/// Run Dijkstra's shortest path on the GraphData provided to this function.  This
/// function will compute the shortest path distance from sourceVertices[n] ->
/// endVertices[n] and store the cost in outResultCosts[n].  The number of results
/// it will compute is given by numResults.
///
/// This function will run the algorithm on as many GPUs as is available.  It will
/// create N threads, one for each GPU, and chunk the workload up to perform
/// (numResults / N) searches per GPU.
///
/// \param gpuContext Current GPU context, must be created by caller
/// \param graph Structure containing the vertex, edge, and weight arra
///              for the input graph
/// \param startVertices Indices into the vertex array from which to
///                      start the search
/// \param outResultsCosts A pre-allocated array where the results for
///                        each shortest path search will be written.
///                        This must be sized numResults * graph->numVertices.
/// \param numResults Should be the size of all three passed inarrays
///
///
void runDijkstraMultiGPU( const char* dir, cl_context gpuContext,
                          GraphData* graph, int *sourceVertices,
                          int *outResultCosts, int numResults,
                const std::vector<std::pair<int, int> >& query_to_node,
                boost::unordered_map<int, std::vector<int> >& node_to_query);


///////////////////
/// The following code is a implementation of Dijkstra using minheap and
/// AdjacentList structure to get o(nlog(n)) time complexity
///
///
// A structure to represent a node in adjacency list
struct AdjListNode
{
    int dest;
    int weight;
    struct AdjListNode* next;
};

// A structure to represent an adjacency liat
struct AdjList
{
    struct AdjListNode *head;  // pointer to head node of list
};

// A structure to represent a graph. A graph is an array of adjacency lists.
// Size of array will be V (number of vertices in graph)
struct CPUGraph
{
    int V;
    struct AdjList* array;
};

// Structure to represent a min heap node
struct MinHeapNode
{
    int  v;
    int dist;
};

// Structure to represent a min heap
struct MinHeap
{
    int size;      // Number of heap nodes present currently
    int capacity;  // Capacity of min heap
    int *pos;     // This is needed for decreaseKey()
    struct MinHeapNode **array;
};

struct CPUGraph* createGraph(int V);

void addEdgeToGraph(struct CPUGraph* graph, int src, int dest, int weight);

void dijkstra(struct CPUGraph* graph, int src, int* results,
              const std::vector<std::pair<int, int> >& query_to_node,
              boost::unordered_map<int, std::vector<int> >& node_to_query,
              int* path = 0);

void freeGraph(CPUGraph* graph);
#endif // DIJKSTRA_KERNEL_H
