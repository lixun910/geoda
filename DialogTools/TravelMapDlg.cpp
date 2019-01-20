//
//  TravelMapConfigureDlg
//  GeoDa
//
//  Created by Xun Li on 9/4/18.
//
#include <vector>
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/grid.h>

#include "../ShpFile.h"
#include "../Project.h"
#include "../Explore/MapLayer.hpp"
#include "../osm/TravelTool.h"

#include "TravelMapDlg.h"

using namespace std;


TravelMapConfigureDlg::TravelMapConfigureDlg(wxWindow* parent,
                                             Project* _project,
                                             const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(450, 250),
               wxDEFAULT_DIALOG_STYLE),
    project(_project)
{
    wxFlexGridSizer* grid_sizer1 = new wxFlexGridSizer(2, 2, 8, 10);
    grid_sizer1->Add(new wxStaticText(this, -1, _("Select map type:")));
    co_map_type = new wxChoice(this, -1);
    co_map_type->Append("Hexagon Map");
    co_map_type->SetSelection(0);
    grid_sizer1->Add(co_map_type, 0, wxALIGN_LEFT);
    grid_sizer1->Add(new wxStaticText(this, -1,
                                      _("The radius of selected type (meter):")));
    tc_radius = new wxTextCtrl(this, -1, "160");
    grid_sizer1->Add(tc_radius, 0, wxALIGN_LEFT);
    grid_sizer1->AddGrowableCol(0, 1);

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

    // buttons
    wxButton* ok_btn = new wxButton(this, wxID_ANY, _("OK"), wxDefaultPosition,
                                    wxDefaultSize, wxBU_EXACTFIT);
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, _("Close"),
                            wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(ok_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(cancel_btn, 0, wxALIGN_CENTER | wxALL, 5);
    
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(grid_sizer1, 0,  wxALIGN_CENTER | wxALL, 20);
    vbox->Add(nb, 1, wxEXPAND | wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizer(vbox);
    vbox->Fit(this);
    
    Center();
    
    ok_btn->Bind(wxEVT_BUTTON, &TravelMapConfigureDlg::OnOK, this);

    InitDataSetup();
}

void TravelMapConfigureDlg::InitDataSetup()
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

void TravelMapConfigureDlg::OnOK(wxCommandEvent& e)
{
    bool error = false;
    wxString error_msg;

    double d_val;
    // check radius input
    wxString tmp = tc_radius->GetValue();
    if (tmp.ToDouble(&d_val) == false) {
        error_msg = _("The input of radius is not valid.");
        error = true;
    }
    tmp = tc_default_speed->GetValue();
    if (tmp.ToDouble(&d_val) == false) {
        error_msg = _("The input of default speed is not valid.");
        error = true;
    }
    tmp = tc_speed_penalty->GetValue();
    if (tmp.ToDouble(&d_val) == false) {
        error_msg = _("The input of speed penalty is not valid.");
        error = true;
    }
    if (error) {
        wxMessageDialog msg_dlg(this, error_msg,
                                _("Error"),
                                wxOK | wxOK_DEFAULT | wxICON_INFORMATION);
        msg_dlg.ShowModal();
        return;
    }

    EndDialog(wxID_OK);
}

double TravelMapConfigureDlg::GetRadius()
{
    double d_val;
    wxString tmp = tc_radius->GetValue();
    tmp.ToDouble(&d_val); // already checked in OnOk
    return d_val;
}

double TravelMapConfigureDlg::GetDefaultSpeed()
{
    double d_val;
    wxString tmp = tc_default_speed->GetValue();
    tmp.ToDouble(&d_val); // already checked in OnOk
    return d_val;
}

double TravelMapConfigureDlg::GetSpeedPenalty()
{
    double d_val;
    wxString tmp = tc_speed_penalty->GetValue();
    tmp.ToDouble(&d_val); // already checked in OnOk
    return d_val;
}

std::map<wxString, double> TravelMapConfigureDlg::GetSpeedLimitDict()
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

wxString TravelMapConfigureDlg::GetHighwayTypeField()
{
    if (cb_field_highway->IsChecked() == false) return wxEmptyString;
    return co_field_highway->GetStringSelection();
}

wxString TravelMapConfigureDlg::GetMaxSpeedField()
{
    if (cb_field_speed->IsChecked() == false) return wxEmptyString;
    return co_field_speed->GetStringSelection();
}

wxString TravelMapConfigureDlg::GetOneWayField()
{
    if (cb_field_oneway->IsChecked() == false) return wxEmptyString;
    return co_field_oneway->GetStringSelection();
}

void TravelMapConfigureDlg::InitGrid()
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
