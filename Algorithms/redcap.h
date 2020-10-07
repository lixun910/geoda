/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created: 5/30/2017 lixun910@gmail.com
 */

#ifndef __GEODA_CENTER_REDCAP_H__
#define __GEODA_CENTER_REDCAP_H__

#include <vector>
#include <set>
#include <float.h>

#include "azp.h"
#include "../ShapeOperations/GalWeight.h"

#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/heap/priority_queue.hpp>
#include <boost/graph/adjacency_list.hpp>




using namespace std;
using namespace boost;

namespace SpanningTreeClustering {
    
    class Node;
    class Edge;
    class Tree;
    class AbstractClusterFactory;
    
    /////////////////////////////////////////////////////////////////////////
    //
    // SSDUtils
    //
    /////////////////////////////////////////////////////////////////////////
    struct Measure
    {
        double ssd;
        double ssd_part1;
        double ssd_part2;
        double measure_reduction;
    };
    
    class SSDUtils
    {
        const double** raw_data;
        int row;
        int col;
        
        boost::unordered_map<vector<int>, double> cache;
        
    public:
        SSDUtils(const double** data, int _row, int _col) {
            raw_data = data;
            row = _row;
            col = _col;
        }
        ~SSDUtils() {}
        
        double ComputeSSD(vector<int>& visited_ids, int start, int end);
        void MeasureSplit(double ssd, vector<int>& visited_ids, int split_position, Measure& result);
        
        void MeasureSplit(double ssd, vector<int> &cand_ids, vector<int>& ids,  Measure& result);
    };
    

struct CandidateCut {
    int id1;
    int id2;
    
    double sqsum1;
    double sqsum2;
    double sum1;
    double sum2;
    double size1;
    double size2;
    
    const double** raw_data;
    int row;
    int col;
    
    void SetValues(int _id1, int _id2, double _sum1, double _sum2, double _sqsum1,
                 double _sqsum2, double _size1, double _size2,
                   const double** _raw_data, int _col)  {
        id1  = _id1;
        id2 = _id2;
        sum1 = _sum1;
        sum2 = _sum2;
        sqsum1 = _sqsum1;
        sqsum2 = _sqsum2;
        size1 = _size1;
        size2 =  _size2;
        raw_data = _raw_data;
        col = _col;
    }
    
    double GetSSD() {
        double sum_squared1 = sqsum1 - (sum1 * sum1 / size1);
        double sum_squared2 = sqsum2 - (sum2 * sum2 / size2);
        
        double ssd1 = sum_squared1 / col;
        double ssd2 = sum_squared2 / col;
        
        return ssd1 + ssd2;
    }
};

    /////////////////////////////////////////////////////////////////////////
    //
    // Node
    //
    /////////////////////////////////////////////////////////////////////////
    
    struct NeighborInfo
    {
        Node* p;
        Node* n1;
        Node* n2;
        Edge* e1;
        Edge* e2;
        
        void SetDefault(Node* parent) {
            p = parent;
            n1 = NULL;
            n2 = NULL;
            e1 = NULL;
            e2 = NULL;
        }
        
        void AddNeighbor(Node* nbr, Edge* e) {
            if (n1 == NULL) {
                n1 = nbr;
                e1 = e;
            } else if (n2 == NULL) {
                n2 = nbr;
                e2 = e;
            } else {
                //cout << "AddNeighbor() > 2" << endl;
            }
        }
    };
    
    class Node
    {
    public:
        Node(int id);
        virtual ~Node() {}
        
        
        int id; // mapping to record id
        Node* parent;
        int rank;
        
        //Cluster* container;
        //NeighborInfo nbr_info;
    };
    
    class DisjoinSet
    {
        boost::unordered_map<int, Node*> map;
    public:
        DisjoinSet();
        DisjoinSet(int id);
        ~DisjoinSet() {};
        
        Node* MakeSet(int id);
        void Union(Node* n1, Node* n2);
        Node* FindSet(Node* node);
    };
                   
    /////////////////////////////////////////////////////////////////////////
    //
    // Edge
    //
    /////////////////////////////////////////////////////////////////////////
    class Edge
    {
    public:
        Edge(Node* a, Node* b, double length);
        ~Edge() {}
        
        Node* orig;
        Node* dest;
        double length; // legnth of the edge |a.val - b.val|
        
        bool operator < (const Edge* e) const
        {
           if (length < e->length) {
                return true;
            } else if (length > e->length ) {
                return false;
            } else if (orig->id < e->orig->id) {
                return true;
            } else if (orig->id > e->orig->id) {
                return false;
            } else if (dest->id < e->dest->id) {
                return true;
            } else if (dest->id > e->dest->id) {
                return false;
            }
            return true;
        }
    };
    
    /////////////////////////////////////////////////////////////////////////
    //
    // Tree
    //
    /////////////////////////////////////////////////////////////////////////
    struct SplitSolution
    {
        int split_pos;
        vector<int> split_ids;
        double ssd;
        double ssd_reduce;
        boost::unordered_map<int, int> group;
    };
    
    class Tree
    {
    public:
        Tree(vector<int> ordered_ids, vector<Edge*> _edges, AbstractClusterFactory* cluster);
        ~Tree();
        
        void Partition(int start, int end, vector<int>& ids,
                       vector<pair<int, int> >& od_array,
                       boost::unordered_map<int, vector<int> >& nbr_dict);
        
        void Split(int orig, int dest,
                   boost::unordered_map<int, vector<int> >& nbr_dict,
                   vector<int>& cand_ids);

        bool checkControl(const vector<int>& cand_ids, vector<int>& ids, int flag);

        bool checkBounds();

        pair<Tree*, Tree*> GetSubTrees();
        
        double ssd_reduce;
        double ssd;
        
        vector<pair<int, int> > od_array;
        AbstractClusterFactory* cluster;
        pair<Tree*, Tree*> subtrees;
        
        int max_id;
        int split_pos;
        vector<int> split_ids;
        boost::unordered_map<int, int> group;
        
        vector<Edge*> edges;
        vector<int> ordered_ids;
        SSDUtils* ssd_utils;
        
        double* controls;
        double control_thres;
        
        // threads
        boost::mutex mutex;
        void run_threads(vector<int>& ids,
                       vector<pair<int, int> >& od_array,
                       boost::unordered_map<int, vector<int> >& nbr_dict);
        vector<SplitSolution> split_cands;
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // AbstractRedcap
    //
    ////////////////////////////////////////////////////////////////////////////////
    struct CompareTree
    {
    public:
        bool operator() (const Tree* lhs, const Tree* rhs) const
        {
            return lhs->ssd_reduce < rhs->ssd_reduce;
        }
    };
    
    typedef heap::priority_queue<Tree*, heap::compare<CompareTree> > PriorityQueue;
    
    class AbstractClusterFactory
    {
    public:
        int rows;
        int cols;
        GalElement* w;
        double** dist_matrix;
        const double** raw_data;
        const vector<bool>& undefs; // undef = any one item is undef in all variables
        double* controls;
        double control_thres;
        SSDUtils* ssd_utils;
        
        //Cluster* cluster;
        DisjoinSet djset;
        
        vector<Node*> nodes;
        vector<Edge*> edges;
        
        vector<int> ordered_ids;
        vector<Edge*> ordered_edges;
        
        vector<boost::unordered_map<int, double> > dist_dict;
        
        vector<vector<int> > cluster_ids;

        std::vector<ZoneControl> zone_controls;
        
        AbstractClusterFactory(int row, int col,
                               double** distances,
                               const double** data,
                               const vector<bool>& undefs,
                               GalElement * w,
                               const std::vector<ZoneControl>& c);
        virtual ~AbstractClusterFactory();
        
        virtual void Clustering()=0;
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum) { return 0;}
        
        Edge* GetShortestEdge(vector<Edge*>& edges, int start, int end){ return NULL;}
        
        void init();
        void Partitioning(int k);
        vector<vector<int> >& GetRegions();
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 1 Skater
    //
    ////////////////////////////////////////////////////////////////////////////////
    typedef adjacency_list <
    vecS,
    vecS,
    undirectedS,
    boost::no_property,         //VertexProperties
    property < edge_weight_t, double>   //EdgeProperties
    > Graph;
    
    class Skater : public AbstractClusterFactory
    {
    public:
        Skater(int rows, int cols,
               double** _distances,
               const double** data,
               const vector<bool>& undefs,
               GalElement * w,
               double* controls,
               double control_thres,
               const std::vector<ZoneControl>& c);
        virtual ~Skater();
        virtual void Clustering();
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 1 FirstOrderSLKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FirstOrderSLKRedCap : public AbstractClusterFactory
    {
    public:
        FirstOrderSLKRedCap(int rows, int cols,
                            double** _distances,
                            const double** data,
                            const vector<bool>& undefs,
                            GalElement * w,
                            double* controls,
                            double control_thres,
                            const std::vector<ZoneControl>& c);
        virtual ~FirstOrderSLKRedCap();
        
        virtual void Clustering();
    };
    
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 2 FirstOrderALKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FirstOrderALKRedCap : public AbstractClusterFactory
    {
    public:
        FirstOrderALKRedCap(int rows, int cols,
                            double** _distances,
                            const double** data,
                            const vector<bool>& undefs,
                            GalElement * w,
                            double* controls,
                            double control_thres,
                            const std::vector<ZoneControl>& c);
        
        virtual ~FirstOrderALKRedCap();
        
        virtual void Clustering();
        
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 3 FirstOrderCLKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FirstOrderCLKRedCap : public AbstractClusterFactory
    {
    public:
        FirstOrderCLKRedCap(int rows, int cols,
                            double** _distances,
                            const double** data,
                            const vector<bool>& undefs,
                            GalElement * w,
                            double* controls,
                            double control_thres,
                            const std::vector<ZoneControl>& c);
        
        virtual ~FirstOrderCLKRedCap();
        
        virtual void Clustering();
        
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 5 FullOrderALKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FullOrderALKRedCap : public AbstractClusterFactory
    {
    public:
        FullOrderALKRedCap(int rows, int cols,
                           double** _distances,
                           const double** data,
                           const vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres,
                           const std::vector<ZoneControl>& c,
                           bool init=true);
        
        virtual ~FullOrderALKRedCap();
        
        virtual void Clustering();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum);
        
        Edge* GetShortestEdge(vector<Edge*>& edges, int start, int end);
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 4 FullOrderSLKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FullOrderSLKRedCap : public FullOrderALKRedCap
    {
    public:
        FullOrderSLKRedCap(int rows, int cols,
                           double** _distances,
                           const double** data,
                           const vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres,
                           const std::vector<ZoneControl>& c);
        virtual ~FullOrderSLKRedCap();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum);
        
    };
    
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 6 FullOrderCLKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FullOrderCLKRedCap : public FullOrderALKRedCap
    {
    public:
        FullOrderCLKRedCap(int rows, int cols,
                           double** _distances,
                           const double** data,
                           const vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres,
                           const std::vector<ZoneControl>& c);
        
        virtual ~FullOrderCLKRedCap();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum);
        
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // 6 Ward
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FullOrderWardRedCap : public AbstractClusterFactory
    {
    public:
        FullOrderWardRedCap(int rows, int cols,
                           double** _distances,
                           const double** data,
                           const vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres,
                            const std::vector<ZoneControl>& c);
        
        virtual ~FullOrderWardRedCap();
        
        virtual void Clustering();
    };
}

#endif
