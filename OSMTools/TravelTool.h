//
// Created by Xun Li on 12/22/18.
//

#ifndef OSMTOOLSPROJECT_TRAVELTOOL_H
#define OSMTOOLSPROJECT_TRAVELTOOL_H

#include <vector>
#include <map>
#include <boost/unordered_map.hpp>
#include <wx/wx.h>

#ifdef __GEODA__
#include "../kNN/ANN/ANN.h"
#else
#include <ANN/ANN.h>
#endif
#include <ogrsf_frmts.h>

#include "oclDijkstraKernel.h"
#include "Roads.h"

namespace OSMTools {
    typedef std::pair<double, double> RD_POINT;

    class GradientColor {
    public:
        GradientColor(const wxString& color_file_path);

        wxColour GetColor(double val);

    protected:
        wxImage* image;
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Class TravelBass
    //
    ////////////////////////////////////////////////////////////////////////////////
    class TravelBass {
    public:
        TravelBass();

        TravelBass(std::vector<OGRFeature*> roads,
                   double default_speed,
                   double penalty,
                   std::map<wxString, double> speed_limit_dict,
                   const wxString& highway_type_field,
                   const wxString& max_speed_field,
                   const wxString& one_way_field,
                   const wxString& one_way_flag = "yes"
                   );

        virtual ~TravelBass();

        static int DetectGPU();

        void CreateWeightsFile(wxString w_file_path);
        
    protected:

        void PreprocessRoads();

        double EarthMeterToDegree(double d);
        
        double ComputeArcDist(OGRPoint& from, OGRPoint& to);

        wxString GetExeDir();
        
    protected:
        double** xy;
        ANNkd_tree* kd_tree;

        double default_speed;
        double speed_penalty;
        
        int  num_nodes;
        int  num_edges;

        bool is_osm;
        int  way_type_idx;
        int  one_way_idx;
        int  max_speed_idx;
        wxString one_way_flag;

        std::vector<OGRFeature*> roads;

        std::vector<OGRPoint> nodes;

        // index for finding idx of node in nodes[] by (lat, lon)
        boost::unordered_map<RD_POINT, int> nodes_dict;

        // node: count of its edge appearance
        boost::unordered_map<int, int> node_appearance;

        std::vector<std::vector<OGRPoint> > edges;

        std::vector<int> edge_to_feture;

        std::vector<bool> oneway_dict;

        // end node: [way index, way index...]
        boost::unordered_map<int, std::vector<int> > endpoint_dict;

        std::map<wxString, double> speed_limit_dict;

    };


    ////////////////////////////////////////////////////////////////////////////////
    //
    // Class TravelDistanceMatrix
    //
    ////////////////////////////////////////////////////////////////////////////////
    class TravelDistanceMatrix : public TravelBass
    {
    protected:

        //std::vector<OGRFeature*> query_points;
        float ratio_cpu_to_gpu;
        int   num_gpus;
        int   num_cores;

        // indicate if node was included in final graph
        std::vector<bool> nodes_flag;

        // final query nodes for dijkstra
        std::vector<int> query_nodes;

        // removed edge: concat to edge
        boost::unordered_map<int, int> removed_edges;

        // node : [nbr_node, cost], [nbr_node, cost],...
        boost::unordered_map<int, std::vector<std::pair<int, double> > > edges_dict;

        // [node idx, node idx] : cost (seconds)
        boost::unordered_map<std::pair<int, int>, int> pair_cost;

        // anchor point idx : [idx-in-query_nodes]
        boost::unordered_map<int, std::vector<int> > node_to_query;

        // idx-in-query : [anchor point idx, cost]
        std::vector<std::pair<int, int> > query_to_node;

    public:
        TravelDistanceMatrix(std::vector<OGRFeature*> roads,
                             double default_speed,
                             double penalty,
                             std::map<wxString, double> speed_limit_dict,
                             const wxString& highway_type_field,
                             const wxString& max_speed_field,
                             const wxString& one_way_field,
                             const wxString& _one_way_flag = "yes");

        virtual ~TravelDistanceMatrix();

        void GetDistanceMatrix(std::vector<OGRFeature*> query_points,
                               const wxString& out_file,
                               bool save_intermediate_files=false);

        void SetGPURatio(double val);

    protected:
        int GetValidEdgeId(int idx);

        void AddEdge(int way_idx, OGRPoint& from, OGRPoint& to, int cost);

        bool MergeTwoWaysByStart(int w1, int w2);

        bool MergeTwoWaysByEnd(int w1, int w2);

        bool SaveQueryResults(const char* file_path,
                          int n_query, int* results,
                          const std::vector<wxString>& query_ids);

        void ComputeDistMatrix(int* results);

        void ComputeDistMatrixCPU(int* results, int query_size, int nCPUs);

        void ComputeDistMatrixGPU(int* results, int query_size, int offset);

        void dijkstra_thread(CPUGraph* graph,
                         const std::vector<int>& query_indexes,
                         int* results,
                         const std::vector<std::pair<int, int> >& query_to_node,
                         boost::unordered_map<int, std::vector<int> >& node_to_query,
                         int a, int b);

        void SaveMergedRoads(const char* shp_file_name);
        void SaveQueryNodes(const char* shp_file_name);
        void SaveGraphToShapefile(const char* shp_file_name);
    };


    ////////////////////////////////////////////////////////////////////////////////
    //
    // Class TravelHeatMap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class TravelHeatMap : public TravelBass
    {
    protected:
        CPUGraph* cpu_graph;

        // [node idx, node idx] : way id
        boost::unordered_map<std::pair<int, int>, int> edge_to_way;


    public:
        TravelHeatMap(std::vector<OGRFeature*> roads,
                      double default_speed,
                      double penalty,
                      std::map<wxString, double> speed_limit_dict,
                      const wxString& highway_type_field,
                      const wxString& max_speed_field,
                      const wxString& one_way_field,
                      const wxString& _one_way_flag = "yes");

        virtual ~TravelHeatMap();

        int Query(OGRPoint& from_pt, OGRPoint& to_pt,
                  std::vector<OGRLineString>& ogr_line,
                  std::vector<int>& way_ids);

        void QueryHexMap(OGRPoint& start_pt, OGREnvelope& extent,
                         double hexagon_radius,
                         std::vector<std::vector<OGRPolygon> >& hexagons,
                         std::vector<std::vector<int> >& costs,
                         bool create_hexagons = true);
    protected:
        void BuildCPUGraph();
    };
}

#endif //OSMTOOLSPROJECT_TRAVELTOOL_H
