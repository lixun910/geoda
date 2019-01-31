#include "OGRDataUtils.h"

std::vector<const char*> OGRDataUtils::GetLayerNames(const char *ds_path)
{
    std::vector<const char*> results;
    GDALAllRegister();
    GDALDataset *poDS;
    poDS = (GDALDataset*) GDALOpenEx(ds_path, GDAL_OF_VECTOR, NULL, NULL, NULL);
    if( poDS == NULL ) {
        return results;
    }
    OGRLayer *poLayer = NULL;
    int n_layers = poDS->GetLayerCount();
    for (size_t i=0; i<n_layers; i++)  {
        poLayer = poDS->GetLayer(i);
        const char* name_ref = poLayer->GetName();
        char* name = new char[strlen(name_ref)];
        name = strcpy(name, name_ref);
        results.push_back(name);
    }
    GDALClose(poDS);
    return results;
}

std::vector<OGRFeature*> OGRDataUtils:: GetFeatures(const char* ds_path,
                                                    int layer_idx,
                                                    OGRSpatialReference* dest_sr)
{
    std::vector<OGRFeature*> results;
    GDALAllRegister();
    GDALDataset *poDS;
    poDS = (GDALDataset*) GDALOpenEx(ds_path, GDAL_OF_VECTOR, NULL, NULL, NULL);
    if( poDS == NULL ) {
        return results;
    }
    OGRLayer *poLayer = NULL;
    int n_layers = poDS->GetLayerCount();
    if (layer_idx < 0 || layer_idx > n_layers -1 ) {
        GDALClose(poDS);
        return results;
    }

    poLayer = poDS->GetLayer(layer_idx);
    if (poLayer == NULL) {
        GDALClose(poDS);
        return results;
    }

    OGRSpatialReference* source_sr = poLayer->GetSpatialRef();
    OGRCoordinateTransformation *poCT = NULL;
    if (dest_sr && source_sr) {
        poCT = OGRCreateCoordinateTransformation(source_sr, dest_sr);
    }

    poLayer->ResetReading();
    OGRFeature *poFeature;
    while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
        OGRFeature* feat = poFeature->Clone();
        OGRGeometry* geom = feat->GetGeometryRef();
        if (poCT) geom->transform(poCT);
        results.push_back(feat);
        OGRFeature::DestroyFeature(poFeature);
    }
    GDALClose(poDS);
    return results;
}

bool OGRDataUtils::SaveFeatures(std::vector<OGRFeature *> features,
                                const char *ds_path,
                                const char *layer_name)
{
    OGRFeature* feat = features[0];
    OGRField* field = feat->GetRawFieldRef(0);
    feat->GetFieldDefnRef(0);
    return false;

}
