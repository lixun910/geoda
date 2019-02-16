//
// Created by Xun Li on 2/14/19.
//

#ifndef SPATIALWEIGHTS_ABSTRACTWEIGHTS_H
#define SPATIALWEIGHTS_ABSTRACTWEIGHTS_H

#include <vector>
#include <boost/unordered_map.hpp>

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

namespace Gda {
    // GalElement is borrowed from GeoDa's internal usage.
    // The name is for GAL weights, however, it is extended to be used
    // as a generic Weights structure for each observation.
    class GalElement
    {
    public:
        GalElement();
        void SetSizeNbrs(size_t sz);
        void SetNbr(size_t pos, long n);
        void SetNbr(size_t pos, long n, double w);
        void SetNbrs(const GalElement& gal);
        const std::vector<long>& GetNbrs() const;
        const std::vector<double>& GetNbrWeights() const;
        void SortNbrs();
        long Size() const;
        long operator[](size_t n) const;
        double SpatialLag(const std::vector<double>& x) const;
        double SpatialLag(const double* x) const;
        double SpatialLag(const std::vector<double>& x, const int* perm) const;
        double GetRW(int idx);
        bool   Check(long nbrIdx);
        void   Update(const std::vector<bool>& undefs);

    protected:
        std::vector<long> nbr;
        std::vector<double> nbrWeight;
        std::vector<double> nbrAvgW;
        boost::unordered_map<long, int> nbrLookup; // nbr_id, idx_in_nbrWeight
    };
}



class AbstractWeights
{
public:
    AbstractWeights();

    AbstractWeights(const std::vector<int64_t> &m_ids);

    virtual ~AbstractWeights();

protected:
    // meta data (todo)
    const char* m_ds_name;

    std::vector<int64_t> m_ids;

    Gda::GalElement *m_weights;

public:
    const std::vector<int64_t> &getM_ids() const;

    void setM_ids(const std::vector<int64_t> &m_ids);

    bool Save(const char* file_path);

    Gda::GalElement *GetGalElements();
};


#endif //SPATIALWEIGHTS_ABSTRACTWEIGHTS_H
