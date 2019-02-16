//
// Created by Xun Li on 2/14/19.
//

#include <float.h>
#include <algorithm>
#include <boost/unordered_map.hpp>
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

Gda::GalElement *PointContiWeights::CreateWeights() {
    return 0;
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

    typedef std::pair<int,int> int_pair;
    typedef std::list<int> id_list;
    typedef std::list<const VD::vertex_type*> v_list;

    std::map<int_pair, id_list* > pt_to_id_list;
    // for each unique point, the list of cells at that point
    std::map<int_pair, id_list* >::iterator pt_to_id_list_iter;

    std::map<int_pair, std::set<id_list* > > pt_to_nbr_sets;
    // for each unique point, the set of lists of points that are neighbors
    std::map<int_pair, std::set<id_list* > >::iterator pt_to_nbr_sets_iter;

    std::vector<boost::polygon::point_data<int> > points;

    std::vector<int_pair> int_pts(n_geoms);

    for (size_t i=0; i<n_geoms; i++) {
        int xp = (xs[i] - x_orig_min) * p;
        int yp = (ys[i] - y_orig_min) * p;
        points.push_back(boost::polygon::point_data<int>(xp, yp));
        int_pts[i] = std::make_pair(xp, yp);
    }

    for (int i=0; i<n_geoms; i++) {
        pt_to_id_list_iter = pt_to_id_list.find(int_pts[i]);
        if (pt_to_id_list_iter == pt_to_id_list.end()) {
            pt_to_id_list[int_pts[i]] = new id_list;
        }
        pt_to_id_list[int_pts[i]]->push_back(i);
    }

    // Voronoi diagram construction
    VD vd;
    boost::polygon::construct_voronoi(points.begin(), points.end(), &vd);

    std::vector<std::set<std::list<int> > > nbr_set_array;
    VD::const_cell_iterator cell_it;
    const VD::edge_type* edge;
    const VD::cell_type* cell;

    for (cell_it = vd.cells().begin(); cell_it != vd.cells().end(); ++cell_it) {
        unsigned long idx = cell_it->source_index();
        int_pair key = int_pts[idx];
        std::set<id_list* >& nbr_set = pt_to_nbr_sets[key];

        edge = cell_it->incident_edge();
        v_list verts;
        do {
            id_list* nbr_list = getCellList(*(edge->twin()->cell()),
                                            pt_to_id_list, int_pts);
            if (!nbr_list) {
                return;
            }
            double x0, y0, x1, y1;
            if (clipEdge(*edge, int_pts,
                         bb_xmin, bb_ymin, bb_xmax, bb_ymax,
                         x0, y0, x1, y1)) {
                nbr_set.insert(nbr_list);
            }
            if (is_queen) { // add all cells that share each edge vertex
                if (edge->vertex0() &&
                    !isVertexOutsideBB(*edge->vertex0(), bb_xmin, bb_ymin,
                                       bb_xmax, bb_ymax)) {
                    verts.push_back(edge->vertex0());
                }
                if (edge->vertex1() &&
                    !isVertexOutsideBB(*edge->vertex1(), bb_xmin, bb_ymin,
                                       bb_xmax, bb_ymax)) {
                    verts.push_back(edge->vertex1());
                }
            }
            edge = edge->next();
        } while (edge != cell_it->incident_edge());
    }
}

std::list<int> *PointContiWeights::getCellList(const VD::cell_type &cell,
                                               std::map<std::pair<int, int>, std::list<int> *> &pt_to_id_list,
                                               std::vector<std::pair<int, int> > &int_pts) {
    std::map<std::pair<int,int>, std::list<int>* >::iterator iter;
    iter = pt_to_id_list.find(int_pts[cell.source_index()]);
    if (iter == pt_to_id_list.end()) {
        return 0;
    }
    return iter->second;
}

bool PointContiWeights::clipEdge(const VD::edge_type &edge,
                                 std::vector<std::pair<int, int> > &int_pts, const double &xmin, const double &ymin,
                                 const double &xmax, const double &ymax, double &x0, double &y0, double &x1,
                                 double &y1) {
    if (edge.is_finite()) {
        return clipFiniteEdge(edge, int_pts, xmin, ymin, xmax, ymax,
                              x0, y0, x1, y1);
    } else {
        return clipInfiniteEdge(edge, int_pts, xmin, ymin, xmax, ymax,
                                x0, y0, x1, y1);
    }
}

bool PointContiWeights::clipInfiniteEdge(const VD::edge_type &edge,
                                         std::vector<std::pair<int, int> > &int_pts, const double &xmin,
                                         const double &ymin, const double &xmax, const double &ymax, double &x0,
                                         double &y0, double &x1, double &y1) {
    const VD::cell_type& cell1 = *edge.cell();
    const VD::cell_type& cell2 = *edge.twin()->cell();
    double origin_x, origin_y, direction_x, direction_y;
    // Infinite edges could not be created by two segment sites.
    if (cell1.contains_point() && cell2.contains_point()) {
        double p1_x = (double) int_pts[cell1.source_index()].first;
        double p1_y = (double) int_pts[cell1.source_index()].second;
        double p2_x = (double) int_pts[cell2.source_index()].first;
        double p2_y = (double) int_pts[cell2.source_index()].second;
        origin_x = ((p1_x + p2_x) * 0.5);
        origin_y = ((p1_y + p2_y) * 0.5);
        direction_x = (p1_y - p2_y);
        direction_y = (p2_x - p1_x);
    } else {
        // This case should never happen for point maps.
        //("Warning! one clipInfiniteEdge cells contains a segment!");
        return false;
    }
    double side = xmax - xmin;
    double koef =
            side / (std::max)(fabs(direction_x), fabs(direction_y));
    if (edge.vertex0() == NULL) {
        x0 = origin_x - direction_x * koef;
        y0 = origin_y - direction_y * koef;
    } else {
        x0 = edge.vertex0()->x();
        y0 = edge.vertex0()->y();
    }
    if (edge.vertex1() == NULL) {
        x1 = origin_x + direction_x * koef;
        y1 = origin_y + direction_y * koef;
    } else {
        x1 = edge.vertex1()->x();
        y1 = edge.vertex1()->y();
    }
    return ClipToBB(x0, y0, x1, y1, xmin, ymin, xmax, ymax);
}

bool PointContiWeights::clipFiniteEdge(const boost::polygon::voronoi_edge<double> &edge,
                                       std::vector<std::pair<int, int> > &int_pts, const double &xmin,
                                       const double &ymin, const double &xmax, const double &ymax, double &x0,
                                       double &y0, double &x1, double &y1) {
    // we know that edge is finite, so both vertex0 and vertex1 are defined
    x0 = edge.vertex0()->x();
    y0 = edge.vertex0()->y();
    x1 = edge.vertex1()->x();
    y1 = edge.vertex1()->y();
    return ClipToBB(x0, y0, x1, y1, xmin, ymin, xmax, ymax);
}

// Cohen–Sutherland clipping algorithm clips a line from
// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with
// diagonal from (xmin, ymin) to (xmax, ymax).
// Based on http://en.wikipedia.org/wiki/Cohen-Sutherland_algorithm
// return false if line segment outside of bounding box
bool PointContiWeights::ClipToBB(double &x0, double &y0, double &x1, double &y1, const double &xmin, const double &ymin,
                                 const double &xmax, const double &ymax) {
    // compute outcodes for P0, P1,
    // and whatever point lies outside the clip rectangle
    int outcode0 = ComputeOutCode(x0, y0, xmin, ymin, xmax, ymax);
    int outcode1 = ComputeOutCode(x1, y1, xmin, ymin, xmax, ymax);
    bool accept = false;

    while (true) {
        if (!(outcode0 | outcode1)) {
            // Bitwise OR is 0. Trivially accept and get out of loop
            accept = true;
            break;
        } else if (outcode0 & outcode1) {
            // Bitwise AND is not 0. Trivially reject and get out of loop
            break;
        } else {
            // failed both tests, so calculate the line segment to clip
            // from an outside point to an intersection with clip edge
            double x, y;

            // At least one endpoint is outside the clip rectangle; pick it.
            int outcodeOut = outcode0 ? outcode0 : outcode1;

            // Now find the intersection point;
            // use formulas y = y0 + slope * (x - x0),
            //   x = x0 + (1 / slope) * (y - y0)
            if (outcodeOut & TOP) {
                // point is above the clip rectangle
                x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
                y = ymax;
            } else if (outcodeOut & BOTTOM) {
                // point is below the clip rectangle
                x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
                y = ymin;
            } else if (outcodeOut & RIGHT) {
                // point is to the right of clip rectangle
                y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
                x = xmax;
            } else if (outcodeOut & LEFT) {
                // point is to the left of clip rectangle
                y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
                x = xmin;
            }

            // Now we move outside point to intersection point to clip
            // and get ready for next pass.
            if (outcodeOut == outcode0) {
                x0 = x;
                y0 = y;
                outcode0 = ComputeOutCode(x0, y0, xmin, ymin, xmax, ymax);
            } else {
                x1 = x;
                y1 = y;
                outcode1 = ComputeOutCode(x1, y1, xmin, ymin, xmax, ymax);
            }
        }
    }
    return accept;
}

// Based on http://en.wikipedia.org/wiki/Cohen-Sutherland_algorithm
int PointContiWeights::ComputeOutCode(const double &x, const double &y, const double &xmin, const double &ymin,
                                      const double &xmax, const double &ymax) {
    int code = INSIDE;       // initialised as being inside of clip window

    if (x < xmin)           // to the left of clip window
        code |= LEFT;
    else if (x > xmax)      // to the right of clip window
        code |= RIGHT;
    if (y < ymin)           // below the clip window
        code |= BOTTOM;
    else if (y > ymax)      // above the clip window
        code |= TOP;

    return code;
}

bool PointContiWeights::isVertexOutsideBB(const boost::polygon::voronoi_vertex<double> &vertex, const double &xmin,
                                          const double &ymin, const double &xmax, const double &ymax) {
    double x = vertex.x();
    double y = vertex.y();
    return (x < xmin || x > xmax || y < ymin || y > ymax);
}
