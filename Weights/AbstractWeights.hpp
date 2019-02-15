//
//  AbstractWeights.hpp
//  GeoDa
//
//  Created by Xun Li on 2/13/19.
//

#ifndef AbstractWeights_hpp
#define AbstractWeights_hpp

#include <vector>
#include <boost/unordered_map.hpp>

namespace GDA {
    /**
     * This design will integrate geometric based weights (contiguity and
     * distance weights) and variable based weights into a stand-alone module.
     *
     * The weights creation will be implemented for (1) geometric dataset, e.g.
     * points (multi-points will not be supported since Thessien Polygons can't
     * be created appropriately), polygons/multi-polygons, and lines/multi-lines
     * which only contiguity weights will be implemented using the topology.
     * (2) table dataset, e.g. using distance of variable(s) between observations
     * to determine the neighboring relationship in variable space. The former
     * one is often called spatial weights, and the latter one is called
     * non-spatial weights.
     *
     * Weights meta-data are generated when creating weights. The meta-data
     * describes the weights, e.g. the dataset used to create weights; the type
     * of created weights; the method used to create weights; the parameters of
     * the method; the statistic informmation of created weights;
     *
     * Created weights (spatial and non-spatial) can be accessed directly in
     * memory, and also can be saved as or loaded from a file on local hard disk.
     * The supported weights formats include:
     *  GAL, GWT/KWT, SWM, MAT, H5
     *
     * The weights creation will be running in multi-threading paradigm for
     * better performance.
     *
     */

    //typedef std::vector<std::vector<std::pair<int, double> > > Weights;
    typedef std::vector<boost::unordered_map<int, double> > Weights;

    class AbstractWeights
    {
    protected:
    std::vector<long long> m_ids;

    // meta data
    const char* m_ds_name;

    public:

    };
}
#endif /* AbstractWeights_hpp */
