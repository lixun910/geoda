//
// Created by Xun Li on 2/14/19.
//

#include "WeightsIO.h"
#include "AbstractWeights.h"

Gda::GalElement::GalElement() {

}

bool Gda::GalElement::Check(long nbrIdx)
{
    if (nbrLookup.find(nbrIdx) != nbrLookup.end())
        return true;
    return false;
}

// return row standardized weights value
// called by Transpose Weights Matrix
double Gda::GalElement::GetRW(int idx)
{
    size_t sz = nbr.size();
    nbrAvgW.resize(sz);
    double sumW = 0.0;

    for (size_t i=0; i<sz; i++)
        sumW += nbrWeight[i];

    for (size_t i=0; i<sz; i++) {
        nbrAvgW[i] = nbrWeight[i] / sumW;
    }

    if (nbrLookup.find(idx) != nbrLookup.end())
        return nbrAvgW[nbrLookup[idx]];
    return 0;
}

void Gda::GalElement::SetSizeNbrs(size_t	sz)
{
    nbr.resize(sz);
    nbrWeight.resize(sz);
    for(size_t i=0; i<sz; i++) {
        nbrWeight[i] = 1.0;
    }
}

// which neighbor, what ID
void Gda::GalElement::SetNbr(size_t pos, long n)
{
    if (pos < nbr.size()) {
        nbr[pos] = n;
        nbrLookup[n] = pos;
    }
    // this should be called by GAL created only
    if (pos < nbrWeight.size()) {
        nbrWeight[pos] = 1.0;
    }
}

// which neighbor, what ID, what value
void Gda::GalElement::SetNbr(size_t pos, long n, double w)
{
    if (pos < nbr.size()) {
        nbr[pos] = n;
        nbrLookup[n] = pos;
    } else {
        nbr.push_back(n);
        nbrLookup[n] = pos;
    }

    // this should be called by GWT-GAL
    if (pos < nbrWeight.size()) {
        nbrWeight[pos] = w;
    } else {
        nbrWeight.push_back(w);
    }
}

// Update neighbor information on the fly using undefs information
// NOTE: this has to be used with a copy of weights (keep the original weights!)
void Gda::GalElement::Update(const std::vector<bool>& undefs)
{
    std::vector<int> undef_obj_positions;

    for (int i=0; i<nbr.size(); i++) {
        int obj_id = nbr[i];
        if (undefs[obj_id]) {
            int pos = nbrLookup[obj_id];
            undef_obj_positions.push_back(pos);
        }
    }

    if (undef_obj_positions.empty())
        return;

    // sort the positions in descending order, for removing from std::vector
    std::sort(undef_obj_positions.begin(),
              undef_obj_positions.end(), std::greater<int>());

    for (int i=0; i<undef_obj_positions.size(); i++) {
        int pos = undef_obj_positions[i];
        if (pos < nbr.size()) {
            nbrLookup.erase( nbr[pos] );
            nbr.erase( nbr.begin() + pos);
        }
        if (pos < nbrWeight.size()) {
            nbrWeight.erase( nbrWeight.begin() + pos);
        }
    }
}

void Gda::GalElement::SetNbrs(const GalElement& gal)
{
    size_t sz = gal.Size();
    nbr.resize(sz);
    nbrWeight.resize(sz);

    nbr = gal.GetNbrs();
    nbrLookup = gal.nbrLookup;
    nbrWeight = gal.GetNbrWeights();
    nbrLookup = gal.nbrLookup;
    nbrAvgW = gal.nbrAvgW;
}

const std::vector<long> & Gda::GalElement::GetNbrs() const
{
    return nbr;
}

const std::vector<double> & Gda::GalElement::GetNbrWeights() const
{
    return nbrWeight;
}

void Gda::GalElement::SortNbrs()
{
    std::sort(nbr.begin(), nbr.end(), std::greater<long>());
}

// Compute spatial lag for a contiguity weights matrix.
// Automatically performs standardization of the result.
double Gda::GalElement::SpatialLag(const std::vector<double>& x) const
{
    double lag = 0;
    size_t sz = Size();

    for (size_t i=0; i<sz; ++i) {
        lag += x[nbr[i]];
    }
    if (sz>1) lag /= (double) sz;

    return lag;
}

// Compute spatial lag for a contiguity weights matrix.
// Automatically performs standardization of the result.
double Gda::GalElement::SpatialLag(const double *x) const
{
    double lag = 0;
    size_t sz = Size();

    for (size_t i=0; i<sz; ++i) lag += x[nbr[i]];
    if (sz>1) lag /= (double) sz;

    return lag;
}

//
double Gda::GalElement::SpatialLag(const std::vector<double>& x,
                              const int* perm) const
{
    // todo: this should also handle ReadGWtAsGAL like previous 2 functions
    double lag = 0;
    size_t sz = Size();
    for (size_t i=0; i<sz; ++i) lag += x[perm[nbr[i]]];
    if (sz>1) lag /= (double) sz;
    return lag;
}

long Gda::GalElement::Size() const {
    return nbr.size();
}

long Gda::GalElement::operator[](size_t n) const {
    return nbr[n];
}


AbstractWeights::AbstractWeights() {

}

AbstractWeights::AbstractWeights(const std::vector<int64_t> &m_ids) {

}

AbstractWeights::~AbstractWeights() {

}

const std::vector<int64_t> &AbstractWeights::getM_ids() const {
    return m_ids;
}

void AbstractWeights::setM_ids(const std::vector<int64_t> &m_ids) {
    AbstractWeights::m_ids = m_ids;
}

bool AbstractWeights::Save(const char *file_path) {
    WeightsIO wio;
    return wio.Save(file_path);
}

Gda::GalElement *AbstractWeights::GetGalElements() {
    return 0;
}



