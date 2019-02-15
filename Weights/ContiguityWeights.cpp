//
// Created by Xun Li on 2/14/19.
//

#include <float.h>
#include <algorithm>
#include <boost/unordered_map.hpp>
#include <boost/polygon/voronoi.hpp>
#include <boost/polygon/point_data.hpp>

#include "ContiguityWeights.h"


ContiguityWeights::ContiguityWeights() {}

ContiguityWeights::ContiguityWeights(OGRwkbGeometryType geom_type,
        const std::vector<OGRGeometry *> &geoms)
        : geom_type(geom_type), geoms(geoms) {
    n_geoms = geoms.size();
}

ContiguityWeights::~ContiguityWeights() {

}


PointContiWeights::PointContiWeights(OGRwkbGeometryType geom_type, const std::vector<OGRGeometry *> &geoms)
        : ContiguityWeights(geom_type, geoms),
        x_orig_min(DBL_MAX), x_orig_max(DBL_MIN),
        y_orig_min(DBL_MAX), y_orig_max(DBL_MIN) {

}

PointContiWeights::~PointContiWeights() {

}

GalElement *PointContiWeights::CreateWeights() {
    return ContiguityWeights::CreateWeights();
}

void PointContiWeights::ReadPoints() {
    double x, y;
    for (size_t i=0; i<n_geoms; ++i) {
        OGRPoint *pt = (OGRPoint*)geoms[i];
        x = pt->getX();
        y = pt->getY();
        xs.push_back(x);
        ys.push_back(y);
        if (x < x_orig_min) x_orig_min = x;
        else if (x > x_orig_max) x_orig_max = x;
        if (y < y_orig_min) y_orig_min = y;
        else if (y > y_orig_max) y_orig_max = y;
    }

    double x_orig_range = x_orig_max - x_orig_min;
    double y_orig_range = y_orig_max - y_orig_min;
    orig_scale = std::max(x_orig_range, y_orig_range);

    if (orig_scale == 0) orig_scale = 1;
    double big_dbl = 1073741824; // 2^30
    double p = big_dbl / orig_scale;
    // Add 2% offset to the bounding rectangle
    double bb_pad = 0.02;

    // note data has been translated to origin and scaled
    double bb_xmin = -bb_pad * big_dbl;
    double bb_xmax = x_orig_range * p + bb_pad * big_dbl;
    double bb_ymin = -bb_pad * big_dbl;
    double bb_ymax = y_orig_range * p + bb_pad * big_dbl;

    std::vector<boost::polygon::point_data<int> > points;
    for (size_t i=0; i<n_geoms; i++) {
        int xp = (xs[i] - x_orig_min) * p;
        int yp = (ys[i] - y_orig_min) * p;
        points.push_back(boost::polygon::point_data<int>(xp, yp));
    }

    // Voronoi diagram construction
    boost::polygon::voronoi_diagram<double> vd;
    boost::polygon::construct_voronoi(points.begin(), points.end(), &vd);

    boost::polygon::voronoi_diagram<double>::const_cell_iterator cell_it;
    for (cell_it = vd.cells().begin(); cell_it != vd.cells().end(); ++cell_it) {
        unsigned long idx = cell_it->source_index();
        cell_it->incident_edge();
    }
}
