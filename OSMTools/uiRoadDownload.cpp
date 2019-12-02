//
// Created by Xun Li on 12/23/18.
//
#include <wx/notebook.h>
#include <wx/artprov.h>
#include <wx/filename.h>
#include <wx/filefn.h>

#include "../OGRTools/OGRShapeUtils.h"
#include "../OGRTools/OGRDataUtils.h"
#include "Roads.h"
#include "uiRoadDownload.h"

using namespace OSMTools;

const wxString uiRoadDownload::wildcard = "Data Files (*.shp, *.geojson, *.json, *.sqlite, *.gpkg, *.gdb, *.gml, *.kml)|*.shp;*.geojson;*.json;*.sqlite;*.gpkg;*.gdb;*.gml;*.kml";

const wxString uiRoadDownload::overpass_road = "way[\"highway\"][\"highway\"!~\"cycleway|bus_stop|elevator|footway|path|pedestrian|steps|track|proposed|construction|bridleway|abandoned|platform|raceway|service\"][\"motor_vehicle\"!~\"no\"][\"motorcar\"!~\"no\"][\"service\"!~\"parking|parking_aisle|driveway|emergency_access\"]";

const wxString uiRoadDownload::overpass_walk = "way[\"highway\"][\"highway\"!~\"motor|proposed|construction|abandoned|platform|raceway\"][\"foot\"!~\"no\"][\"service\"!~\"private\"]";

const wxString uiRoadDownload::overpass_bike = "way[\"highway\"][\"highway\"!~\"footway|corridor|motor|proposed|construction|abandoned|platform|raceway\"][\"bicycle\"!~\"no\"][\"service\"!~\"private\"]";

uiRoadDownload::uiRoadDownload(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(500, 440))
{
    InitControls();
}

uiRoadDownload::uiRoadDownload(double top, double bottom, double left,
                               double right, const wxString &cwd,
                               const wxString &title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(500, 440))
{
    InitControls();
    working_dir = cwd;
    wxString s_top, s_bottom, s_left, s_right;
    s_top << top;
    s_bottom << bottom;
    s_left << left;
    s_right << right;
    tc_bbox_up->SetValue(s_top);
    tc_bbox_bottom->SetValue(s_bottom);
    tc_bbox_left->SetValue(s_left);
    tc_bbox_right->SetValue(s_right);
}

void uiRoadDownload::InitControls()
{
    // main body
    wxPanel *panel = new wxPanel(this, -1);

    nb = new wxNotebook(panel, -1);

    // bbox page:
    wxPanel *page_bbox = new wxPanel(nb, -1);
    wxStaticBox *st_bbox = new wxStaticBox(page_bbox, -1, _("Extent"),
            wxDefaultPosition, wxSize(300,100));
    wxSize latlon_sz(100,-1);
    tc_bbox_up = new wxTextCtrl(st_bbox, wxID_ANY, _("Top"),
            wxDefaultPosition, latlon_sz);
    tc_bbox_left = new wxTextCtrl(st_bbox, wxID_ANY, _("Left"),
            wxDefaultPosition, latlon_sz);
    tc_bbox_right = new wxTextCtrl(st_bbox, wxID_ANY, _("Right"),
            wxDefaultPosition, latlon_sz);
    tc_bbox_bottom = new wxTextCtrl(st_bbox, wxID_ANY, _("Bottom"),
            wxDefaultPosition, latlon_sz);
    wxGridSizer* bbox_sizer = new wxGridSizer(3, 3, 10, 10);
    bbox_sizer->Add(new wxStaticText(st_bbox, -1, ""));
    bbox_sizer->Add(tc_bbox_up, 0, wxEXPAND);
    bbox_sizer->Add(new wxStaticText(st_bbox, -1, ""));
    bbox_sizer->Add(tc_bbox_left, 0, wxEXPAND);
    bbox_sizer->Add(new wxStaticText(st_bbox, -1, ""));
    bbox_sizer->Add(tc_bbox_right, 0, wxEXPAND);
    bbox_sizer->Add(new wxStaticText(st_bbox, -1, ""));
    bbox_sizer->Add(tc_bbox_bottom, 0, wxEXPAND);
    bbox_sizer->Add(new wxStaticText(st_bbox, -1, ""));
    st_bbox->SetSizer(bbox_sizer);
    wxBoxSizer* bbox_v_sizer = new wxBoxSizer(wxVERTICAL);
    bbox_v_sizer->Add(st_bbox, 0, wxEXPAND, 10);
    page_bbox->SetSizer(bbox_v_sizer);
    nb->AddPage(page_bbox, _("Bounding Box"));

    // input file page:
    wxPanel *infile_panel = new wxPanel(nb, -1);
    wxBoxSizer* infile_h_sizer = new wxBoxSizer(wxHORIZONTAL);
    infile_h_sizer->Add(new wxStaticText(infile_panel, -1, _("Select a file:")));
    tc_infile_path = new wxTextCtrl(infile_panel, -1, "",
            wxDefaultPosition, wxSize(300, -1));
    infile_h_sizer->Add(tc_infile_path);
    wxBitmap bmp = wxArtProvider::GetBitmap(wxART_FILE_OPEN);
    btn_open_file = new wxBitmapButton(infile_panel, -1,
            bmp, wxDefaultPosition, wxSize(18,18));
    infile_h_sizer->Add(btn_open_file, 0, wxLEFT, 10);
    wxStaticBox *st = new wxStaticBox(infile_panel, -1, _("Options:"));
    rb_bbox = new wxRadioButton(st, -1,
            _("Use bounding box of input datasource"),
            wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    rb_outline = new wxRadioButton(st, -1,
            _("Use outline of input datasource"));
    //rb_bbox_outline = new wxRadioButton(st, -1,
    //        _("Use outline + motorway(~bbox) of input datasource"));
    wxBoxSizer* stb_v_sizer = new wxBoxSizer(wxVERTICAL);
    stb_v_sizer->Add(rb_bbox, 0, wxALL, 5);
    stb_v_sizer->Add(rb_outline, 0, wxALL, 5);
    rb_bbox->SetValue(true); // default using bbox
    //stb_v_sizer->Add(rb_bbox_outline, 0, wxALL, 5);
    st->SetSizer(stb_v_sizer);
    wxBoxSizer* infile_v_sizer = new wxBoxSizer(wxVERTICAL);
    infile_v_sizer->Add(infile_h_sizer,0, wxTOP, 10);
    infile_v_sizer->Add(st, 1, wxTOP | wxEXPAND, 10);

    infile_panel->SetSizer(infile_v_sizer);
    nb->AddPage(infile_panel, _("Input data source"));

    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(nb, 1, wxEXPAND);

    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    wxButton *ok = new wxButton(panel, -1, _("Ok"));
    wxButton *cancel = new wxButton(panel, -1, _("Cancel"));
    hbox2->Add(ok);
    hbox2->Add(cancel);

    wxBoxSizer *hbox3 = new wxBoxSizer(wxHORIZONTAL);
    cb_buffer = new wxCheckBox(panel, -1, _("Buffer query area:"));
    tc_buffer = new wxTextCtrl(panel, -1, "5");
    cb_buffer->SetValue(true);
    hbox3->Add(cb_buffer);
    hbox3->Add(tc_buffer, 0, wxRIGHT, 5);
    hbox3->Add(new wxStaticText(panel, -1, "%"));

    wxBoxSizer *hbox4 = new wxBoxSizer(wxHORIZONTAL);
    ch_way_type = new wxChoice(panel, -1);
    ch_way_type->Append("Drive");
    ch_way_type->Append("Walk");
    ch_way_type->Append("Bike");
    ch_way_type->SetSelection(0);
    hbox4->Add(new wxStaticText(panel, -1, "Select road type:"));
    hbox4->Add(ch_way_type, 0, wxLEFT, 5);
    hbox4->Add(new wxStaticText(panel, -1, " Overpass-API query text:"));

    wxBoxSizer *hbox5 = new wxBoxSizer(wxHORIZONTAL);
    tc_overpass = new wxTextCtrl(panel, -1, overpass_road, wxDefaultPosition,
                                 wxSize(450, 100));
    hbox5->Add(tc_overpass, 0, wxEXPAND);

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(hbox1, 1, wxEXPAND);
    vbox->Add(hbox3, 0, wxEXPAND | wxALL, 10);
    vbox->Add(hbox4, 0, wxEXPAND | wxALL, 10);
    vbox->Add(hbox5, 0, wxEXPAND | wxALL, 10);
    vbox->Add(hbox2, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 10);
    panel->SetSizer(vbox);

    Centre();

    nb->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &uiRoadDownload::OnTabChange, this);
    rb_bbox->Bind(wxEVT_RADIOBUTTON, &uiRoadDownload::OnInputDSOptionCheck, this);
    rb_outline->Bind(wxEVT_RADIOBUTTON, &uiRoadDownload::OnInputDSOptionCheck, this);
    btn_open_file->Bind(wxEVT_BUTTON, &uiRoadDownload::OnOpenFile, this);
    ch_way_type->Bind(wxEVT_CHOICE, &uiRoadDownload::OnRoadTypeChange, this);
    ok->Bind(wxEVT_BUTTON, &uiRoadDownload::OnOKClick, this);
    cancel->Bind(wxEVT_BUTTON, &uiRoadDownload::OnCancelClick, this);
}

void uiRoadDownload::OnOpenFile(wxCommandEvent &event)
{
    wxFileDialog openFileDialog(this, _("Open file"), working_dir, "",
                                wildcard, wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL) return;

    wxString path = openFileDialog.GetPath();
    tc_infile_path->SetValue(path);
}

wxString uiRoadDownload::get_output_path()
{
    wxString save_ttl = _("Save file");
    wxString filter = "ESRI Shapefiles (*.shp)|*.shp";
    wxString path;

    wxFileDialog openFileDialog(this, save_ttl, working_dir, "",
                                filter, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (openFileDialog.ShowModal() == wxID_OK) {
        path = openFileDialog.GetPath();
    }
    return path;
}

void uiRoadDownload::OnTabChange(wxCommandEvent &event)
{
    if (nb->GetSelection() == 0) {
        cb_buffer->Enable();
        tc_buffer->Enable();
    } else {
        wxCommandEvent ev;
        OnInputDSOptionCheck(ev);
    }
}

void uiRoadDownload::OnInputDSOptionCheck(wxCommandEvent &event)
{
    bool flag = rb_outline->GetValue();
    cb_buffer->Enable(!flag);
    tc_buffer->Enable(!flag);
}

void uiRoadDownload::OnRoadTypeChange(wxCommandEvent &event)
{
    int sel = ch_way_type->GetSelection();
    if (sel == 0) {
        tc_overpass->SetValue(overpass_road);
    } else if (sel == 1) {
        tc_overpass->SetValue(overpass_walk);
    } else if (sel == 2) {
        tc_overpass->SetValue(overpass_bike);
    }
}

bool uiRoadDownload::CheckInput(wxTextCtrl *tc, double& val)
{
    wxString tmp_val, error_msg;

    tmp_val = tc->GetValue();
    if (tmp_val.IsEmpty() ) {
        error_msg = _("Please enter values of bounding box.");
    } else {
        if (tmp_val.ToDouble(&val) == false) {
            error_msg = _("Please check input values are valid");
        }
    }
    if (error_msg.IsEmpty() == false) {
        wxMessageDialog msg_dlg(this, error_msg,
                _("Error"),
                wxOK | wxOK_DEFAULT | wxICON_INFORMATION);
        msg_dlg.ShowModal();
        return false;
    }
    return true;
}

void uiRoadDownload::OnCancelClick(wxCommandEvent& event)
{
    Destroy();
}

void uiRoadDownload::OnOKClick(wxCommandEvent& event)
{
    int page_index = nb->GetSelection();
    bool success = false;
    if (page_index == 0) {
        success = download_from_bbox();
    } else if (page_index == 1) {
        success = download_from_input_ds();
    }

    if (success) {
        wxMessageDialog msg_dlg(this, _("Save OSM roads to file successfully."),
                                _("Info"),
                                wxOK | wxOK_DEFAULT | wxICON_INFORMATION);
        msg_dlg.ShowModal();
    }
}

bool uiRoadDownload::download_from_input_ds()
{
    wxString ds_path_s = tc_infile_path->GetValue();
    if (ds_path_s.IsEmpty()) {
        wxString msg = _("Please select an input data source.");
        wxMessageDialog dlg(this, msg, _("Info"),wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return false;
    }
    const char* ds_path_ref = ds_path_s.mb_str(wxConvUTF8);
    char* ds_path = new char[strlen(ds_path_ref)];
    ds_path = strcpy(ds_path, ds_path_ref);

    std::vector<const char*> layer_names;
    layer_names = OGRDataUtils::GetLayerNames((const char*)ds_path);
    if (layer_names.empty()) {
        wxString msg = _("The input datasource has no layer, please try another datasource.");
        wxMessageDialog dlg(this, msg, _("Info"),wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return false;
    }

    int n_layers = layer_names.size();
    wxString *choices = new wxString[n_layers];
    for (size_t i=0; i<n_layers; i++)  {
        choices[i] = layer_names[i];
        delete layer_names[i];
    }
    wxString ttl = _("Layer names");
    wxString prompt = _("Please select the layer name to connect:");
    wxSingleChoiceDialog choiceDlg(NULL, prompt, ttl, n_layers,
                                   (const wxString*)choices);
    if (choiceDlg.ShowModal() != wxID_OK) {
        return false;
    }

    int sel_idx = choiceDlg.GetSelection();
    wxString sel_layer_name = choiceDlg.GetStringSelection();

    OGRSpatialReference dest_sr;
    dest_sr.importFromEPSG(4326); // always use lat/lon

    bool success = false;
    if (rb_bbox->IsEnabled() && rb_bbox->GetValue() == true) {
        success = use_bbox_of_ds(ds_path, sel_idx, &dest_sr);
    } else {
        success = use_outline_of_ds(ds_path, sel_idx, &dest_sr);
    }
    return success;
}

bool uiRoadDownload::use_bbox_of_ds(const char *ds_path, int sel_idx,
                                    OGRSpatialReference *dest_sr)
{
    wxString out_path = get_output_path();
    OSMTools::RoadType road_type = get_road_type();
    wxString osm_filter = tc_overpass->GetValue();
    OGREnvelope* bbox = OGRShapeUtils::GetBBox(ds_path, sel_idx, dest_sr);

    long buffer_val;
    wxString buffer_txt = tc_buffer->GetValue();
    if (buffer_txt.ToLong(&buffer_val) ) {
        double buffer_ratio = 1;
        buffer_ratio = 1 + ((double)buffer_val / 100.0);
        double lat_min = bbox->MinY;
        double lng_min = bbox->MinX;
        double lat_max = bbox->MaxY;
        double lng_max = bbox->MaxX;
        double w = lat_max - lat_min;
        double h = lng_max - lng_min;
        double offset_w = (w * buffer_ratio - w) / 2.0;
        double offset_h = (h * buffer_ratio - h) / 2.0;

        lat_min = lat_min - offset_w;
        lat_max = lat_max + offset_w;
        lng_min = lng_min - offset_h;
        lng_max = lng_max + offset_h;

        bbox->MinY = lat_min;
        bbox->MinX = lng_min;
        bbox->MaxY = lat_max;
        bbox->MaxX = lng_max;
    }

    OSMTools::Roads roads;
    roads.DownloadByBoundingBox(bbox, road_type, osm_filter.mb_str(wxConvUTF8),
                                out_path.mb_str(wxConvUTF8));

    // clean-up memory before return
    delete bbox;
    return true;
}

bool uiRoadDownload::use_outline_of_ds(const char* ds_path, int sel_idx,
                                       OGRSpatialReference* dest_sr)
{
    std::vector<OGRFeature*> features;
    features = OGRDataUtils::GetFeatures((const char*)ds_path, sel_idx, dest_sr);
    if (features.empty()) {
        wxString msg = _("The selected input data source is empty, please try another data source.");
        wxMessageDialog dlg(this, msg, _("Info"),wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return false;
    }

    OGRGeometry *contour = OGRShapeUtils::GetMapOutline(features);
    if (contour == NULL) {
        wxString msg = _("Can't get map contour from input data source. Please check if the input datasource is polygon type.");
        wxMessageDialog dlg(this, msg, _("Info"),wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return false;
    }

    wxString out_path = get_output_path();
    OSMTools::RoadType road_type = get_road_type();
    wxString osm_filter = tc_overpass->GetValue();
    OGREnvelope* bbox = OGRShapeUtils::GetBBox(ds_path, sel_idx, dest_sr);

    OSMTools::Roads roads;
    roads.DownloadByMapOutline(bbox, contour, road_type,
                               osm_filter.mb_str(wxConvUTF8),
                               out_path.mb_str(wxConvUTF8));

    // clean-up memory before return
    delete bbox;
    delete contour;
    for (size_t i=0; i<features.size(); ++i) {
        delete features[i];
    }
    return true;
}

OSMTools::RoadType uiRoadDownload::get_road_type()
{
    OSMTools::RoadType road_type = OSMTools::drive;

    if (ch_way_type->GetSelection() == 1) road_type = OSMTools::walk;
    else if (ch_way_type->GetSelection() == 2) road_type = OSMTools::bike;

    return road_type;
}

bool uiRoadDownload::download_from_bbox()
{
    double lat_min, lng_min, lat_max, lng_max;
    if (CheckInput(tc_bbox_right, lat_max) == false) return false;
    if (CheckInput(tc_bbox_left, lat_min) == false) return false;
    if (CheckInput(tc_bbox_up, lng_max) == false) return false;
    if (CheckInput(tc_bbox_bottom, lng_min) == false) return false;

    OSMTools::RoadType road_type = get_road_type();

    if (cb_buffer->IsChecked()) {
        long buffer_val;
        wxString buffer_txt = tc_buffer->GetValue();
        if (buffer_txt.ToLong(&buffer_val) ) {
            double buffer_ratio = 1;
            buffer_ratio = 1 + ((double)buffer_val / 100.0);
            double w = lat_max - lat_min;
            double h = lng_max - lng_min;
            double offset_w = (w * buffer_ratio - w) / 2.0;
            double offset_h = (h * buffer_ratio - h) / 2.0;
            lat_min = lat_min - offset_w;
            lat_max = lat_max + offset_w;
            lng_min = lng_min - offset_h;
            lng_max = lng_max + offset_h;
        }
    }

    wxString out_path = get_output_path();
    wxString osm_filter = tc_overpass->GetValue();

    OGREnvelope bbox;
    bbox.MinY = lat_min;
    bbox.MinX = lng_min;
    bbox.MaxY = lat_max;
    bbox.MaxX = lng_max;

    OSMTools::Roads roads;
    roads.DownloadByBoundingBox(&bbox, road_type,
                                osm_filter.mb_str(wxConvUTF8),
                                out_path.mb_str(wxConvUTF8));
    return true;
}


