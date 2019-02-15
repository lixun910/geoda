//
//  ContiguityWeights.hpp
//  GeoDa
//
//  Created by Xun Li on 2/13/19.
//

#ifndef ContiguityWeights_hpp
#define ContiguityWeights_hpp

#include <vector>
#include <ogrsf_frmts.h>

namespace GDA {
    class ContiguityWeights
    {
    public:
    ContiguityWeights(std::vector<OGRGeometry*> geoms);
    virtual ~ContiguityWeights();
    };

    class PolygonContiguityWeights : public ContiguityWeights
    {
    public:
    PolygonContiguityWeights();
    virtual ~PolygonContiguityWeights();
    };

    class PointContiguityWeights: public ContiguityWeights
    {

    };

    class LineContiguityWeights: public ContiguityWeights
    {

    };
}
#endif /* ContiguityWeights_hpp */
