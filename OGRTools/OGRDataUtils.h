#ifndef OGRUTILS_OGRDATAUTILS_H
#define OGRUTILS_OGRDATAUTILS_H

#include <vector>
#include <ogrsf_frmts.h>

class OGRDataUtils {
public:
    // get all layer names from data source
    static std::vector<const char*> GetLayerNames(const char* ds_path);

    static std::vector<OGRFeature*> GetFeatures(const char* ds_path,
                                                int layer_idx,
                                                OGRSpatialReference* dest_sr = NULL);

    static bool SaveFeaturesToShapefile(std::vector<OGRGeometry*> geoms,
                             const char* ds_path,
                             const char* layer_name, OGRwkbGeometryType geom_type);
};


#endif //OGRUTILS_OGRUTILS_H
