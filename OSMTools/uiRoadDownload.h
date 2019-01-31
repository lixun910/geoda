//
// Created by Xun Li on 12/23/18.
//

#ifndef OSMTOOLSPROJECT_UIROADDOWNLOAD_H
#define OSMTOOLSPROJECT_UIROADDOWNLOAD_H

#include <wx/wx.h>

namespace OSMTools {

    class uiRoadDownload : public wxFrame {

    public:
        uiRoadDownload(const wxString &title = _("Download OSM Roads"));

        uiRoadDownload(double top, double bottom,
                double left, double right,
                const wxString &title = _("Download OSM Roads"));

    protected:
        void InitControls();

        bool CheckInput(wxTextCtrl *tc, double& val);

        void OnOKClick(wxCommandEvent& event);

        void OnCancelClick(wxCommandEvent& event);

        void OnRoadTypeChange(wxCommandEvent& event);

        void OnOpenFile(wxCommandEvent& event);

        void download_from_bbox();

        void download_from_input_ds();

        wxString get_output_path();

        OSMTools::RoadType get_road_type();
        
    protected:
        wxNotebook *nb;
        wxTextCtrl *tc_bbox_up;
        wxTextCtrl *tc_bbox_left;
        wxTextCtrl *tc_bbox_right;
        wxTextCtrl *tc_bbox_bottom;
        wxTextCtrl *tc_infile_path;
        wxBitmapButton *btn_open_file;
        wxRadioButton *rb_bbox;
        wxRadioButton *rb_outline;
        wxRadioButton *rb_bbox_outline;
        wxCheckBox *cb_buffer;
        wxTextCtrl *tc_buffer;
        wxChoice *ch_way_type;
        wxTextCtrl *tc_overpass;

        const wxString wildcard = "Data Files (*.shp, *.geojson, *.json, *.sqlite, *.gpkg, *.gdb, *.gml, *.kml)|*.shp;*.geojson;*.json;*.sqlite;*.gpkg;*.gdb;*.gml;*.kml";
        const wxString overpass_road = "way[\"highway\"][\"highway\"!~\"cycleway|bus_stop|elevator|footway|path|pedestrian|steps|track|proposed|construction|bridleway|abandoned|platform|raceway|service\"][\"motor_vehicle\"!~\"no\"][\"motorcar\"!~\"no\"][\"service\"!~\"parking|parking_aisle|driveway|emergency_access\"]";
        const wxString overpass_walk = "way[\"highway\"][\"highway\"!~\"motor|proposed|construction|abandoned|platform|raceway\"][\"foot\"!~\"no\"][\"service\"!~\"private\"]";
        const wxString overpass_bike = "way[\"highway\"][\"highway\"!~\"footway|corridor|motor|proposed|construction|abandoned|platform|raceway\"][\"bicycle\"!~\"no\"][\"service\"!~\"private\"]";
        /*
         [out:json][bbox:33.431173703367314,-111.99351352639498,33.44582077131526,-111.98149723000824];
         ((
         relation[type=route][route~"^(subway|monorail|aerialway|bus|trolleybus|ferry|train|tram)$"];
         relation[type=public_transport][public_transport=stop_area];
         );)->.result;
         */
    };
}

#endif //OSMTOOLSPROJECT_UIROADDOWNLOAD_H
