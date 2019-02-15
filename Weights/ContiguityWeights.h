//
// Created by Xun Li on 2/14/19.
//

#ifndef SPATIALWEIGHTS_CONTIGUITYWEIGHTS_H
#define SPATIALWEIGHTS_CONTIGUITYWEIGHTS_H

#include <vector>
#include <ogrsf_frmts.h>
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

    virtual GalElement *CreateWeights();
};

class PointContiWeights : public ContiguityWeights {
public:
    PointContiWeights(OGRwkbGeometryType geom_type, const std::vector<OGRGeometry *> &geoms);

    virtual ~PointContiWeights();

protected:
    std::vector<double> xs;
    std::vector<double> ys;
    double x_orig_min;
    double x_orig_max;
    double y_orig_min;
    double y_orig_max;
    double orig_scale;

protected:
    GalElement *CreateWeights() override;

    void ReadPoints();
};
#endif //SPATIALWEIGHTS_CONTIGUITYWEIGHTS_H
