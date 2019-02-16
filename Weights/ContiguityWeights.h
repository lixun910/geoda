//
// Created by Xun Li on 2/14/19.
//

#ifndef SPATIALWEIGHTS_CONTIGUITYWEIGHTS_H
#define SPATIALWEIGHTS_CONTIGUITYWEIGHTS_H

#include <vector>
#include <map>
#include <ogrsf_frmts.h>
#include <boost/polygon/voronoi.hpp>

#include "AbstractWeights.h"

class ContiguityWeights : public AbstractWeights {
public:
    ContiguityWeights(OGRwkbGeometryType geom_type,
            const std::vector<OGRGeometry *> &geoms);

    ContiguityWeights();

    virtual ~ContiguityWeights();

protected:
    OGRwkbGeometryType geom_type;

    std::vector<OGRGeometry*> geoms;

    int n_geoms;

    virtual Gda::GalElement *CreateWeights() = 0;
};

class PointContiWeights : public ContiguityWeights {
    typedef boost::polygon::voronoi_diagram<double> VD;

public:
    PointContiWeights(OGRwkbGeometryType geom_type, const std::vector<OGRGeometry *> &geoms);

    virtual ~PointContiWeights();

protected:
    bool is_queen;
    std::vector<double> xs;
    std::vector<double> ys;
    double x_orig_min;
    double x_orig_max;
    double y_orig_min;
    double y_orig_max;
    double orig_scale;

    const int INSIDE = 0; // 0000
    const int LEFT = 1;   // 0001
    const int RIGHT = 2;  // 0010
    const int BOTTOM = 4; // 0100
    const int TOP = 8;    // 1000

protected:
    virtual Gda::GalElement *CreateWeights();

    void ReadPoints();

    std::list<int>* getCellList(
            const VD::cell_type& cell,
            std::map<std::pair<int,int>, std::list<int>* >& pt_to_id_list,
            std::vector<std::pair<int,int> >& int_pts);

    bool clipEdge(const VD::edge_type& edge,
                  std::vector<std::pair<int,int> >& int_pts,
                  const double& xmin, const double& ymin,
                  const double& xmax, const double& ymax,
                  double& x0, double& y0,
                  double& x1, double& y1);

    bool clipInfiniteEdge(const VD::edge_type& edge,
                          std::vector<std::pair<int,int> >& int_pts,
                          const double& xmin, const double& ymin,
                          const double& xmax, const double& ymax,
                          double& x0, double& y0,
                          double& x1, double& y1);

    bool clipFiniteEdge(const VD::edge_type& edge,
                        std::vector<std::pair<int,int> >& int_pts,
                        const double& xmin, const double& ymin,
                        const double& xmax, const double& ymax,
                        double& x0, double& y0,
                        double& x1, double& y1);

    bool ClipToBB(double& x0, double& y0, double& x1, double& y1,
                  const double& xmin, const double& ymin,
                  const double& xmax, const double& ymax);

    int ComputeOutCode(const double& x, const double& y,
                       const double& xmin, const double& ymin,
                       const double& xmax, const double& ymax);

    bool isVertexOutsideBB(const VD::vertex_type& vertex,
                           const double& xmin,
                           const double& ymin,
                           const double& xmax,
                           const double& ymax);
};
#endif //SPATIALWEIGHTS_CONTIGUITYWEIGHTS_H
