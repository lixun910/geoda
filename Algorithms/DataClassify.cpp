//
//  DataClassify.cpp
//  GeoDa
//
//  Created by Xun Li on 2/1/19.
//
#include <set>
#include <cfloat>
#include <boost/unordered_map.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "DataClassify.h"

uint64_t ClassifyUtils::gda_user_seed = 123456789;


/** Assume that b.size() <= N-1 */
void ClassifyUtils::pick_rand_breaks(std::vector<int>& b, int N,
                                     boost::uniform_01<boost::mt19937>& X)
{
    int num_breaks = b.size();
    if (num_breaks > N-1) return;

    std::set<int> s;
    while (s.size() != num_breaks) s.insert(1 + (N-1)*X());
    int cnt=0;
    for (std::set<int>::iterator it=s.begin(); it != s.end(); it++) {
        b[cnt++] = *it;
    }
    std::sort(b.begin(), b.end());
}

// translate unique value breaks into normal breaks given unique value mapping
void ClassifyUtils::unique_to_normal_breaks(const std::vector<int>& u_val_breaks,
    const std::vector<std::pair<double, std::pair<int, int> > >& u_val_mapping,
    std::vector<int>& n_breaks)
{
    if (n_breaks.size() != u_val_breaks.size()) {
        n_breaks.resize(u_val_breaks.size());
    }
    for (int i=0, iend=u_val_breaks.size(); i<iend; i++) {
        n_breaks[i] = u_val_mapping[u_val_breaks[i]].second.first; // first occu
    }
}

/** Assume input b and v is sorted.  If not, can sort
 with std::sort(v.begin(), v.end())
 We assume that b and v are sorted in ascending order and are
 valid (ie, no break indicies out of range and all categories
 have at least one value.
 gssd is the global sum of squared differences from the mean */
double ClassifyUtils::calc_gvf(const std::vector<int>& b,
                               const std::vector<double>& v, double gssd)
{
    int N = v.size();
    int num_cats = b.size()+1;
    double tssd=0; // total sum of local sums of squared differences
    for (size_t i=0; i<num_cats; i++) {
        int s = (i == 0) ? 0 : b[i-1];
        int t = (i == num_cats-1) ? N : b[i];

        double m=0; // local mean
        double ssd=0; // local sum of squared differences (variance)
        for (int j=s; j<t; j++) m += v[j];
        m /= ((double) t-s);
        for (int j=s; j<t; j++) ssd += (v[j]-m)*(v[j]-m);
        tssd += ssd;
    }

    return 1-(tssd/gssd);
}

std::vector<int> ClassifyUtils::sort(std::vector<double>& vals)
{
    std::vector<int> indices;
    std::vector<std::pair<double, int> > val_pairs;
    for (size_t i=0; i<vals.size(); ++i) {
        val_pairs.push_back(std::make_pair(vals[i], i));
    }
    std::sort(val_pairs.begin(), val_pairs.end());
    for (size_t i=0; i<val_pairs.size(); ++i) {
        indices.push_back(val_pairs[i].second);
        vals[i] = val_pairs[i].first;
    }
    return indices;
}

std::vector<double> ClassifyUtils::NaturalBreaks(const std::vector<double> &_vals,
                                                 const std::vector<bool> &undefs,
                                                 int n_breaks)
{
    std::vector<double> breaks;

    // copy to change the content of input data
    std::vector<double> vals = _vals;
    int num_obs = vals.size();

    // sort input array
    std::vector<int> indices = sort(vals);

    // if there are fewer unique values than number of categories,
    // we will automatically reduce the number of categories to the
    // number of unique values.
    boost::unordered_map<double, int> unique_values;
    // [(value, (first occur idx, last occur idx)), ...]
    std::vector<std::pair<double, std::pair<int, int> > > uv_mapping;

    double mean = 0.0, val;
    int valid_obs = 0, idx;
    std::vector<double> valid_vals;
    for (size_t i=0; i<num_obs; ++i) {
        if (undefs[ indices[i] ]) continue;
        val = vals[i];
        if (unique_values.find(val) == unique_values.end()) {
            unique_values[val] = i;
            uv_mapping.push_back(std::make_pair(val, std::make_pair(i, i)));
        } else {
            idx = unique_values[val];
            uv_mapping[idx].second.second = i;
        }
        valid_obs += 1;
        mean += val;
        valid_vals.push_back(val);
    }
    mean /= (double)valid_obs;

    int n_unique_values = unique_values.size();
    if (n_unique_values < n_breaks) n_breaks = n_unique_values;

    // gssd: global sum of squared differences from the mean
    double gssd = 0.0, tmp;
    for (size_t i=0; i<num_obs; ++i) {
        if (undefs[ indices[i] ]) continue;
        tmp = vals[i] - mean;
        gssd += tmp * tmp;
    }

    std::vector<int> rand_b(n_breaks-1);
    std::vector<int> best_breaks(n_breaks-1);
    std::vector<int> uv_rand_b(n_breaks-1);
    double max_gvf_found = 0;
    int max_gvf_ind = 0;

    // for 5000 permutations, 2200 obs, and 4 time periods, slow enough
    // make sure permutations is such that this total is not exceeded.
    double c = 5000*2200*4;
    int perms = c / ((double) num_obs);
    if (perms < 10) perms = 10;
    if (perms > 10000) perms = 10000;

    boost::mt19937 rng(gda_user_seed);
    boost::uniform_01<boost::mt19937> X(rng);

    for (size_t i=0; i<perms; ++i) {
        pick_rand_breaks(uv_rand_b, n_unique_values, X);
        // translate uv_rand_b into normal breaks
        unique_to_normal_breaks(uv_rand_b, uv_mapping, rand_b);
        double new_gvf = calc_gvf(rand_b, valid_vals, gssd);
        if (new_gvf > max_gvf_found) {
            max_gvf_found = new_gvf;
            max_gvf_ind = i;
            best_breaks = rand_b;
        }
    }

    breaks.resize(best_breaks.size());
    for (int i=0, iend=best_breaks.size(); i<iend; i++) {
        breaks[i] = vals[best_breaks[i]];
    }

    return breaks;
}

// v is a sorted array
double ClassifyUtils::percentile(double x, std::vector<double> &v)
{
    int N = v.size();
    double Nd = (double) N;
    double p_0 = (100.0/Nd) * (1.0-0.5);
    double p_Nm1 = (100.0/Nd) * (Nd-0.5);

    if (x <= p_0)
        return v[0];

    if (x >= p_Nm1)
        return v[N-1];

    for (int i=1; i<N; i++) {
        double p_i = (100.0/Nd) * ((((double) i)+1.0)-0.5);
        if (x == p_i) return v[i];
        if (x < p_i) {
            double p_im1 = (100.0/Nd) * ((((double) i))-0.5);
            return v[i-1] + Nd*((x-p_im1)/100.0)*(v[i]-v[i-1]);
        }
    }
    return v[N-1]; // execution should never get here
}

std::vector<double> ClassifyUtils::QuantileBreaks(const std::vector<double> &_vals,
                                                 const std::vector<bool> &undefs,
                                                 int n_breaks)
{
    std::vector<double> breaks;
    std::vector<double> vals = _vals;
    // sort input array
    std::vector<int> indices = sort(vals);
    double val;
    std::vector<double> valid_vals;
    for (size_t i=0; i<vals.size(); ++i) {
        if (undefs[ indices[i] ]) continue;
        val = vals[i];
        valid_vals.push_back(val);
    }

    for (size_t i=0; i<n_breaks; ++i) {
        double b = ((i+1)*100.0)/((double) n_breaks);
        val = percentile(b, vals);
        breaks.push_back(val);
    }
    return breaks;
}

std::vector<double> ClassifyUtils::EqualBreaks(const std::vector<double> &_vals,
                                               const std::vector<bool> &undefs,
                                               int n_breaks)
{
    std::vector<double> breaks;

    double min_v = DBL_MAX, max_v = DBL_MIN, v;
    for (size_t i=0; i<_vals.size();++i) {
        if (undefs[i]) continue;
        v = _vals[i];
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
    }

    double gap = (max_v - min_v) / n_breaks;
    for (size_t i=0; i<n_breaks; ++i) {
        v = v + gap;
        breaks.push_back(v);
    }
    return breaks;
}
