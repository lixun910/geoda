//
//  RoadDistancesDlg
//  GeoDa
//
//  Created by Xun Li on 9/4/18.
//
#include <vector>
#include <wx/wx.h>
#include <wx/notebook.h>

#include "../ShpFile.h"
#include "../Project.h"
#include "../Explore/MapLayer.hpp"
#include "../osm/TravelTool.h"
#include "RoadDistancesDlg.h"

using namespace std;


RoadDistancesDlg::RoadDistancesDlg(wxWindow* parent,
                                   Project* _project,
                                   const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(350, 250)),
    project(_project)
{
    wxString info = _("Please select a layer to compute travel distance "
                      "matrix on current map of road networks:");
    wxStaticText* st = new wxStaticText(this, wxID_ANY, info);
    
    map_list = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(160,-1));
    field_st = new wxStaticText(this, wxID_ANY,
                                _("Select ID Variable (Optional)"));
    field_list = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(100,-1));
    wxBoxSizer* mbox = new wxBoxSizer(wxHORIZONTAL);
    mbox->Add(map_list, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(field_st, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(field_list, 0, wxALIGN_CENTER | wxALL, 5);
    
    cbox = new wxBoxSizer(wxVERTICAL);
    cbox->Add(st, 0, wxALIGN_CENTER | wxALL, 15);
    cbox->Add(mbox, 0, wxALIGN_CENTER | wxALL, 10);

    wxButton* ok_btn = new wxButton(this, wxID_ANY, _("OK"), wxDefaultPosition,
                                    wxDefaultSize, wxBU_EXACTFIT);
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, _("Close"),
                            wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(ok_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(cancel_btn, 0, wxALIGN_CENTER | wxALL, 5);

    wxNotebook *nb = new wxNotebook(this, -1, wxDefaultPosition, wxDefaultSize);
    // data page:
    wxNotebookPage* vis_page = new wxNotebookPage(nb, -1, wxDefaultPosition,
                                                  wxSize(450, 150));
    nb->AddPage(vis_page, _("Data Setup"));

    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    cb_field_highway = new wxCheckBox(vis_page, -1, _("Field of highway type:"));
    co_field_highway = new wxChoice(vis_page, -1);
    hbox1->Add(cb_field_highway);
    hbox1->Add(co_field_highway, 0, wxLEFT, 5);
    hbox1->Add(new wxStaticText(vis_page, -1, _("(Optional)")));

    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    cb_field_speed = new wxCheckBox(vis_page, -1, _("Field of max speed:"));
    co_field_speed = new wxChoice(vis_page, -1);
    hbox2->Add(cb_field_speed);
    hbox2->Add(co_field_speed, 0, wxLEFT, 20);
    hbox2->Add(new wxStaticText(vis_page, -1, _("(Optional)")));

    wxBoxSizer *hbox3 = new wxBoxSizer(wxHORIZONTAL);
    cb_field_oneway = new wxCheckBox(vis_page, -1, _("Field of one-way:"));
    co_field_oneway = new wxChoice(vis_page, -1);
    hbox3->Add(cb_field_oneway);
    hbox3->Add(co_field_oneway, 0, wxLEFT, 33);
    hbox3->Add(new wxStaticText(vis_page, -1, _("(Optional)")));

    wxBoxSizer* bbox_v_sizer = new wxBoxSizer(wxVERTICAL);
    bbox_v_sizer->Add(hbox1, 0, wxALIGN_CENTER | wxALL, 10);
    bbox_v_sizer->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 5);
    bbox_v_sizer->Add(hbox3, 0, wxALIGN_CENTER | wxALL, 5);
    bbox_v_sizer->Fit(vis_page);
    vis_page->SetSizer(bbox_v_sizer);

    // network setup
    wxNotebookPage* road_page = new wxNotebookPage(nb, -1, wxDefaultPosition,
                                                   wxSize(450, 150));
    nb->AddPage(road_page, _("Speed Limit Setup"));

    wxBoxSizer *hbox4 = new wxBoxSizer(wxHORIZONTAL);
    tc_default_speed = new wxTextCtrl(road_page, -1, "10");
    hbox4->Add(new wxStaticText(road_page, -1, _("Default speed (km/hr):")));
    hbox4->Add(tc_default_speed);

    wxBoxSizer *hbox5 = new wxBoxSizer(wxHORIZONTAL);
    tc_speed_penalty = new wxTextCtrl(road_page, -1, "1.2");
    hbox5->Add(new wxStaticText(road_page, -1, _("Speed penalty:")));
    hbox5->Add(tc_speed_penalty);

    gd_speed = new wxGrid(road_page, -1, wxDefaultPosition, wxSize(300, 150));
    gd_speed->CreateGrid(36, 2, wxGrid::wxGridSelectRows);
    gd_speed->EnableEditing(true);
    gd_speed->SetDefaultCellAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    gd_speed->SetColLabelValue(0, "Way Type");
    gd_speed->SetColLabelValue(1, "Maximum Speed (km/hr)");
    InitGrid();
    wxBoxSizer* road_v_sizer = new wxBoxSizer(wxVERTICAL);
    road_v_sizer->Add(hbox4, 0, wxALIGN_CENTER | wxALL, 10);
    road_v_sizer->Add(hbox5, 0, wxALIGN_CENTER | wxALL, 5);
    road_v_sizer->Add(gd_speed, 1, wxEXPAND| wxALL, 10);
    road_page->SetSizer(road_v_sizer);

    cb_use_gpu = new wxCheckBox(this, -1, _("Use CPU+GPU hybrid solution. The ratio of GPU task:"));
    tc_gpu_ratio = new wxTextCtrl(this, -1, "0.2");
    wxBoxSizer* hbox6 = new wxBoxSizer(wxHORIZONTAL);
    hbox6->Add(cb_use_gpu, 0, wxALIGN_CENTER | wxALL, 5);
    hbox6->Add(tc_gpu_ratio, 0, wxALIGN_CENTER | wxALL, 5);

    int num_gpu = OSMTools::TravelBass::DetectGPU();
    if (num_gpu < 2) {
        cb_use_gpu->SetValue(false);
        cb_use_gpu->Disable();
        tc_gpu_ratio->Disable();
    } else {
        cb_use_gpu->SetValue(true);
    }

    vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(cbox, 0, wxALL, 15);
    vbox->Add(nb, 1, wxEXPAND | wxALL, 10);
    vbox->Add(hbox6, 0, wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizer(vbox);
    vbox->Fit(this);
    
    Center();
    
    map_list->Bind(wxEVT_CHOICE, &RoadDistancesDlg::OnLayerSelect, this);
    ok_btn->Bind(wxEVT_BUTTON, &RoadDistancesDlg::OnOK, this);
    
    InitMapList();
    field_st->Disable();
    field_list->Disable();

    InitDataSetup();
}

void RoadDistancesDlg::InitDataSetup()
{
    TableInterface* table_int = project->GetTableInt();
    int n_cols = table_int->GetNumberCols();
    for (size_t i=0; i<n_cols; ++i) {
        co_field_speed->Append(table_int->GetColName(i));
        co_field_highway->Append(table_int->GetColName(i));
        co_field_oneway->Append(table_int->GetColName(i));
    }

    int col_idx = table_int->FindColId("highway");
    if ( col_idx>= 0) {
        cb_field_highway->SetValue(true);
        co_field_highway->SetSelection(col_idx);
    }
    col_idx = table_int->FindColId("oneway");
    if ( col_idx>= 0) {
        cb_field_oneway->SetValue(true);
        co_field_oneway->SetSelection(col_idx);
    }
    col_idx = table_int->FindColId("maxspeed");
    if ( col_idx>= 0) {
        cb_field_speed->SetValue(true);
        co_field_speed->SetSelection(col_idx);
    }
}

void RoadDistancesDlg::InitMapList()
{
    map_list->Clear();
    map<wxString, BackgroundMapLayer*>::iterator it;
    BackgroundMapLayer* ml;
    for (it=project->bg_maps.begin(); it!=project->bg_maps.end(); it++) {
        ml = it->second;
        if (ml->GetShapeType() == Shapefile::POINT_TYP) {
            wxString name = it->first;
            map_list->Append(name);
        }
    }
    for (it=project->fg_maps.begin(); it!=project->fg_maps.end(); it++) {
        ml = it->second;
        if (ml->GetShapeType() == Shapefile::POINT_TYP) {
            wxString name = it->first;
            map_list->Append(name);
        }
    }
    if (map_list->GetCount() > 0) {
        // check the first item in list
        wxString name = map_list->GetString(0);
        UpdateFieldList(name);
    }
}

void RoadDistancesDlg::UpdateFieldList(wxString name)
{
    BackgroundMapLayer* ml = project->GetMapLayer(name);
    if (ml) {
        if (Shapefile::POINT_TYP == ml->GetShapeType()) {
            field_list->Clear();
            vector<wxString> field_names = ml->GetIntAndStringFieldNames();
            field_list->Append("");
            for (int i=0; i<field_names.size(); i++) {
                field_list->Append(field_names[i]);
            }
            field_list->Enable();
            field_st->Enable();
            
        } else {
            field_list->Clear();
            field_list->Disable();
            field_st->Disable();
        }
    }
}

void RoadDistancesDlg::OnLayerSelect(wxCommandEvent& e)
{
    int layer_idx = map_list->GetSelection();
    if ( layer_idx < 0) {
        return;
    }
    wxString layer_name = map_list->GetString(layer_idx);
    UpdateFieldList(layer_name);
}

void RoadDistancesDlg::OnOK(wxCommandEvent& e)
{
    int layer_idx = map_list->GetSelection();
    if ( layer_idx < 0) {
        return;
    }
    wxString layer_name = map_list->GetString(layer_idx);
    BackgroundMapLayer* ml = NULL;
    ml = project->GetMapLayer(layer_name);
    
    if (ml) {
        int n = ml->GetNumRecords();
        if (project->GetShapeType() != Shapefile::POLY_LINE &&
            ml->GetShapeType() == Shapefile::POINT_TYP) {
            wxMessageDialog dlg (this, _("Input data and select layer are not "
                                         "valid."),
                                 _("Warning"), wxOK | wxICON_INFORMATION);
            dlg.ShowModal();
            return;
        }
        wxString txt_gpu_ratio  = tc_gpu_ratio->GetValue();
        double gpu_ratio;
        txt_gpu_ratio.ToDouble(&gpu_ratio);

        wxString filter = "Binary file (*.bin)|*.bin";

        wxFileDialog save_dlg(this, "Save Distance Matrix to a file", "", "",
                              filter, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

        if (save_dlg.ShowModal() != wxID_OK) return;

        wxStopWatch sw_vd;

        wxFileName fname = wxFileName(save_dlg.GetPath());
        wxString out_fname = fname.GetPathWithSep() + fname.GetName();

        vector<OGRFeature*> roads = project->layer_proxy->data;
        vector<OGRFeature*> query_points = ml->layer_proxy->data;

        double default_speed = GetDefaultSpeed();
        double penalty = GetSpeedPenalty();
        std::map<wxString, double> speed_limits = GetSpeedLimitDict();
        wxString highway_type_field = GetHighwayTypeField();
        wxString max_speed_field = GetMaxSpeedField();
        wxString one_way_field = GetOneWayField();

        OSMTools::TravelDistanceMatrix travel(roads, default_speed, penalty,
                                              speed_limits, highway_type_field,
                                              max_speed_field, one_way_field);
        if (cb_use_gpu->IsChecked()) travel.SetGPURatio(gpu_ratio);
        travel.GetDistanceMatrix(query_points, out_fname);

        wxString msg = wxString::Format(_("Distance Matrix Saved Successfully in %ld ms."),
                                        sw_vd.Time());
        wxMessageDialog dlg (this, msg, _("Information"),
                             wxOK | wxICON_INFORMATION);
        dlg.ShowModal();

        EndDialog(wxID_OK);
    }
}

double RoadDistancesDlg::GetDefaultSpeed()
{
    double d_val;
    wxString tmp = tc_default_speed->GetValue();
    tmp.ToDouble(&d_val); // already checked in OnOk
    return d_val;
}

double RoadDistancesDlg::GetSpeedPenalty()
{
    double d_val;
    wxString tmp = tc_speed_penalty->GetValue();
    tmp.ToDouble(&d_val); // already checked in OnOk
    return d_val;
}

std::map<wxString, double> RoadDistancesDlg::GetSpeedLimitDict()
{
    std::map<wxString, double> speed_limit_dict;
    int n = 35;
    for (size_t i=0; i<n; ++i) {
        wxString way_type = gd_speed->GetCellValue(i, 0);
        wxString speed_val = gd_speed->GetCellValue(i, 1);
        double val = 20.0; // default 20 km/hr
        if (speed_val.ToDouble(&val)) {
            speed_limit_dict[way_type] = val;
        }
    }
    return speed_limit_dict;
}

wxString RoadDistancesDlg::GetHighwayTypeField()
{
    if (cb_field_highway->IsChecked() == false) return wxEmptyString;
    return co_field_highway->GetStringSelection();
}

wxString RoadDistancesDlg::GetMaxSpeedField()
{
    if (cb_field_speed->IsChecked() == false) return wxEmptyString;
    return co_field_speed->GetStringSelection();
}

wxString RoadDistancesDlg::GetOneWayField()
{
    if (cb_field_oneway->IsChecked() == false) return wxEmptyString;
    return co_field_oneway->GetStringSelection();
}

void RoadDistancesDlg::InitGrid()
{
    wxString way_types[36] = {
        "road",
        "motorway",
        "motorway_link",
        "motorway_junction",
        "trunk",
        "trunk_link",
        "primary",
        "primary_link",
        "secondary",
        "secondary_link",
        "tertiary",
        "tertiary_link",
        "residential",
        "living_street",
        "service",
        "track",
        "pedestrian",
        "services",
        "bus_guideway",
        "path",
        "cycleway",
        "footway",
        "bridleway",
        "byway",
        "steps",
        "unclassified",
        "lane",
        "track",
        "opposite_lane",
        "opposite",
        "grade1",
        "grade2",
        "grade3",
        "grade4",
        "grade5",
        "roundabout"
    };
    double max_speeds[36] = {15, 50, 30, 30, 35, 25, 25, 20, 20, 20, 20, 15,
        12, 10, 7, 7, 2, 2, 2, 5, 10, 2, 2, 2, 0.1, 15, 10, 20, 10, 10, 10,
        10, 10, 10, 10, 25
    };
    int n = 36;
    gd_speed->SetColumnWidth(0, 150);
    gd_speed->SetColumnWidth(1, 150);
    for (size_t i=0; i<n; ++i) {
        gd_speed->SetCellValue(i, 0, way_types[i]);
        gd_speed->SetReadOnly(i, 0 , true);
        wxString tmp;
        tmp << max_speeds[i];
        gd_speed->SetCellValue(i, 1, tmp);
    }
}
