//
//  SnapPointsToRoadDlg.cpp
//  GeoDa
//
//  Created by Xun Li on 1/10/19.
//
#include <vector>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "../Project.h"
#include "../MapLayerStateObserver.h"
#include "../Explore/MapLayerTree.hpp"
#include "../osm/RoadUtils.h"
#include "SaveToTableDlg.h"
#include "SnapPointsToRoadDlg.h"

SnapPointsToRoadDlg::SnapPointsToRoadDlg(wxWindow* parent, Project* _project)
: wxDialog(parent, wxID_ANY, "Snap Points to Road", wxDefaultPosition, wxSize(350, 250))
{
    project = _project;
    panel = new wxPanel(this, -1);

    wxString info = _("Please select a points layer to snap to current road map (%s):");
    info = wxString::Format(info, project->GetProjectTitle());
    wxStaticText* st = new wxStaticText(panel, wxID_ANY, info);

    map_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(160,-1));
    field_st = new wxStaticText(panel, wxID_ANY, "Select ID Variable (Optional)");
    field_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(100,-1));
    wxBoxSizer* mbox = new wxBoxSizer(wxHORIZONTAL);
    mbox->Add(map_list, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(field_st, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(field_list, 0, wxALIGN_CENTER | wxALL, 5);

    cbox = new wxBoxSizer(wxVERTICAL);
    cbox->Add(st, 0, wxALIGN_CENTER | wxALL, 15);
    cbox->Add(mbox, 0, wxALIGN_CENTER | wxALL, 10);
    panel->SetSizerAndFit(cbox);

    wxButton* ok_btn = new wxButton(this, wxID_ANY, _("OK"), wxDefaultPosition,
                                    wxDefaultSize, wxBU_EXACTFIT);
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, _("Close"),
                                        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(ok_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(cancel_btn, 0, wxALIGN_CENTER | wxALL, 5);

    vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(panel, 1, wxALL, 15);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);

    SetSizer(vbox);
    vbox->Fit(this);

    Center();

    map_list->Bind(wxEVT_CHOICE, &SnapPointsToRoadDlg::OnLayerSelect, this);
    ok_btn->Bind(wxEVT_BUTTON, &SnapPointsToRoadDlg::OnOK, this);

    InitMapList();
    field_st->Disable();
    field_list->Disable();
}

void SnapPointsToRoadDlg::InitMapList()
{
    map_list->Clear();
    map<wxString, BackgroundMapLayer*>::iterator it;

    for (it=project->bg_maps.begin(); it!=project->bg_maps.end(); it++) {
        wxString name = it->first;
        map_list->Append(name);
    }
    for (it=project->fg_maps.begin(); it!=project->fg_maps.end(); it++) {
        wxString name = it->first;
        map_list->Append(name);
    }
    if (map_list->GetCount() > 0) {
        // check the first item in list
        wxString name = map_list->GetString(0);
        UpdateFieldList(name);
    }
}

void SnapPointsToRoadDlg::UpdateFieldList(wxString name)
{
    BackgroundMapLayer* ml = project->GetMapLayer(name);
    if (ml) {
        if (Shapefile::POLYGON == ml->GetShapeType() &&
            project->IsPointTypeData()) {
            // assign polygon to point
            field_list->Clear();
            vector<wxString> field_names = ml->GetIntegerFieldNames();
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

void SnapPointsToRoadDlg::OnLayerSelect(wxCommandEvent& e)
{
    int layer_idx = map_list->GetSelection();
    if ( layer_idx < 0) {
        return;
    }
    wxString layer_name = map_list->GetString(layer_idx);
    UpdateFieldList(layer_name);
}

void SnapPointsToRoadDlg::OnOK(wxCommandEvent& e)
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
        if (ml->GetShapeType() != Shapefile::POINT_TYP) {
            wxMessageDialog dlg (this, _("Snapping points can not be applied on "
                                         "a points layers. Please select "
                                         "another layer."),
                                 _("Warning"), wxOK | wxICON_INFORMATION);
            dlg.ShowModal();
            return;
        }

        std::vector<OGRFeature*> roads = project->layer_proxy->data;
        std::vector<OGRFeature*> points = ml->layer_proxy->data;

        RoadUtils road_utils(roads);
        std::vector<int> results = road_utils.SnapPointsOnRoad(points);
        vector<wxInt64> spatial_counts;
        for (size_t i=0; i<results.size(); ++i) {
            spatial_counts.push_back(results[i]);
        }
        wxString label = "Snap Count";
        wxString field_name = "SC";
        wxString dlg_title = _("Save Results to Table: ") + label;
        // save results
        int new_col = 1;
        std::vector<SaveToTableEntry> new_data(new_col);
        vector<bool> undefs(project->GetNumRecords(), false);
        new_data[0].l_val = &spatial_counts;
        new_data[0].label = label;
        new_data[0].field_default = field_name;
        new_data[0].type = GdaConst::long64_type;
        new_data[0].undefined = &undefs;
        SaveToTableDlg dlg(project, this, new_data, dlg_title,
                           wxDefaultPosition, wxSize(400,400));
        dlg.ShowModal();

        EndDialog(wxID_OK);
    }
}
