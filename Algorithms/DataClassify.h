//
//  DataClassify.hpp
//  GeoDa
//
//  Created by Xun Li on 2/1/19.
//

#ifndef DataClassify_hpp
#define DataClassify_hpp

#include <vector>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>

class ClassifyUtils
{
protected:
    static uint64_t gda_user_seed;

    static void pick_rand_breaks(std::vector<int>& b, int N,
                                 boost::uniform_01<boost::mt19937>& X);

    static double calc_gvf(const std::vector<int>& b,
                           const std::vector<double>& v, double gssd);
    
    static void unique_to_normal_breaks(const std::vector<int>& u_val_breaks,
        const std::vector<std::pair<double, std::pair<int, int> > >& u_val_mapping,
        std::vector<int>& n_breaks);

    static std::vector<int> sort(std::vector<double>& vals);

    static double percentile(double x, std::vector<double> &v);
    
public:
    static std::vector<double> NaturalBreaks(const std::vector<double>& vals,
                                             const std::vector<bool>& undefs,
                                             int n_breaks);

    static std::vector<double> QuantileBreaks(const std::vector<double>& vals,
                                             const std::vector<bool>& undefs,
                                             int n_breaks);

    static std::vector<double> EqualBreaks(const std::vector<double>& vals,
                                             const std::vector<bool>& undefs,
                                             int n_breaks);
};

#endif /* DataClassify_hpp */
