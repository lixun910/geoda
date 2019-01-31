//
// Created by Xun Li on 12/20/18.
//
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <ogrsf_frmts.h>

#include "../OGRTools/OGRDataUtils.h"
#include "../OGRTools/OGRShapeUtils.h"
#include "Downloader.h"
#include "Roads.h"

#ifndef PI
#define PI 3.141592653589793238463
#endif

#ifndef PI_OVER_180
#define PI_OVER_180 0.017453292519943
#endif

#ifndef EARTH_RADIUS
#define EARTH_RADIUS 6372795  // meters
#endif

using namespace OSMTools;

static inline std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

Roads::Roads()
{
    // max_query_area_size=50*1000*50*1000
    base_url = "http://www.overpass-api.de/api/interpreter";
}

Roads::~Roads() {}

int Roads::GetNumNodes()
{
    return id_map.size();
}

int Roads::GetNumEdges(std::vector<std::string>& nodes)
{
    boost::unordered_map<std::string, int> node_dict;
    int num_edges = 0;
    boost::unordered_map<std::string,
            std::vector<std::pair<std::string, double> > >::iterator it;
    for (it=edge_dict.begin(); it!=edge_dict.end(); ++it) {
        node_dict[it->first] = 1;
        num_edges += it->second.size();
        for (size_t i=0; i<it->second.size(); i++) {
            node_dict[it->second[i].first] = 1;
        }
    }
    boost::unordered_map<std::string, int>::iterator node_it;
    for (node_it=node_dict.begin(); node_it!=node_dict.end(); ++node_it) {
        nodes.push_back(node_it->first);
    }
    return num_edges;
}

double Roads::ComputeArcDist(int from, int to)
{
    double lat1 = lat_arr[from];
    double lon1 = lon_arr[from];
    double lat2 = lat_arr[to];
    double lon2 = lon_arr[to];
    // degree to rad
    lat1 = lat1 * PI_OVER_180;
    lon1 = lon1 * PI_OVER_180;
    lat2 = lat2 * PI_OVER_180;
    lon2 = lon2 * PI_OVER_180;

    // this is the haversine formula which is particularly well-conditioned
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = pow(sin(dlat/2.0), 2);
    double b = cos(lat1) * cos(lat2) * pow(sin(dlon/2.0),2);
    return 2 * EARTH_RADIUS * asin(sqrt(a+b));
}

const char* Roads::get_json_path(const char *shp_path)
{
    int sz = strlen(shp_path);
    char* json_path = new char[sz+1];
    json_path = strcpy(json_path, shp_path);
    strcpy(&json_path[sz-3], "json");
    return json_path;
}

bool Roads::download_by_bbox(double lat_min, double lng_min,
                            double lat_max, double lng_max,
                            OSMTools::RoadType road_type,
                            const char* osm_filter,
                            const char *json_path)
{
    std::ostringstream ss;
    ss << base_url << "?";
    ss << "data=[out:json][timeout:180];(";
    ss << osm_filter;
    ss << "(" << lat_min << "," << lng_min << ",";
    ss << lat_max << "," << lng_max << ")";
    ss << ";>;);out;";

    std::string url = ss.str();
    Downloader down;
    bool success = down.Get(url.c_str(), 0, json_path);
    return success;
}

bool Roads::DownloadByBoundingBox(double lat_min, double lng_min,
        double lat_max, double lng_max,
        OSMTools::RoadType road_type,  const char* osm_filter,
        const char *shp_path)
{
    bool flag = false;

    if (strlen(osm_filter) == 0) return false;

    const char* json_path = get_json_path(shp_path);

    flag = download_by_bbox(lat_min, lng_min, lat_max, lng_max, road_type,
                            osm_filter, json_path);
    if (!flag) return false;

    ReadOSMNodes(json_path);
    SaveToShapefile(shp_path);

    return flag;
}

bool Roads::DownloadByMapOutline(OGREnvelope* bbox, OGRGeometry *contour,
                                 OSMTools::RoadType road_type,
                                 const char *osm_filter, const char *shp_path)
{
    bool flag = false;

    if (strlen(osm_filter) == 0) return false;

    const char* json_path = get_json_path(shp_path);

    double lat_min = bbox->MinY;
    double lng_min = bbox->MinX;
    double lat_max = bbox->MaxY;
    double lng_max = bbox->MaxX;

    flag = download_by_bbox(lat_min, lng_min, lat_max, lng_max, road_type,
                            osm_filter, json_path);
    if (!flag) return false;

    ReadOSMNodes(json_path);
    SaveToShapefile(shp_path, contour);

    //std::vector<OGRFeature*> features = OGRDataUtils::GetFeatures(shp_path, 0);
    //std::vector<OGRFeature*> results = OGRShapeUtils::GetFeaturesWithin(features, contour);
    //OGRDataUtils::SaveFeatures(results, shp_path, "roads");

    return flag;
}

std::string Roads::GetValueFromLine(std::string line, bool quoted)
{
    // example:  "id": 237667011,
    // example:  "name": "King Drive & 25th Street",
    if (line.empty()) return "";

    size_t start = line.rfind(":");
    if (start == std::string::npos) return "";

    size_t end = line.length() - 1;
    if (line.find(",") != std::string::npos) {
        end = end - 1; // ending with comma
    }
    std::string tmp = line.substr(start + 1, end - start);

    if (quoted) {
        start = tmp.find("\"");
        if (start != std::string::npos) {
            end = tmp.length() - 2; // right-quote
            tmp = tmp.substr(start + 1, end - start);
        }
    }
    return trim(tmp);
}

void Roads::ReadOSMNodes(const char *file_name)
{
    if (file_name == 0) return;

    id_map.clear();
    lat_arr.clear();
    lon_arr.clear();

    int state = 0, cnt = 0, way_cnt = 0, start = 0, end = 0;
    std::ifstream infile(file_name);
    std::string line, tmp, tmp_way;
    std::vector<std::string> tmp_way_nodes;
    std::vector<std::string> tmp_way_property;

    while (std::getline(infile, line)) {
        if (line.find("node") != std::string::npos &&
            line.find("type") != std::string::npos) {
            state = 1;
            continue;
        } else if (line.find("way") != std::string::npos &&
                   line.find("type") != std::string::npos) {
            state = 5;
            continue;
        }

        if (state == 1) {
            // when node is found previously
            if (line.find("id") != std::string::npos) {
                tmp = GetValueFromLine(line, false);
                if (!tmp.empty()) {
                    tmp = trim(tmp);
                    id_map[tmp] = cnt++;

                    state = 2;
                }
            }
        } else if (state == 2) {
            // when id is found
            if (line.find("lat") != std::string::npos) {
                tmp = GetValueFromLine(line, false);
                if (!tmp.empty()) {
                    lat_arr.push_back(std::atof(tmp.c_str()));
                    state = 3;
                }
            }
        } else if (state == 3) {
            // when lat is found
            if (line.find("lon") != std::string::npos) {
                tmp = GetValueFromLine(line, false);
                if (!tmp.empty()) {
                    lon_arr.push_back(std::atof(tmp.c_str()));
                    state = 4;
                }
            }

        } else if (state == 5) {
            // way id
            if (line.find("id") != std::string::npos) {
                tmp_way = GetValueFromLine(line);
                if (!tmp_way.empty()) {
                    way_arr.push_back(tmp_way);
                    if (tmp_way_property.empty() == false) {
                        way_properties.push_back(tmp_way_property);
                    }
                    tmp_way_nodes.clear();
                    tmp_way_property.clear();
                    tmp_way_property.resize(5);
                    state = 6;
                }
            }
        } else if (state == 6) {
            if (line.find("nodes") != std::string::npos) {
                // start parsing nodes
                state = 7;
                continue;
            }
        } else if (state == 7) {
            if (line.find("]") != std::string::npos) {
                // end parsing nodes
                ways.push_back(tmp_way_nodes);
                state = 8;
                continue;
            } else if (line.empty()==false){
                tmp = line;
                if (line.find(",") != std::string::npos) {
                    tmp = line.substr(0, line.length()-1); // ending comma
                }
                tmp = trim(tmp);
                if (node_cnt_dict.find(tmp) == node_cnt_dict.end()) {
                    node_cnt_dict[tmp] = 1;
                } else {
                    node_cnt_dict[tmp]++;
                }
                tmp_way_nodes.push_back(tmp);
            }
        } else if (state == 8) {
            if (line.find("highway") != std::string::npos) {
                tmp = GetValueFromLine(line);
                tmp_way_property[0] = tmp;
            } else if (line.find("\"name\"") != std::string::npos) {
                tmp = GetValueFromLine(line);
                tmp_way_property[1] = tmp;
            } else if (line.find("county") != std::string::npos) {
                tmp = GetValueFromLine(line);
                tmp_way_property[2] = tmp;
            } else if (line.find("oneway") != std::string::npos) {
                tmp = GetValueFromLine(line);
                tmp_way_property[3] = tmp;
            } else if (line.find("maxspeed") != std::string::npos) {
                tmp = GetValueFromLine(line);
                tmp_way_property[4] = tmp;
            }
        }
    }
    // record last way property
    way_properties.push_back(tmp_way_property);
}

void Roads::SaveEdgesToShapefile(const char *file_name)
{
    std::string shp_file_name = file_name;
    shp_file_name = shp_file_name.substr(0, shp_file_name.rfind("."));
    shp_file_name = shp_file_name + ".shp";

    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL ) {
        printf( "%s driver not available.\n", pszDriverName );
        return;
    }
    GDALDataset *poDS;
    poDS = poDriver->Create(shp_file_name.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL ) {
        printf( "Creation of output file failed.\n" );
        return;
    }
    OGRLayer *poLayer;
    poLayer = poDS->CreateLayer("roads", NULL, wkbLineString, NULL );
    if( poLayer == NULL ) {
        printf( "Layer creation failed.\n" );
        GDALClose( poDS );
        return;
    }
    // wayid, highway, name, county, oneway, maxspeed, length,
    const char* prop_names[255] = {"wayid", "highway", "name", "county", "oneway", "maxspeed", "from", "to"};
    for (size_t i=0; i<6; ++i) {
        OGRFieldDefn oField(prop_names[i], OFTString );
        oField.SetWidth(32);
        if( poLayer->CreateField( &oField ) != OGRERR_NONE ) {
            printf( "Creating Name field failed.\n" );
            GDALClose( poDS );
            return;
        }
    }
    OGRFieldDefn oField("dist_m", OFTReal);
    if( poLayer->CreateField( &oField ) != OGRERR_NONE ) {
        printf( "Creating Name field failed.\n" );
        GDALClose( poDS );
        return;
    }
    for (size_t i=0; i<ways.size(); ++i) {
        std::vector<std::string> valid_nodes;
        for (size_t j=0; j<ways[i].size(); ++j) {
            std::string tmp_id = ways[i][j];
            if (node_cnt_dict[tmp_id] > 1) {
                valid_nodes.push_back(tmp_id);
            }
        }
        if (valid_nodes.empty()) continue;

        for (size_t j=0; j<valid_nodes.size()-1; ++j) {
            std::string from = valid_nodes[j];
            std::string to = valid_nodes[j+1];
            OGRFeature *poFeature;
            poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
            for (size_t c=0; c<6; ++c) {
                if (c == 0) {
                    std::string eid = way_arr[i];
                    poFeature->SetField(c, eid.c_str());
                } else {
                    const char *val = way_properties[i][c-1].c_str();
                    poFeature->SetField(c, val);
                }
            }
            int from_idx = id_map[from];
            int to_idx = id_map[to];
            double dist = ComputeArcDist(from_idx, to_idx);
            poFeature->SetField(6, dist);

            OGRLineString line;
            OGRPoint pt1;
            pt1.setX(lon_arr[from_idx]);
            pt1.setY(lat_arr[from_idx]);
            line.addPoint(&pt1);
            OGRPoint pt2;
            pt2.setX(lon_arr[to_idx]);
            pt2.setY(lat_arr[to_idx]);
            line.addPoint(&pt2);
            poFeature->SetGeometry(&line);
            if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE ) {
                printf( "Failed to create feature in shapefile.\n" );
                GDALClose( poDS );
                return;
            }
            OGRFeature::DestroyFeature( poFeature );

            edge_dict[from].push_back(std::make_pair(to, dist));
            if (boost::iequals(way_properties[i][3], "yes") == false) {
                edge_dict[to].push_back(std::make_pair(from, dist));
            }
        }
    }
    GDALClose( poDS );
}

void Roads::SaveToShapefile(const char *shp_file_name, OGRGeometry *contour)
{
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL ) {
        printf( "%s driver not available.\n", pszDriverName );
        return;
    }
    GDALDataset *poDS;
    poDS = poDriver->Create(shp_file_name, 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL ) {
        printf( "Creation of output file failed.\n" );
        return;
    }
    OGRSpatialReference dest_sr;
    dest_sr.importFromEPSG(4326); // use lat/lon

    OGRLayer *poLayer;
    poLayer = poDS->CreateLayer("roads", &dest_sr, wkbLineString, NULL );
    if( poLayer == NULL ) {
        printf( "Layer creation failed.\n" );
        GDALClose( poDS );
        return;
    }
    // wayid, highway, name, county, oneway, maxspeed, length,
    const char* prop_names[255] = {"wayid", "highway", "name", "county", "oneway", "maxspeed"};
    for (size_t i=0; i<6; ++i) {
        OGRFieldDefn oField(prop_names[i], OFTString );
        oField.SetWidth(32);
        if( poLayer->CreateField( &oField ) != OGRERR_NONE ) {
            printf( "Creating Name field failed.\n" );
            GDALClose( poDS );
            return;
        }
    }
    OGRFieldDefn oField("Length", OFTReal);
    if( poLayer->CreateField( &oField ) != OGRERR_NONE ) {
        printf( "Creating Name field failed.\n" );
        GDALClose( poDS );
        return;
    }
    for (size_t i=0; i<ways.size(); ++i) {
        OGRLineString line;
        for (size_t j=0; j<ways[i].size(); ++j) {
            std::string node_id = ways[i][j];
            if (id_map.find(node_id) != id_map.end()) {
                int idx = id_map[node_id];
                OGRPoint pt;
                pt.setX(lon_arr[idx]);
                pt.setY(lat_arr[idx]);
                line.addPoint(&pt);
            }
        }
        if (contour) {
            if (line.Within(contour) == false) continue;
        }
        OGRFeature *poFeature;
        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
        for (size_t c=0; c<6; ++c) {
            if (c == 0) {
                std::string eid = way_arr[i];
                poFeature->SetField(c, eid.c_str());
            } else {
                const char *val = way_properties[i][c-1].c_str();
                poFeature->SetField(c, val);
            }
        }

        poFeature->SetGeometry(&line);
        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE ) {
            printf( "Failed to create feature in shapefile.\n" );
            GDALClose( poDS );
            return;
        }
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
}

void Roads::SaveCSVToShapefile(const char *file_name)
{
    std::string shp_file_name = file_name;
    shp_file_name = shp_file_name.substr(0, shp_file_name.rfind("."));
    shp_file_name = shp_file_name + ".shp";

    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL ) {
        printf( "%s driver not available.\n", pszDriverName );
        return;
    }
    GDALDataset *poDS;
    poDS = poDriver->Create(shp_file_name.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL ) {
        printf( "Creation of output file failed.\n" );
        return;
    }
    OGRLayer *poLayer;
    poLayer = poDS->CreateLayer("roads", NULL, wkbLineString, NULL );
    if( poLayer == NULL ) {
        printf( "Layer creation failed.\n" );
        GDALClose( poDS );
        return;
    }

    std::string line;
    std::ifstream infile(file_name);
    std::string delimeter = ",";
    while (std::getline(infile, line)) {
        std::vector<std::string> vec;
        boost::algorithm::split(vec, line, boost::is_any_of(delimeter));

        OGRFeature *poFeature;
        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
        OGRLineString line;

        std::string node1 = vec[0];
        std::string node2 = vec[1];

        size_t node1_idx = id_map[node1];
        size_t node2_idx = id_map[node2];

        OGRPoint pt1;
        pt1.setX(lon_arr[node1_idx]);
        pt1.setY(lat_arr[node1_idx]);
        line.addPoint(&pt1);

        OGRPoint pt2;
        pt2.setX(lon_arr[node2_idx]);
        pt2.setY(lat_arr[node2_idx]);
        line.addPoint(&pt2);

        poFeature->SetGeometry(&line);
        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE ) {
            printf( "Failed to create feature in shapefile.\n" );
            GDALClose( poDS );
            return;
        }
        OGRFeature::DestroyFeature( poFeature );
    }
    GDALClose( poDS );
}
