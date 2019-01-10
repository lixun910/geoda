//
//  RoadWeights.hpp
//  GeoDa
//
//  Created by Xun Li on 1/10/19.
//

#ifndef RoadWeights_hpp
#define RoadWeights_hpp

#include <stdio.h>
#include <vector>
#include <ogrsf_frmts.h>
#include <wx/wx.h>
#include <boost/unordered_map.hpp>

namespace GeoDa {

class RoadWeights
{
    public:
    RoadWeights(std::vector<OGRFeature*> roads);
    virtual ~RoadWeights();

    void CreateWeightsFile(const wxString& w_file_path);

    protected:
    void ProcessRoads();

    protected:
    std::vector<OGRFeature*> roads;

    std::vector<std::vector<OGRPoint> > edges;

    std::vector<OGRPoint> nodes;

    boost::unordered_map<std::pair<double, double>, int> nodes_dict;

    std::vector<std::vector<int> > node_to_edges;
};

}

#endif /* RoadWeights_hpp */
