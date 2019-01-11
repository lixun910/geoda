//
//  RoadWeights.cpp
//  GeoDa
//
//  Created by Xun Li on 1/10/19.
//
#include <set>
#include <wx/textfile.h>

#include "RoadWeights.h"

using namespace GeoDa;

RoadWeights::RoadWeights(std::vector<OGRFeature*> _roads)
{
    roads = _roads;
    ProcessRoads();
}

RoadWeights::~RoadWeights()
{

}

void RoadWeights::CreateWeightsFile(const wxString &w_file_path)
{
    wxTextFile file(w_file_path);
    file.Create(w_file_path);
    file.Open(w_file_path);
    file.Clear();

    int node_idx, n_pts;
    std::pair<double, double> rd_pt;
    OGRFeature* feature;
    OGRGeometry* geom;
    OGRLineString* line;
    OGRMultiLineString* mline;
    OGRGeometryCollection *poCol;

    size_t n_roads = roads.size();

    wxString header;
    header << "0 " << n_roads << " Roads";
    file.AddLine(header);

    boost::unordered_map<int, int>::iterator it;

    for (size_t i=0; i<n_roads; ++i) {
        feature = roads[i];
        geom = feature->GetGeometryRef();
        boost::unordered_map<int, int> nbr_ways;
        
        if (mline = dynamic_cast<OGRMultiLineString*>(geom)) {
            poCol = (OGRGeometryCollection*) geom;
            for(size_t k=0; k< poCol->getNumGeometries(); ++k) {
                line = (OGRLineString*)(poCol->getGeometryRef(k));
                n_pts = line->getNumPoints();
                boost::unordered_map<int, int> nbr_ways;
                for (size_t j=0; j<n_pts; ++j) {
                    OGRPoint pt;
                    line->getPoint(j, &pt);
                    rd_pt = std::make_pair(pt.getX(), pt.getY());
                    if (nodes_dict.find(rd_pt) == nodes_dict.end()) {
                        // should never be here
                        continue;
                    }
                    node_idx = nodes_dict[rd_pt];
                    std::vector<int>& way_ids = node_to_ways[node_idx];
                    for (size_t k=0; k<way_ids.size(); ++k) {
                        if ( i != way_ids[k]) {
                            nbr_ways[way_ids[k]] = 1;
                        }
                    }
                }
            }
            mline = NULL;
        } else {
            line = (OGRLineString*) geom;
            n_pts = line->getNumPoints();

            for (size_t j=0; j<n_pts; ++j) {
                OGRPoint pt;
                line->getPoint(j, &pt);
                rd_pt = std::make_pair(pt.getX(), pt.getY());
                if (nodes_dict.find(rd_pt) == nodes_dict.end()) {
                    // should never be here
                    continue;
                }
                node_idx = nodes_dict[rd_pt];
                std::vector<int>& way_ids = node_to_ways[node_idx];
                for (size_t k=0; k<way_ids.size(); ++k) {
                    if ( i != way_ids[k]) {
                        nbr_ways[way_ids[k]] = 1;
                    }
                }
            }
        }

        wxString line1, line2;
        line1 << i << " " << nbr_ways.size();
        file.AddLine(line1);

        for (it = nbr_ways.begin(); it != nbr_ways.end(); ++it) {
            line2 << it->first << " ";
        }
        file.AddLine(line2);
    }
    file.Write();
    file.Close();
}

void RoadWeights::ProcessRoads()
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
        std::vector<OGRPoint> e;
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
                    e.push_back(pt); // todo: possible memory issue
                }
            }
        }
        edges.push_back(e);
    }
}
