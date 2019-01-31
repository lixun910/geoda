//
// Created by Xun Li on 12/22/18.
//
#include<stdio.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include "TravelTool.h"


#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

#ifndef PI_OVER_180
#define PI_OVER_180 0.017453292519943295
#endif

#ifndef EARTH_RADIUS
#define EARTH_RADIUS 6372795  // meters
#endif

#ifndef KPH_TO_MPS
#define KPM_TO_MPS  0.2778 // KM per hour to Meters per second: 1000/60/60
#endif

using namespace OSMTools;
namespace pt = boost::posix_time;

GradientColor::GradientColor(const wxString& color_file_path)
{
    wxInitAllImageHandlers();
    image = new wxImage();
    image->LoadFile(color_file_path, wxBITMAP_TYPE_PNG);
}

wxColour GradientColor::GetColor(double val)
{
    if (image == NULL) return *wxWHITE;

    if (image->IsOk()) {
        int y = val * 255;
        if (y >= 254)  y = 254;
        unsigned char r = image->GetRed(1, y);
        unsigned char g = image->GetGreen(1, y);
        unsigned char b = image->GetBlue(1, y);

        return wxColour(r, g, b);
    }
    return *wxWHITE;
}

////////////////////////////////////////////////////////////////////////////////
//
// Class TravelBass
//
////////////////////////////////////////////////////////////////////////////////
TravelBass::TravelBass()
{

}

TravelBass::TravelBass(std::vector<OGRFeature*> in_roads,
                       double _default_speed,
                       double _penalty,
                       std::map<wxString, double> _speed_limit_dict,
                       const wxString& highway_type_field,
                       const wxString& max_speed_field,
                       const wxString& one_way_field,
                       const wxString& _one_way_flag)
: default_speed(_default_speed),
speed_penalty(_penalty), speed_limit_dict(_speed_limit_dict),
one_way_flag(_one_way_flag)
{
    roads = in_roads;
    
    PreprocessRoads();

    // test if osm roads
    is_osm = true;
    OGRFeature* feature = roads[0];
    way_type_idx = feature->GetFieldIndex(highway_type_field.mb_str(wxConvUTF8));
    one_way_idx = feature->GetFieldIndex(one_way_field.mb_str(wxConvUTF8));
    max_speed_idx = feature->GetFieldIndex(max_speed_field.mb_str(wxConvUTF8));

    if (way_type_idx < 0 || one_way_idx < 0 || max_speed_idx < 0) {
        is_osm = false;
    }
}

TravelBass::~TravelBass()
{
    for (int i=0; i<kd_tree->nPoints(); ++i) delete[] xy[i];
    delete[] xy;
    delete kd_tree;
}

wxString TravelBass::GetExeDir()
{
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName exeFile(exePath);
    wxString exeDir = exeFile.GetPathWithSep();
    return exeDir;

}
void TravelBass::PreprocessRoads()
{
    if (roads.empty() == true) return;

    size_t n_roads = roads.size();
    OGRFeature* feature;
    OGRGeometry* geom;
    OGRLineString* line;
    OGRMultiLineString* mline;
    OGRGeometryCollection *poCol;
    int i, j, k, idx, n_pts, node_count=0;

    // read ways
    for (i=0; i<n_roads; ++i) {
        feature = roads[i];
        geom = feature->GetGeometryRef();

        if (geom && geom->IsEmpty() == false) {
            mline = dynamic_cast<OGRMultiLineString*>(geom);
            if (mline != NULL) {
                // multi-linestring
                poCol = (OGRGeometryCollection*) geom;
                for (k=0; k< poCol->getNumGeometries(); ++k) {
                    std::vector<OGRPoint> e;
                    line = (OGRLineString*)(poCol->getGeometryRef(k));
                    n_pts = line->getNumPoints();
                    for (j=0; j<n_pts; ++j) {
                        OGRPoint pt;
                        line->getPoint(j, &pt);
                        RD_POINT rd_pt = std::make_pair(pt.getX(), pt.getY());

                        if (nodes_dict.find(rd_pt) == nodes_dict.end()) {
                            nodes_dict[rd_pt] = node_count;
                            node_appearance[node_count] = 1;
                            nodes.push_back(pt);
                            node_count ++;
                        } else {
                            idx = nodes_dict[rd_pt];
                            node_appearance[idx] += 1;
                        }
                        // end points: [wayid, wayid..]
                        if (j==0 || j == n_pts-1) {
                            idx = nodes_dict[rd_pt];
                            endpoint_dict[idx].push_back(i);
                        }
                        e.push_back(pt); // todo: possible memory issue
                    }
                    edge_to_feture.push_back(i);
                    edges.push_back(e);
                }
                mline = NULL;

            } else {
                // linestring
                std::vector<OGRPoint> e;
                line = (OGRLineString*) geom;
                n_pts = line->getNumPoints();
                for (j=0; j<n_pts; ++j) {
                    OGRPoint pt;
                    line->getPoint(j, &pt);
                    RD_POINT rd_pt = std::make_pair(pt.getX(), pt.getY());

                    if (nodes_dict.find(rd_pt) == nodes_dict.end()) {
                        nodes_dict[rd_pt] = node_count;
                        node_appearance[node_count] = 1;
                        nodes.push_back(pt);
                        node_count ++;
                    } else {
                        idx = nodes_dict[rd_pt];
                        node_appearance[idx] += 1;
                    }
                    // end points: [wayid, wayid..]
                    if (j==0 || j == n_pts-1) {
                        idx = nodes_dict[rd_pt];
                        endpoint_dict[idx].push_back(i);
                    }
                    e.push_back(pt); // todo: possible memory issue
                }
                edge_to_feture.push_back(i);
                edges.push_back(e);
            }
        }
    }
    oneway_dict.resize(edges.size(), false);

    node_count = node_appearance.size();

    // create a kdtree using nodes in ways
    xy = new double*[node_count];
    for (i=0; i<node_count; ++i) {
        OGRPoint pt = nodes[i];
        xy[i] = new double[2];
        xy[i][0] = pt.getX();
        xy[i][1] = pt.getY();
    }
    kd_tree = new ANNkd_tree(xy, node_count, 2);
}

void TravelBass::CreateWeightsFile(wxString w_file_path)
{

}

int TravelBass::DetectGPU()
{
    cl_int errNum;
    cl_platform_id platform;
    cl_context gpuContext;
    cl_uint numPlatforms;

    // First, select an OpenCL platform to run on.  For this example, we
    // simply choose the first available platform.  Normally, you would
    // query for all available platforms and select the most appropriate one.

    errNum = clGetPlatformIDs(1, &platform, &numPlatforms);
    printf("Number of OpenCL Platforms: %d\n", numPlatforms);
    if (errNum != CL_SUCCESS || numPlatforms <= 0) {
        printf("Failed to find any OpenCL platforms.\n");
        return 0;
    }

    // create the OpenCL context on available GPU devices
    gpuContext = clCreateContextFromType(0, CL_DEVICE_TYPE_GPU, NULL, NULL, &errNum);
    if (errNum != CL_SUCCESS) {
        printf("No GPU devices found.\n");
        return 0;
    }

    size_t deviceBytes;
    cl_uint deviceCount;

    errNum = clGetContextInfo(gpuContext, CL_CONTEXT_DEVICES, 0, NULL, &deviceBytes);
    if (errNum != CL_SUCCESS) {
        printf("No GPU devices found.\n");
        return 0;
    }
    deviceCount = (cl_uint)deviceBytes/sizeof(cl_device_id);

    return deviceCount;
}


double TravelBass::EarthMeterToDegree(double d)
{
    return 360 * d / (2 * EARTH_RADIUS * PI);
}

double TravelBass::ComputeArcDist(OGRPoint& from, OGRPoint& to)
{
    double lon1 = from.getX();
    double lat1 = from.getY();

    double lon2 = to.getX();
    double lat2 = to.getY();

    // degree to rad
    lat1 = lat1 * PI / 180.0;
    lon1 = lon1 * PI / 180.0;

    lat2 = lat2 * PI / 180.0;
    lon2 = lon2 * PI / 180.0;

    // this is the haversine formula which is particularly well-conditioned
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = pow(sin(dlat/2.0), 2);
    double b = cos(lat1) * cos(lat2) * pow(sin(dlon/2.0), 2);
    double d = 2 * EARTH_RADIUS * asin(sqrt(a+b));

    return d; // meters
}

////////////////////////////////////////////////////////////////////////////////
//
// Class TravelDistanceMatrix
//
////////////////////////////////////////////////////////////////////////////////
TravelDistanceMatrix::TravelDistanceMatrix(std::vector<OGRFeature*> roads,
                                    double default_speed, double penalty,
                                    std::map<wxString, double> speed_limit_dict,
                                    const wxString& _highway_type_field,
                                    const wxString& _max_speed_field,
                                    const wxString& _one_way_field,
                                    const wxString& _one_way_flag)
:  TravelBass(roads, default_speed, penalty, speed_limit_dict,
              _highway_type_field, _max_speed_field, _one_way_field,
              _one_way_flag)
{
    num_gpus = DetectGPU();
    num_cores = boost::thread::hardware_concurrency();

    int node_count = node_appearance.size();
    nodes_flag.resize(node_count, false);
}

TravelDistanceMatrix::~TravelDistanceMatrix()
{
}

void TravelDistanceMatrix::SetGPURatio(double val)
{
    ratio_cpu_to_gpu =  1 - val;
}

/**
 * This function is for creating distance matrix
 *
 */
void TravelDistanceMatrix::GetDistanceMatrix(std::vector<OGRFeature *> query_points,
                                             const wxString& out_file,
                                             bool save_intermediate_files)
{
    //query_points = _query_points;
    // anchor points on roads that are associated with query points
    boost::unordered_map<int, bool> anchor_points;

    // using query points to find anchor points from roads
    double eps = 0.00000001; // error bound
#ifdef __GEODA__
    ANN_DIST_TYPE = 2;
#endif
    int i, j, n_pts;
    OGRFeature* feature;
    OGRGeometry* geom;

    int anchor_cnt = 0, query_size = query_points.size();
    query_to_node.resize(query_size);
    for (i=0; i<query_size; ++i) {
        feature = query_points[i];
        geom = feature->GetGeometryRef();
        OGRPoint* m_pt = (OGRPoint*)geom;
        double* q_pt = new double[2];
        q_pt[0] = m_pt->getX();
        q_pt[1] = m_pt->getY();

        ANNidxArray nnIdx = new ANNidx[2];
        ANNdistArray dists = new ANNdist[2];
        kd_tree->annkSearch(q_pt, 2, nnIdx, dists, eps);

        int q_id = nnIdx[0];
        anchor_points[q_id] = true;
        if (node_to_query.find(q_id) == node_to_query.end()) {
            query_nodes.push_back(q_id);
            anchor_cnt++;
        }
        node_to_query[q_id].push_back(i);
        double c = ComputeArcDist(*m_pt, nodes[q_id]) / (default_speed * 1000);
        c = c * 60 * 60; // to seconds
        query_to_node[i] = std::make_pair(q_id, (int)c);

        delete[] q_pt;
        delete[] nnIdx;
        delete[] dists;
    }


    // build graph
    // remove circle way
    int from, to, cost;
    std::string highway, oneway, maxspeed;
    double speed_limit = default_speed, length;
    for (i=0; i<edges.size(); ++i) {
        n_pts = edges[i].size();
        OGRPoint start = edges[i][0];
        OGRPoint end = edges[i][n_pts-1];
        if (start.Equals(&end)) {
            removed_edges[i] = -1; // means circle
        }
        // compute pair_cost
        int feature_idx = edge_to_feture[i];
        feature = roads[feature_idx];
        highway = feature->GetFieldAsString(way_type_idx);
        oneway = feature->GetFieldAsString(one_way_idx);
        maxspeed = feature->GetFieldAsString(max_speed_idx);
        if (maxspeed.empty()) {
            if (speed_limit_dict.find(highway) != speed_limit_dict.end()) {
                speed_limit = speed_limit_dict[highway];
            }
        } else {
            std::string sp_str = maxspeed.substr(0,2);
            int val = atoi(sp_str.c_str()); // km
            if (val > 0 && val < 100) speed_limit = val * 1.6;
        }
        speed_limit = speed_limit / speed_penalty;

        if (oneway.find(one_way_flag.mb_str(wxConvUTF8)) == std::string::npos) {
            oneway_dict[i] = false;
        } else {
            oneway_dict[i] = true;
        }

        for (j=0; j<n_pts-1; ++j) {
            start = edges[i][j];
            end = edges[i][j+1];
            RD_POINT rd_from = std::make_pair(start.getX(), start.getY());
            RD_POINT rd_to = std::make_pair(end.getX(), end.getY());
            from = nodes_dict[rd_from];
            to = nodes_dict[rd_to];
            length = ComputeArcDist(start, end); // meter
            cost = length / (speed_limit * KPM_TO_MPS);
            //if (cost <= 0) cost = 1; // make sure
            pair_cost[std::make_pair(from, to)] = cost;
            if (oneway_dict[i]==false) {
                pair_cost[std::make_pair(to, from)] = cost;
            }
        }
    }
    if (save_intermediate_files) {
        //SaveMergedRoads("/Users/xun/Desktop/no_circle.shp");
    }

    // concat ways
    int node_idx, w1, w2;
    bool is_merged;
    std::vector<int> conn_ways;
    for (i=0; i<edges.size(); ++i) {
        if (removed_edges.find(i) != removed_edges.end()) {
            // this edge has been merged with other edge already
            // so skip
            continue;
        }
        n_pts = edges[i].size();
        OGRPoint start = edges[i][0];
        RD_POINT rd_start = std::make_pair(start.getX(), start.getY());
        node_idx = nodes_dict[rd_start];
        conn_ways = endpoint_dict[node_idx];
        is_merged = false;
        if (conn_ways.size() == 2) { // two ways share node_idx (start)
            w1 = i;
            bool flag = true;
            if (conn_ways[0] == w1) {
                w2 = conn_ways[1];
                w2 = GetValidEdgeId(w2);
            } else if (conn_ways[1] == w1) {
                w2 = conn_ways[0];
                w2 = GetValidEdgeId(w2);
            } else {
                int c1 = GetValidEdgeId(conn_ways[0]);
                int c2 = GetValidEdgeId(conn_ways[1]);
                if (c1 == w1) {
                    w2 = c2;
                } else if (c2 == w1) {
                    w2 = c1;
                } else {
                    flag = false;
                }
            }

            if (flag && w1 != w2 && removed_edges.find(w2) == removed_edges.end()) {
                if (MergeTwoWaysByStart(w1, w2)) {
                    removed_edges[w2] = w1;
                    is_merged = true;
                }
            }
        }

        OGRPoint end = edges[i][n_pts-1];
        if (start.Equals(&end)) continue; // cont if two ends are same

        RD_POINT rd_end = std::make_pair(end.getX(), end.getY());
        node_idx = nodes_dict[rd_end];
        conn_ways = endpoint_dict[node_idx];
        if (conn_ways.size() == 2) { // tow ways share node_idx (end)
            w1 = i;
            bool flag = true;
            if (conn_ways[0] == w1) {
                w2 = conn_ways[1];
                w2 = GetValidEdgeId(w2);
            } else if (conn_ways[1] == w1) {
                w2 = conn_ways[0];
                w2 = GetValidEdgeId(w2);
            } else {
                int c1 = GetValidEdgeId(conn_ways[0]);
                int c2 = GetValidEdgeId(conn_ways[1]);
                if (c1 == w1) {
                    w2 = c2;
                } else if (c2 == w1) {
                    w2 = c1;
                } else {
                    flag = false;
                }
            }

            if (flag && w1 != w2 && removed_edges.find(w2) == removed_edges.end()) {
                if (MergeTwoWaysByEnd(w1, w2)) {
                    removed_edges[w2] = w1;
                    is_merged = true;
                }
            }
        }
        if (is_merged) {
            i--;
            // reprocessing current way since it's merged w/ other ways
            // and therefore its content has been updated
        }
    }
    if (save_intermediate_files) {
        //SaveMergedRoads("/Users/xun/Desktop/merge.shp");
    }

    // simplify ways
    for (i=0; i<edges.size(); ++i) {
        if (removed_edges.find(i) != removed_edges.end()) {
            continue;
        }
        n_pts = edges[i].size();
        if (n_pts < 3) continue; // if way has 2 or 1 nodes, just skip

        int total_cost = 0;
        OGRPoint pt, prev_pt = edges[i][n_pts-1];
        std::pair<int, int> from_to;

        for (j=n_pts-2; j>=0; --j) {
            pt = edges[i][j];
            RD_POINT rd_pt = std::make_pair(pt.getX(), pt.getY());
            node_idx = nodes_dict[rd_pt];
            from_to.first = node_idx;
            from_to.second = nodes_dict[std::make_pair(prev_pt.getX(), prev_pt.getY())];

            if (j> 0 && node_appearance[node_idx] == 1 &&
                pair_cost.find(from_to) != pair_cost.end() &&
                anchor_points.find(node_idx) == anchor_points.end())
                {
                total_cost += pair_cost[from_to];
                edges[i].erase(edges[i].begin() + j);
                } else {
                    total_cost += pair_cost[from_to];
                    // add {from, to, dist} to edge_dict
                    AddEdge(i, edges[i][j], edges[i][j+1], total_cost);
                    total_cost = 0; // reset total_cost to 0
                }
            prev_pt = pt;
        }
    }
    if (save_intermediate_files) {
        //SaveMergedRoads("/Users/xun/Desktop/simplified.shp");
        //SaveGraphToShapefile("/Users/xun/Desktop/graph.shp");
        //SaveQueryNodes("/Users/xun/Desktop/query.shp");
    }

    // re-index nodes and edges
    int valid_index = 0;
    int node_count = kd_tree->nPoints();
    boost::unordered_map<int, int> index_map; // new idx : original idx
    boost::unordered_map<int, int> rev_index_map; // orig idx : new idx
    for (i=0; i<node_count; ++i) {
        if (nodes_flag[i] == true) {
            index_map[valid_index] = i;
            rev_index_map[i] = valid_index;
            valid_index ++;
        }
    }
    num_nodes = valid_index;
    num_edges = 0;
    boost::unordered_map<int, std::vector<std::pair<int, double> > >::iterator it;
    for (it = edges_dict.begin(); it!= edges_dict.end(); ++it) {
        num_edges += it->second.size();
    }
    boost::unordered_map<int, std::vector<int> > new_source_dict;
    for (i=0; i<query_nodes.size(); ++i) {
        int new_idx = rev_index_map[ query_nodes[i] ];
        new_source_dict[new_idx] = node_to_query[ query_nodes[i] ];
        query_nodes[i] = new_idx;
    }
    node_to_query.clear();
    node_to_query = new_source_dict;
    new_source_dict.clear();
    for (i=0; i<query_to_node.size(); ++i) {
        query_to_node[i].first = rev_index_map[ query_to_node[i].first ];
    }
    boost::unordered_map<int,
    std::vector<std::pair<int, double> > > new_edges_dict;
    int nbr_idx;
    for(i = 0; i < num_nodes; i++) {
        // for each valid node
        node_idx = index_map[i];
        std::vector<std::pair<int, double> >& nbrs = edges_dict[node_idx];
        for (j=0; j<nbrs.size(); ++j) {
            nbr_idx = rev_index_map[ nbrs[j].first ];
            cost = nbrs[j].second;
            new_edges_dict[i].push_back(std::make_pair(nbr_idx, cost));
        }
    }
    edges_dict.clear();
    edges_dict = new_edges_dict;
    new_edges_dict.clear();

    // compute distance

    // query_size: the size of input query points
    // full matrix is needes since we have one-way only roads
    size_t results_mem = sizeof(int) * (size_t)query_size * (size_t)query_size;
    int *results = (int*) malloc(results_mem);

    ComputeDistMatrix(results);

    std::vector<wxString> query_ids;
    SaveQueryResults(out_file.mb_str(wxConvUTF8), query_size, results,
                     query_ids);

    free(results);
}

bool TravelDistanceMatrix::MergeTwoWaysByStart(int w1, int w2)
{
    std::vector<OGRPoint>& e1 = edges[w1];
    std::vector<OGRPoint>& e2 = edges[w2];
    OGRPoint start1 = e1[0];
    OGRPoint start2 = e2[0];
    OGRPoint end2 = e2[e2.size() -1];

    if (start1.Equals(&start2)) {
        // reverse w2, concat w1
        if (oneway_dict[w2]) return false;
        for (int i=1; i < e2.size(); ++i) {
            e1.insert(e1.begin(), e2[i]);
        }
    } else if (start1.Equals(&end2)) {
        // w2 concat w1
        for (int i=e2.size()-2; i >= 0; --i) {
            e1.insert(e1.begin(), e2[i]);
        }
    } else {
        return false;
    }
    RD_POINT rd_pt = std::make_pair(start1.getX(), start1.getY());
    int node_idx = nodes_dict[rd_pt];
    node_appearance[node_idx] -= 1;

    //OGRPoint left = e1[0];
    //OGRPoint right = e1[e1.size() -1];
    //int left_idx = nodes_dict[std::make_pair(left.getX(), left.getY())];
    //int right_idx = nodes_dict[std::make_pair(right.getX(), right.getY())];
    return true;
}

bool TravelDistanceMatrix::MergeTwoWaysByEnd(int w1, int w2)
{
    std::vector<OGRPoint>& e1 = edges[w1];
    std::vector<OGRPoint>& e2 = edges[w2];
    OGRPoint end1 = e1[e1.size()-1];
    OGRPoint start2 = e2[0];
    OGRPoint end2 = e2[e2.size() -1];

    if (end1.Equals(&start2)) {
        // w1, concat w2
        for (int i=1; i < e2.size(); ++i) {
            e1.push_back(e2[i]);
        }
    } else if (end1.Equals(&end2)) {
        // w1 concat reversed w2
        if (oneway_dict[w2]) return false;
        for (int i=e2.size()-2; i >= 0; --i) {
            e1.push_back(e2[i]);
        }
    } else {
        return false;
    }
    RD_POINT rd_pt = std::make_pair(end1.getX(), end1.getY());
    int node_idx = nodes_dict[rd_pt];
    node_appearance[node_idx] -= 1;
    return true;
}

void TravelDistanceMatrix::AddEdge(int way_idx, OGRPoint& from, OGRPoint& to,
                                   int cost)
{
    RD_POINT rd_from = std::make_pair(from.getX(), from.getY());
    RD_POINT rd_to = std::make_pair(to.getX(), to.getY());
    int from_idx = nodes_dict[rd_from];
    int to_idx = nodes_dict[rd_to];

    nodes_flag[from_idx] = true;
    nodes_flag[to_idx] = true;

    edges_dict[from_idx].push_back(std::make_pair(to_idx, cost));
    if (oneway_dict[way_idx] == false) {
        edges_dict[to_idx].push_back(std::make_pair(from_idx, cost));
    }
}

int TravelDistanceMatrix::GetValidEdgeId(int idx) {
    while (removed_edges.find(idx) != removed_edges.end()) {
        if (idx != removed_edges[idx]) {
            idx = removed_edges[idx];
        } else {
            break;
        }
    }
    return idx;
}

void TravelDistanceMatrix::ComputeDistMatrix(int* results)
{
    // actually query_size using anchor nodes
    int query_size = query_nodes.size();

    if (num_gpus <= 1) {
        ComputeDistMatrixCPU(results, query_size, num_cores);

    } else {
        if (ratio_cpu_to_gpu < 0 || ratio_cpu_to_gpu > 1) {
            ratio_cpu_to_gpu = 0.8;
        }
        int cpu_results = query_size * (ratio_cpu_to_gpu);
        int gpu_results = query_size - cpu_results;

        boost::thread* bthread[2];

        bthread[0] = new boost::thread(
                        boost::bind(&TravelDistanceMatrix::ComputeDistMatrixCPU,
                                                   this,
                                                   results,
                                                   cpu_results,
                                                   num_cores));
        bthread[1] = new boost::thread(
                        boost::bind(&TravelDistanceMatrix::ComputeDistMatrixGPU,
                                                   this,
                                                   results,
                                                   gpu_results,
                                                   cpu_results));

        for (int i=0; i<2; ++i) {
            bthread[i]->join();
        }
        for (int i=0; i<2; ++i) {
            delete bthread[i];
        }
    }
}

void TravelDistanceMatrix::ComputeDistMatrixGPU(int* results, int query_size,
                                                int result_start)
{
    pt::ptime startTimeGPUCPU = pt::microsec_clock::local_time();

    int* vertex_array = (int*) malloc(num_nodes * sizeof(int));
    int* edge_array = (int*)malloc(num_edges * sizeof(int));
    int* weight_array = (int*)malloc(num_edges * sizeof(int));

    GraphData graph;
    graph.vertexCount = num_nodes;
    graph.vertexArray = vertex_array;
    graph.edgeCount = num_edges;
    graph.edgeArray = edge_array;
    graph.weightArray = weight_array;

    size_t i, j, cost, nbr_idx, e_idx = 0, offset = 0;
    for(i = 0; i < graph.vertexCount; i++) {
        // for each valid node
        graph.vertexArray[i] = offset;
        std::vector<std::pair<int, double> >& nbrs = edges_dict[i];
        for (j=0; j<nbrs.size(); ++j) {
            nbr_idx = nbrs[j].first;
            cost = nbrs[j].second;
            graph.edgeArray[e_idx] = nbr_idx;
            graph.weightArray[e_idx++] = cost;
        }
        offset += nbrs.size();
    }

    // get directory of kernel *.cl file
    char cl_dir[1024];
    wxString current_dir = GetExeDir();
    strncpy(cl_dir, (const char*)current_dir.mb_str(wxConvUTF8), 1023);

    int *query_indices = new int[query_size];
    for (i=result_start, j=0; i<query_nodes.size(); ++i) {
        query_indices[j++] = query_nodes[i];
    }
    cl_int errNum;
    // create the OpenCL context on available GPU devices
    cl_context gpuContext = clCreateContextFromType(0, CL_DEVICE_TYPE_GPU,
                                                    NULL, NULL, &errNum);
    if (errNum != CL_SUCCESS) {
        printf("No GPU devices found.\n");
        return;
    }

    runDijkstraMultiGPU(cl_dir, gpuContext, &graph, query_indices,
                        results, query_size, query_to_node, node_to_query);

    delete[] query_indices;
    free(vertex_array);
    free(edge_array);
    free(weight_array);

    pt::time_duration timeGPUCPU = pt::microsec_clock::local_time() - startTimeGPUCPU;
    printf("\nrunDijkstra - GPU Time: %f s\n", (float)timeGPUCPU.total_milliseconds() / 1000.0f);
}

void TravelDistanceMatrix::ComputeDistMatrixCPU(int* results, int query_size,
                                                int nCPUs)
{
    pt::ptime startTimeGPUCPU = pt::microsec_clock::local_time();

    size_t i, j, cost, node_idx, nbr_idx;

    // adjacent dijkstra on CPU
    CPUGraph* graph = createGraph(num_nodes);
    for(i = 0; i < num_nodes; i++) {
        // for each valid node
        std::vector<std::pair<int, double> >& nbrs = edges_dict[i];
        for (j=0; j<nbrs.size(); ++j) {
            nbr_idx = nbrs[j].first;
            cost = nbrs[j].second;
            addEdgeToGraph(graph, i, nbr_idx, cost);
        }
    }

    int work_chunk = query_size / nCPUs;

    if (work_chunk == 0) work_chunk = 1;

    int obs_start = 0;
    int obs_end = obs_start + work_chunk;
    int quotient = query_size / nCPUs;
    int remainder = query_size % nCPUs;
    int tot_threads = (quotient > 0) ? nCPUs : remainder;

    boost::thread* bthread[tot_threads];

    for (unsigned int i=0; i<tot_threads; i++) {
        int a=0;
        int b=0;
        if (i < remainder) {
            a = i*(quotient+1);
            b = a+quotient;
        } else {
            a = remainder*(quotient+1) + (i-remainder)*quotient;
            b = a+quotient-1;
        }

        bthread[i] = new boost::thread(boost::bind(&TravelDistanceMatrix::dijkstra_thread,
                                                   this,
                                                   (CPUGraph*) graph,
                                                   boost::ref(query_nodes),
                                                   (int*) results,
                                                   boost::ref(query_to_node),
                                                   boost::ref(node_to_query),
                                                   a, b));
    }
    for (unsigned int i = 0; i < tot_threads; i++) {
        bthread[i]->join();
    }

    for (unsigned int i = 0; i < tot_threads; i++) {
        delete bthread[i];
    }

    freeGraph(graph);
    pt::time_duration timeGPUCPU = pt::microsec_clock::local_time() - startTimeGPUCPU;
    printf("\nrunDijkstra - CPU Time: %f s\n", (float)timeGPUCPU.total_milliseconds() / 1000.0f);
}


void TravelDistanceMatrix::dijkstra_thread(CPUGraph* graph,
                                 const std::vector<int>& query_indexes,
                                 int* results,
                                 const std::vector<std::pair<int, int> >& query_to_node,
                                 boost::unordered_map<int, std::vector<int> >& node_to_query,
                                 int a, int b)
{
    for (size_t i=a; i<=b; ++i) {
        dijkstra(graph, query_indexes[i], results, query_to_node,
                 node_to_query, 0);
    }
}

bool TravelDistanceMatrix::SaveQueryResults(const char* file_path,
                                  int n_query,
                                  int* results,
                                  const std::vector<wxString>& query_ids)
{
    if (file_path == 0) return false;

    FILE * fp;
    fp = fopen(file_path, "wb");
    if (fp == 0) return false;

    fwrite(&n_query,sizeof(int), 1, fp);

    fwrite(query_ids.empty() ? "0" : "1", sizeof(char), 1, fp);
    for (size_t i=0; i<query_ids.size(); ++i) {
        int str_len = query_ids[i].length();
        fwrite(&str_len, sizeof(int), 1, fp);
        const char* id = query_ids[i].mb_str(wxConvUTF8);
        fwrite(id, sizeof(char), str_len, fp);
    }

    size_t results_size = (size_t)n_query * (size_t)n_query;
    fwrite(results, sizeof(int), results_size, fp);

    fclose(fp);

    return true;
}

void TravelDistanceMatrix::SaveMergedRoads(const char* shp_file_name)
{
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL ) {
        printf( "%s driver not available.\n", pszDriverName );
        return;
    }
    GDALDataset *poDS;
    poDS = poDriver->Create(shp_file_name, 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL ) {
        printf( "Creation of output file failed.\n" );
        return;
    }
    OGRLayer *poLayer;
    poLayer = poDS->CreateLayer("roads", NULL, wkbLineString, NULL );
    if( poLayer == NULL ) {
        printf( "Layer creation failed.\n" );
        GDALClose( poDS );
        return;
    }
    // wayid, highway, name, county, oneway, maxspeed, length,
    const char* prop_names[255] = {"wayid", "highway", "name", "county", "oneway", "maxspeed"};
    for (size_t i=0; i<6; ++i) {
        OGRFieldDefn oField(prop_names[i], OFTString );
        oField.SetWidth(32);
        if( poLayer->CreateField( &oField ) != OGRERR_NONE ) {
            printf( "Creating Name field failed.\n" );
            GDALClose( poDS );
            return;
        }
    }
    for (size_t i=0; i<edges.size(); ++i) {
        if (removed_edges.find(i) != removed_edges.end()) {
            continue;
        }
        OGRFeature *poFeature;
        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
        for (size_t c=0; c<6; ++c) {
            poFeature->SetField(c, roads[i]->GetFieldAsString(c));
        }
        OGRLineString line;
        for (size_t j=0; j<edges[i].size(); ++j) {
            OGRPoint pt = edges[i][j];
            line.addPoint(&pt);
        }
        poFeature->SetGeometry(&line);
        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE ) {
            printf( "Failed to create feature in shapefile.\n" );
            GDALClose( poDS );
            return;
        }
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
}

void TravelDistanceMatrix::SaveQueryNodes(const char* shp_file_name)
{
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL ) {
        printf( "%s driver not available.\n", pszDriverName );
        return;
    }
    GDALDataset *poDS;
    poDS = poDriver->Create(shp_file_name, 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL ) {
        printf( "Creation of output file failed.\n" );
        return;
    }
    OGRLayer *poLayer;
    poLayer = poDS->CreateLayer("roads", NULL, wkbPoint, NULL );
    if( poLayer == NULL ) {
        printf( "Layer creation failed.\n" );
        GDALClose( poDS );
        return;
    }
    OGRFieldDefn oField("from", OFTInteger );
    oField.SetWidth(32);
    if( poLayer->CreateField( &oField ) != OGRERR_NONE ) {
        printf( "Creating Name field failed.\n" );
        GDALClose( poDS );
        return;
    }

    for (size_t i=0; i<query_nodes.size(); ++i) {
        int from_idx = query_nodes[i];
        OGRPoint from_pt = nodes[from_idx];

        OGRFeature *poFeature;
        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
        poFeature->SetField(0, from_idx);


        poFeature->SetGeometry(&from_pt);
        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE ) {
            printf( "Failed to create feature in shapefile.\n" );
            GDALClose( poDS );
            return;
        }
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
}

void TravelDistanceMatrix::SaveGraphToShapefile(const char* shp_file_name)
{
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL ) {
        printf( "%s driver not available.\n", pszDriverName );
        return;
    }
    GDALDataset *poDS;
    poDS = poDriver->Create(shp_file_name, 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL ) {
        printf( "Creation of output file failed.\n" );
        return;
    }
    OGRLayer *poLayer;
    poLayer = poDS->CreateLayer("roads", NULL, wkbLineString, NULL );
    if( poLayer == NULL ) {
        printf( "Layer creation failed.\n" );
        GDALClose( poDS );
        return;
    }
    OGRFieldDefn oField("from", OFTInteger );
    oField.SetWidth(32);
    if( poLayer->CreateField( &oField ) != OGRERR_NONE ) {
        printf( "Creating Name field failed.\n" );
        GDALClose( poDS );
        return;
    }
    OGRFieldDefn oField1("to", OFTInteger );
    if( poLayer->CreateField( &oField1 ) != OGRERR_NONE ) {
        printf( "Creating Name field failed.\n" );
        GDALClose( poDS );
        return;
    }
    OGRFieldDefn oField2("cost", OFTReal );
    if( poLayer->CreateField( &oField2 ) != OGRERR_NONE ) {
        printf( "Creating Name field failed.\n" );
        GDALClose( poDS );
        return;
    }

    boost::unordered_map<int,
    std::vector<std::pair<int, double> > >::iterator it;
    for (it=edges_dict.begin(); it!=edges_dict.end(); ++it) {
        int from_idx = it->first;
        OGRPoint from_pt = nodes[from_idx];
        std::vector<std::pair<int, double> > nbrs = it->second;
        for (size_t i=0; i<nbrs.size(); ++i) {
            int to_idx = nbrs[i].first;
            double cost = nbrs[i].second;
            OGRFeature *poFeature;
            poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
            poFeature->SetField(0, from_idx);
            poFeature->SetField(1, to_idx);
            poFeature->SetField(2, cost);
            OGRLineString line;
            OGRPoint to_pt = nodes[to_idx];
            line.addPoint(&from_pt);
            line.addPoint(&to_pt);
            poFeature->SetGeometry(&line);
            if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE ) {
                printf( "Failed to create feature in shapefile.\n" );
                GDALClose( poDS );
                return;
            }
            OGRFeature::DestroyFeature( poFeature );
        }
    }
    GDALClose( poDS );
}



TravelHeatMap::TravelHeatMap(std::vector<OGRFeature*> roads,
                             double default_speed,
                             double penalty,
                             std::map<wxString, double> speed_limit_dict,
                             const wxString& _highway_type_field,
                             const wxString& _max_speed_field,
                             const wxString& _one_way_field,
                             const wxString& _one_way_flag)
:  TravelBass(roads, default_speed, penalty, speed_limit_dict,
              _highway_type_field, _max_speed_field, _one_way_field,
              _one_way_flag),
   cpu_graph(0)
{
    BuildCPUGraph();
}

TravelHeatMap::~TravelHeatMap()
{
   if (cpu_graph) delete cpu_graph;
}

/**
 * This function is for query travel path only
 *
 */
void TravelHeatMap::BuildCPUGraph()
{
    num_nodes = nodes_dict.size();
    // adjacent dijkstra on CPU
    cpu_graph = createGraph(num_nodes);

    OGRFeature* feature;
    OGRGeometry* geom;
    OGRLineString* line;

    size_t i, j, cost, n_pts, node_idx, nbr_idx;
    int from, to;
    std::string highway, oneway, maxspeed;
    double speed_limit = default_speed;
    double length;

    for (i=0; i<edges.size(); ++i) {
        // compute pair_cost
        int feature_idx = edge_to_feture[i];
        feature = roads[feature_idx];
        if (is_osm) {
            highway = feature->GetFieldAsString(way_type_idx);
            oneway = feature->GetFieldAsString(one_way_idx);
            maxspeed = feature->GetFieldAsString(max_speed_idx);

            if (maxspeed.empty()) {
                if (speed_limit_dict.find(highway) != speed_limit_dict.end()) {
                    speed_limit = speed_limit_dict[highway];
                }
            } else {
                std::string sp_str = maxspeed.substr(0,2);
                int val = atoi(sp_str.c_str()); // km
                if (val > 0 && val < 100) speed_limit = val * 1.6;
            }
            speed_limit = speed_limit / speed_penalty;
            if (oneway.find(one_way_flag.mb_str(wxConvUTF8)) == std::string::npos) {
                oneway_dict[i] = false;
            } else {
                oneway_dict[i] = true;
            }
        } else {
            oneway_dict[i] = false;
        }

        n_pts = edges[i].size();
        for (j=0; j<n_pts-1; ++j) {
            OGRPoint start = edges[i][j];
            OGRPoint end = edges[i][j+1];
            RD_POINT rd_from = std::make_pair(start.getX(), start.getY());
            RD_POINT rd_to = std::make_pair(end.getX(), end.getY());
            from = nodes_dict[rd_from];
            to = nodes_dict[rd_to];
            length = ComputeArcDist(start, end);
            cost = length;
            if (is_osm) cost = cost / (speed_limit * KPM_TO_MPS);

            edge_to_way[std::make_pair(from, to)] = feature_idx;
            addEdgeToGraph(cpu_graph, from, to, cost);
            if (oneway_dict[i]==false) {
                addEdgeToGraph(cpu_graph, to, from, cost);
                edge_to_way[std::make_pair(to, from)] = feature_idx;
            }
        }
    }
}

int TravelHeatMap::Query(OGRPoint& from_pt, OGRPoint& to_pt,
                      std::vector<OGRLineString>& ogr_line,
                      std::vector<int>& way_ids)
{
    if (cpu_graph == 0) return -1;

    // get points on edges close to from_pt and to_pt
    double eps = 0.00000001; // error bound
#ifdef __GEODA__
    ANN_DIST_TYPE = 2;
#endif

    ANNidxArray nnIdx = new ANNidx[2];
    ANNdistArray dists = new ANNdist[2];
    double* q_pt = new double[2];

    q_pt[0] = from_pt.getX();
    q_pt[1] = from_pt.getY();
    kd_tree->annkSearch(q_pt, 2, nnIdx, dists, eps);
    int from_node_idx = nnIdx[0];
    double c1 = ComputeArcDist(from_pt, nodes[from_node_idx]);
    if (is_osm) {
        c1 = c1 / (default_speed * 1000);
        c1 = c1 * 60 * 60; // to seconds
    }
    q_pt[0] = to_pt.getX();
    q_pt[1] = to_pt.getY();
    kd_tree->annkSearch(q_pt, 2, nnIdx, dists, eps);
    int to_node_idx = nnIdx[0];
    double c2 = ComputeArcDist(to_pt, nodes[to_node_idx]);
    if (is_osm) {
        c2 = c2 / (default_speed * 1000);
        c2 = c2 * 60 * 60; // to seconds
    }
    delete[] q_pt;
    delete[] nnIdx;
    delete[] dists;

    // query path using dijkstra
    int* path = new int[cpu_graph->V];
    int* results = new int[cpu_graph->V];
    // node_to_query & query_to_node are not needed here
    boost::unordered_map<int, std::vector<int> > node_to_query;
    std::vector<std::pair<int, int> > query_to_node;

    dijkstra(cpu_graph, from_node_idx, results, query_to_node,
             node_to_query, path);

    std::vector<int> node_path;
    int i = to_node_idx;
    while (path[i] != -1) {
        node_path.push_back(i);
        i = path[i];
    }
    if (node_path.empty()) return -1;

    double check_length = 0;
    for (i=0; i<node_path.size()-1; ++i) {
        int from = node_path[i];
        int to = node_path[i+1];
        OGRLineString line;
        line.addPoint(&nodes[from]);
        line.addPoint(&nodes[to]);
        check_length += ComputeArcDist(nodes[from], nodes[to]);
        ogr_line.push_back(line);
        std::pair<int, int> e(from, to);
        if (edge_to_way.find(e) != edge_to_way.end()) {
            way_ids.push_back(edge_to_way[e]);
        }
        if (i==node_path.size()-2) {
            OGRLineString line;
            line.addPoint(&nodes[to]);
            line.addPoint(&nodes[from_node_idx]);
            check_length += ComputeArcDist(nodes[from_node_idx], nodes[to]);
            ogr_line.push_back(line);
        }
    }

    int c = (int)(results[to_node_idx]);
    delete[] path;
    delete[] results;
    return c;
}

void TravelHeatMap::QueryHexMap(OGRPoint& start_pt, OGREnvelope& extent,
                             double hexagon_radius,
                             std::vector<std::vector<OGRPolygon> >& hexagons,
                             std::vector<std::vector<int> >& costs,
                             bool create_hexagons)
{
#ifdef __GEODA__
    ANN_DIST_TYPE = 2;
#endif

    // query using dijkstra
    ANNidxArray nnIdx = new ANNidx[2];
    ANNdistArray dists = new ANNdist[2];
    double q_pt[2];
    q_pt[0] = start_pt.getX();
    q_pt[1] = start_pt.getY();
    kd_tree->annkSearch(q_pt, 2, nnIdx, dists);
    int from_node_idx = nnIdx[0];

    // node_to_query & query_to_node are not needed here
    boost::unordered_map<int, std::vector<int> > node_to_query;
    std::vector<std::pair<int, int> > query_to_node;

    int *results = new int[cpu_graph->V];
    dijkstra(cpu_graph, from_node_idx, results, query_to_node,
             node_to_query, 0);

    double eps = 0.00000001; // error bound

    if (hexagons.empty()) create_hexagons = true;
    if (create_hexagons) hexagons.clear();
    costs.clear();

    hexagon_radius = EarthMeterToDegree(hexagon_radius);

    double hex_angle = 0.523598776; // 30 degrees in radians
    double side_length = hexagon_radius / cos(hex_angle);
    double hex_height = sin(hex_angle) * side_length;
    double hex_rect_height = side_length + 2 * hex_height;
    double hex_rect_width = 2 * hexagon_radius;

    double map_width, map_height; // degree unit
    map_width = extent.MaxX - extent.MinX;
    map_height = extent.MaxY - extent.MinY;

    int cell_size_h, cell_size_v;
    cell_size_h = (int)((ceil)(map_width / hex_rect_width));
    cell_size_v = (int)((ceil)(map_height / (hex_height + side_length)));

    // in degree
    double offset_h = (map_width - cell_size_h * hex_rect_width) / 2.0;
    double offset_v = (map_height - cell_size_v * ((hex_height + side_length))) / 2.0;

    double start_x = extent.MinX + offset_h;
    double start_y = extent.MaxY - offset_v;

    double x, y, cx, cy;
    for (size_t i=0; i<cell_size_v; ++i) {
        y = start_y - i * (side_length + hex_height);
        std::vector<OGRPolygon> hex_row;
        std::vector<int> cost_row;
        for (size_t j=0; j<cell_size_h; ++j) {
            // (x,y) left-top corner of hex
            x = start_x + j * hex_rect_width + ((i%2)*hexagon_radius);
            if (create_hexagons) {
                OGRPolygon poly;
                OGRLinearRing ring;
                OGRPoint p1(x+hexagon_radius, y);
                OGRPoint p2(x+hex_rect_width, y - hex_height);
                OGRPoint p3(x+hex_rect_width, y - hex_height - side_length);
                OGRPoint p4(x + hexagon_radius, y - hex_rect_height);
                OGRPoint p5(x, y - side_length - hex_height);
                OGRPoint p6(x, y - hex_height);
                ring.addPoint(&p1);
                ring.addPoint(&p2);
                ring.addPoint(&p3);
                ring.addPoint(&p4);
                ring.addPoint(&p5);
                ring.addPoint(&p6);
                poly.addRing(&ring);

                hex_row.push_back(poly);
            }
            cx = x + hexagon_radius;
            cy = y;// + hex_height + (side_length / 2);

            double q_pt[2];
            q_pt[0] = cx;
            q_pt[1] = cy;

            kd_tree->annkSearch(q_pt, 2, nnIdx, dists, eps);

            int node_idx = nnIdx[0];
            if (sqrt(dists[0]) > hexagon_radius) {
                cost_row.push_back(INT_MAX); // not valid distance
            } else if (results[node_idx] == INT_MAX) {
                node_idx = nnIdx[1]; // try second anchor point
                if (results[node_idx] == INT_MAX) {
                    cost_row.push_back(INT_MAX); // not valid distance
                } else {
                    cost_row.push_back(results[node_idx]);
                }
            } else {
                cost_row.push_back(results[node_idx]);
            }

        }
        if (create_hexagons) hexagons.push_back(hex_row);
        costs.push_back(cost_row);
    }
    delete[] nnIdx;
    delete[] dists;
    delete[] results;
}
