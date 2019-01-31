//
//  RoadUtils.hpp
//  GeoDa
//
//  Created by Xun Li on 1/10/19.
//

#ifndef RoadUtils_hpp
#define RoadUtils_hpp

#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <ogrsf_frmts.h>
#include <stdio.h>

#ifdef __GEODA__
#include "../kNN/ANN/ANN.h"
#else
#include <ANN/ANN.h>
#endif

class RoadUtils
{
public:
    RoadUtils(std::vector<OGRFeature*> roads);

    virtual ~RoadUtils();

    std::vector<int> SnapPointsOnRoad(std::vector<OGRFeature*> points);

protected:
    void PreprocessRoads();

    void thread_snap_point(std::vector<OGRFeature*> points,
                           std::vector<int>& results,
                           int start, int end);

protected:
    std::vector<OGRFeature*> roads;

    std::vector<OGRPoint> nodes;

    boost::unordered_map<std::pair<double, double>, int> nodes_dict;

    std::vector<std::vector<int> > node_to_ways;

    double** xy;
    ANNkd_tree* kd_tree;
    boost::mutex mtx_;
};

#endif /* RoadUtils_hpp */
