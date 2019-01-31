//
//  RoadUtils.cpp
//  GeoDa
//
//  Created by Xun Li on 1/10/19.
//


#include "RoadUtils.h"


RoadUtils::RoadUtils(std::vector<OGRFeature*> _roads)
{
    roads = _roads;

    PreprocessRoads();
}

RoadUtils::~RoadUtils()
{
    for (int i=0; i<kd_tree->nPoints(); ++i) delete[] xy[i];
    delete[] xy;
    delete kd_tree;
}

std::vector<int> RoadUtils::SnapPointsOnRoad(std::vector<OGRFeature*> points)
{
    int n_ways = roads.size();
    std::vector<int> counts(n_ways, 0);

    int n_points = points.size();
    int nCPUs = boost::thread::hardware_concurrency();
    int work_chunk = n_points / nCPUs;
    if (work_chunk == 0) work_chunk = 1;
    int obs_start = 0;
    int obs_end = obs_start + work_chunk;
    int quotient = n_points / nCPUs;
    int remainder = n_points % nCPUs;
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

        bthread[i] = new boost::thread(boost::bind(&RoadUtils::thread_snap_point,
                                                   this,
                                                   boost::ref(points),
                                                   boost::ref(counts),
                                                   a, b));
    }
    for (unsigned int i = 0; i < tot_threads; i++) {
        bthread[i]->join();
    }

    for (unsigned int i = 0; i < tot_threads; i++) {
        delete bthread[i];
    }
    return counts;
}

void RoadUtils::thread_snap_point(std::vector<OGRFeature*> points,
                                  std::vector<int>& results,
                                  int start, int end)
{
    double eps = 0.00000001; // error bound
#ifdef __GEODA__
    ANN_DIST_TYPE = 2;
#endif

    OGRFeature* feature;
    OGRGeometry* geom;

    for (size_t i=start; i<=end; ++i) {
        feature = points[i];
        geom = feature->GetGeometryRef();
        OGRPoint* m_pt = (OGRPoint*)geom;

        double q_pt[2];// = new double[2];
        q_pt[0] = m_pt->getX();
        q_pt[1] = m_pt->getY();

        mtx_.lock();
        ANNidxArray nnIdx = new ANNidx[2];
        ANNdistArray dists = new ANNdist[2];
        kd_tree->annkSearch(q_pt, 2, nnIdx, dists, eps);

        int q_id = nnIdx[0];
        int w_id = node_to_ways[q_id][0]; // just pick first road

        results[w_id] += 1;

        delete[] nnIdx;
        delete[] dists;
        mtx_.unlock();
    }
}

void RoadUtils::PreprocessRoads()
{
    if (roads.empty() == true) return;

    size_t n_roads = roads.size();
    OGRFeature* feature;
    OGRGeometry* geom;
    OGRLineString* line;
    OGRMultiLineString* mline;
    OGRGeometryCollection *poCol;
    int i, j, k, idx, n_pts, node_count=0;
    std::pair<double, double> rd_pt;

    // read ways
    for (i=0; i<n_roads; ++i) {
        feature = roads[i];
        geom = feature->GetGeometryRef();
        if (geom && geom->IsEmpty() == false) {
            if (mline = dynamic_cast<OGRMultiLineString*>(geom)) {
                poCol = (OGRGeometryCollection*) geom;
                for(k=0; k< poCol->getNumGeometries(); ++k) {
                    line = (OGRLineString*)(poCol->getGeometryRef(k));
                    n_pts = line->getNumPoints();
                    for (j=0; j<n_pts; ++j) {
                        OGRPoint pt;
                        line->getPoint(j, &pt);
                        rd_pt = std::make_pair(pt.getX(), pt.getY());
                        if (nodes_dict.find(rd_pt) == nodes_dict.end()) {
                            nodes_dict[rd_pt] = node_count;
                            std::vector<int> edge_ids;
                            edge_ids.push_back(i);
                            node_to_ways.push_back(edge_ids);
                            nodes.push_back(pt);
                            node_count ++;
                        } else {
                            idx = nodes_dict[rd_pt];
                            node_to_ways[idx].push_back(i);
                        }
                    }
                }
                mline = NULL;
            } else {
                line = (OGRLineString*) geom;
                n_pts = line->getNumPoints();
                for (j=0; j<n_pts; ++j) {
                    OGRPoint pt;
                    line->getPoint(j, &pt);
                    std::pair<double, double> rd_pt = std::make_pair(pt.getX(), pt.getY());
                    if (nodes_dict.find(rd_pt) == nodes_dict.end()) {
                        nodes_dict[rd_pt] = node_count;
                        std::vector<int> edge_ids;
                        edge_ids.push_back(i);
                        node_to_ways.push_back(edge_ids);
                        nodes.push_back(pt);
                        node_count ++;
                    } else {
                        idx = nodes_dict[rd_pt];
                        node_to_ways[idx].push_back(i);
                    }
                }
            }
        }
    }

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
