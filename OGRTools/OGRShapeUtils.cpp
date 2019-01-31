//
// Created by Xun Li on 1/29/19.
//

#include "OGRShapeUtils.h"

OGREnvelope* OGRShapeUtils::GetBBox(const char* ds_path, int layer_idx,
                                    OGRSpatialReference* dest_sr)
{
    OGREnvelope* pEnvelope = NULL;

    GDALAllRegister();
    GDALDataset *poDS;
    poDS = (GDALDataset*) GDALOpenEx(ds_path, GDAL_OF_VECTOR, NULL, NULL, NULL);
    if( poDS == NULL ) {
        return pEnvelope;
    }
    OGRLayer *poLayer = NULL;
    int n_layers = poDS->GetLayerCount();
    if (layer_idx < 0 || layer_idx > n_layers -1 ) {
        GDALClose(poDS);
        return pEnvelope;
    }
    poLayer = poDS->GetLayer(layer_idx);
    if (poLayer == NULL) {
        GDALClose(poDS);
        return pEnvelope;
    }
    OGRSpatialReference* source_sr = poLayer->GetSpatialRef();
    OGRCoordinateTransformation *poCT = NULL;
    if (dest_sr && source_sr) {
        poCT = OGRCreateCoordinateTransformation(source_sr, dest_sr);
    }

    pEnvelope = new OGREnvelope();
    if (poLayer->GetExtent(pEnvelope) != OGRERR_NONE) {
        GDALClose(poDS);
        return pEnvelope;
    }
    if (poCT) {
        OGRPoint min_p, max_p;
        min_p.setX( pEnvelope->MinX);
        min_p.setY( pEnvelope->MinY);
        max_p.setX( pEnvelope->MaxX);
        max_p.setY( pEnvelope->MaxY);
        min_p.transform(poCT);
        max_p.transform(poCT);
        pEnvelope->MinX = min_p.getX();
        pEnvelope->MinY = min_p.getY();
        pEnvelope->MaxX = max_p.getX();
        pEnvelope->MaxY = max_p.getY();
    }
    GDALClose(poDS);
    return pEnvelope;
}

OGRGeometry* OGRShapeUtils::GetBBoxShape(const char *ds_path, int layer_idx,
                                         OGRSpatialReference* dest_sr)
{
    OGREnvelope* bbox = GetBBox(ds_path, layer_idx, dest_sr);
    OGRPoint min_p, max_p;
    min_p.setX( bbox->MinX);
    min_p.setY( bbox->MinY);
    max_p.setX( bbox->MaxX);
    max_p.setY( bbox->MaxY);
    delete bbox;

    OGRPolygon* polygon = new OGRPolygon();
    OGRLinearRing* ring = new OGRLinearRing();
    ring->addPoint(bbox->MinX, bbox->MinY);
    ring->addPoint(bbox->MaxX, bbox->MaxY);
    ring->closeRings();
    polygon->addRing(ring);

    return polygon;
}

OGRGeometry* OGRShapeUtils::GetMapOutline(std::vector<OGRFeature*> features)
{
    std::vector<OGRGeometry*> geoms;
    for (size_t i=0; i<features.size(); ++i) {
        geoms.push_back(features[i]->GetGeometryRef());
    }
    return GetMapOutline(geoms);
}

OGRGeometry* OGRShapeUtils::GetMapOutline(std::vector<OGRGeometry *> geoms)
{
    OGRMultiPolygon geo_col;
    for ( int i=0; i < geoms.size(); i++ ) {
        OGRGeometry* geometry= geoms[i];
        OGRwkbGeometryType eType = wkbFlatten(geometry->getGeometryType());
        if (eType == wkbPolygon || eType == wkbCurvePolygon ) {
            geo_col.addGeometry(geometry);

        } else if (eType == wkbMultiPolygon) {
            OGRMultiPolygon* mpolygon = (OGRMultiPolygon *) geometry;
            int n_geom = mpolygon->getNumGeometries();
            // if there is more than one polygon, then we need to count which
            // part is processing accumulatively
            for (size_t i = 0; i < n_geom; i++ ){
                OGRGeometry* ogrGeom = mpolygon->getGeometryRef(i);
                geo_col.addGeometry(ogrGeom);
            }
        } else {
            return NULL;
        }
    }
    OGRGeometry* ogr_contour = geo_col.UnionCascaded();
    return ogr_contour;
}

double OGRShapeUtils::GetShapeLength(OGRGeometry *geom_ref,
                                     OGRCoordinateTransformation *poCT)
{
    if (geom_ref == NULL) return 0.0;

    OGRwkbGeometryType eType = wkbFlatten(geom_ref->getGeometryType());

    if (eType == wkbPoint || eType == wkbMultiPolygon ||
        eType == wkbPolygon || eType == wkbMultiPolygon) {
        return 0.0;
    }

    double len = 0.0;
    OGRGeometry* geom = geom_ref;
    if (poCT) geom = geom_ref->clone();

    // here we can't use eType to detect if geom is wkbPolygon or wkbMultiPolygon
    OGRMultiLineString* mline = dynamic_cast<OGRMultiLineString*>(geom);
    if (mline) {
        OGRGeometryCollection *poCol = (OGRGeometryCollection*)geom;
        for(size_t i=0; i< poCol->getNumGeometries(); ++i) {
            OGRGeometry* ogrGeom = poCol->getGeometryRef(i);
            OGRLineString* poRing = static_cast<OGRLineString*>(ogrGeom);
            len += poRing->get_Length();
        }
    } else {
        OGRLineString* poRing = (OGRLineString*)geom;
        if (poRing) len = poRing->get_Length();
    }

    if (poCT) delete geom;
    return len;
}

double OGRShapeUtils::GetShapeArea(OGRGeometry *geom_ref,
                                   OGRCoordinateTransformation *poCT)
{
    if (geom_ref == NULL) return 0.0;

    OGRwkbGeometryType eType = wkbFlatten(geom_ref->getGeometryType());

    if (eType == wkbPoint || eType == wkbMultiPolygon ||
        eType == wkbLineString || eType == wkbMultiLineString) {
        return 0.0;
    }

    double area = 0.0;
    OGRGeometry* geom = geom_ref;
    if (poCT) geom = geom_ref->clone();

    // here we can't use eType to detect if geom is wkbPolygon or wkbMultiPolygon
    OGRMultiPolygon* mpoly = dynamic_cast<OGRMultiPolygon*>(geom);
    if (mpoly) {
        area = mpoly->get_Area();
    } else {
        OGRPolygon* poly = dynamic_cast<OGRPolygon*>(geom);
        if (poly) area = poly->get_Area();
    }

    if (poCT) delete geom;
    return area;
}

std::vector<double> OGRShapeUtils::GetShapeLengths(OGRwkbGeometryType eType,
        std::vector<OGRFeature *> features, OGRSpatialReference* source_sr,
        bool is_arc, OGRShapeUtils::DistUnit d_unit, std::vector<bool> &undefs)
{
    OGRSpatialReference dest_sr;
    dest_sr.importFromEPSG(3857); // using a projection with units as meters
    OGRCoordinateTransformation *poCT = NULL;
    if (is_arc && source_sr && source_sr->IsSame(&dest_sr) == false) {
        poCT = OGRCreateCoordinateTransformation(source_sr, &dest_sr);
    }

    size_t n = features.size();
    std::vector<double> results(n, 0.0);

    for (size_t i=0; i<n; ++i) {
        OGRFeature* feat = features[i];
        OGRGeometry* geom_ref = feat->GetGeometryRef();
        if (geom_ref == NULL) {
            undefs[i] = true;
            continue;
        }
        double len = GetShapeLength(geom_ref, poCT); //meter
        if (is_arc) {
            if (d_unit == DistUnit::kilometer) {
                len = len / 1000.0; //km
            }  else if (d_unit == DistUnit::mile) {
                len = len * 0.000621371; // miles
            }
        }
        results[i] = len;
    }

    return results;
}

std::vector<double> OGRShapeUtils::GetShapeAreas(OGRwkbGeometryType eType,
        std::vector<OGRFeature *> features, OGRSpatialReference *source_sr,
        bool is_arc, OGRShapeUtils::DistUnit d_unit, std::vector<bool> &undefs)
{
    OGRSpatialReference dest_sr;
    dest_sr.importFromEPSG(3857); // using a projection with units as meters
    OGRCoordinateTransformation *poCT = NULL;
    if (is_arc && source_sr && source_sr->IsSame(&dest_sr) == false) {
        poCT = OGRCreateCoordinateTransformation(source_sr, &dest_sr);
    }

    size_t n = features.size();
    std::vector<double> results(n, 0.0);

    for (size_t i=0; i<n; ++i) {
        OGRFeature* feat = features[i];
        OGRGeometry* geom_ref = feat->GetGeometryRef();
        if (geom_ref == NULL) {
            undefs[i] = true;
            continue;
        }
        // square meters
        double area = GetShapeArea(geom_ref, poCT);
        if (is_arc) {
            if (d_unit == DistUnit::kilometer) {
                area = area / 1000000.0; //sq km
            }  else if (d_unit == DistUnit::mile) {
                area = area * 0.386102 / 1000000.0; // sq miles
            }
        }
        results[i] = area;
    }

    return results;
}

std::vector<OGRFeature*> OGRShapeUtils::GetFeaturesWithin(std::vector<OGRFeature*> features, OGRGeometry *contour,
                                                    bool delete_outside)
{
    if (contour == NULL) return features;

    std::vector<OGRFeature*> results;

    size_t n = features.size();
    for (size_t i=0; i<n; ++i) {
        bool keep = false;
        OGRFeature* feat = features[i];
        OGRGeometry* geom = feat->GetGeometryRef();
        if (geom) {
            if (geom->Within(contour)) {
                results.push_back(feat);
                keep = true;
            }
        }
        if (delete_outside && !keep) {
            OGRFeature::DestroyFeature(feat);
        }
    }

    return results;
}
