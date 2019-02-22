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

bool OGRDataUtils::SaveFeaturesToShapefile(std::vector<OGRGeometry *> geoms,
                                const char *ds_path,
                                const char *layer_name,
                                           OGRwkbGeometryType geom_type)
{
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL ) {
        printf( "%s driver not available.\n", pszDriverName );
        return false;
    }
    GDALDataset *poDS;
    poDS = poDriver->Create(ds_path, 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL ) {
        printf( "Creation of output file failed.\n" );
        return false;
    }
    OGRLayer *poLayer;
    poLayer = poDS->CreateLayer(layer_name, NULL, geom_type, NULL );
    if( poLayer == NULL ) {
        printf( "Layer creation failed.\n" );
        GDALClose( poDS );
        return false;
    }
    OGRFieldDefn oField("id", OFTInteger );
    oField.SetWidth(32);
    if( poLayer->CreateField( &oField ) != OGRERR_NONE ) {
        printf( "Creating Name field failed.\n" );
        GDALClose( poDS );
        return false;
    }
    for (size_t i=0; i<geoms.size(); ++i) {
        OGRFeature *poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn());
        poFeature->SetField(0, (int)i);
        poFeature->SetGeometry(geoms[i]);
        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE ) {
            printf( "Failed to create feature in shapefile.\n" );
            GDALClose( poDS );
            return false;
        }
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
    return true;
}
