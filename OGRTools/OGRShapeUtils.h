//
// Created by Xun Li on 1/29/19.
//

#ifndef OGRUTILS_OGRUTILS_H
#define OGRUTILS_OGRUTILS_H

#include <vector>
#include <ogrsf_frmts.h>

class OGRShapeUtils {
public:
    enum DistUnit {meter, kilometer, mile};

    // Get bounding box
    static OGREnvelope* GetBBox(const char* ds_path, int layer_idx,
                                OGRSpatialReference* dest_sr = NULL);

    static OGRGeometry* GetBBoxShape(const char* ds_path, int layer_idx,
                                OGRSpatialReference* dest_sr = NULL);

    // Get a map contour from a collection of geometries (polygons)
    static OGRGeometry* GetMapOutline(std::vector<OGRFeature*> geoms);

    static OGRGeometry* GetMapOutline(std::vector<OGRGeometry*> geoms);

    // Compute length of a shape (LineString or MultiLineString)
    static double GetShapeLength(OGRGeometry* geom,
                                 OGRCoordinateTransformation *poCT = NULL);

    // Compute area of a shape (Polygon or MultiPolygon)
    static double GetShapeArea(OGRGeometry* geom,
                               OGRCoordinateTransformation *poCT = NULL);

    // Compute length of a vector of shapes (LineString or MultiLineString)
    static std::vector<double> GetShapeLengths(OGRwkbGeometryType eType,
            std::vector<OGRFeature*> features, OGRSpatialReference* source_sr,
            bool is_arc, DistUnit d_unit, std::vector<bool>& undefs);

    // Compute area of a vector of shapes (Polygon or MultiPolygon)
    static std::vector<double> GetShapeAreas(OGRwkbGeometryType eType,
            std::vector<OGRFeature*> features, OGRSpatialReference* source_sr,
            bool is_arc, DistUnit d_unit, std::vector<bool>& undefs);

    // Returns all features within a contour shape
    static std::vector<OGRFeature*>
    GetFeaturesWithin(std::vector<OGRFeature*> features, OGRGeometry* contour,
            bool delete_outside=false);
};


#endif //OGRUTILS_OGRUTILS_H
