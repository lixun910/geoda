//
//  SpatialJoinDlg.cpp
//  GeoDa
//
//  Created by Xun Li on 9/4/18.
//
#include <vector>
#include <wx/wx.h>

#include "../ShpFile.h"
#include "../Project.h"
#include "../Explore/MapLayer.hpp"

#include "RoadDistancesDlg.h"

using namespace std;


RoadDistancesDlg::RoadDistancesDlg(wxWindow* parent,
                                   Project* _project,
                                   const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(350, 250)),
    project(_project)
{
    panel = new wxPanel(this, -1);
    
    wxString info = _("Please select a layer to compute travel distance "
                      "matrix on current map of road networks:");
    wxStaticText* st = new wxStaticText(panel, wxID_ANY, info);
    
    map_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(160,-1));
    field_st = new wxStaticText(panel, wxID_ANY,
                                _("Select ID Variable (Optional)"));
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
    
    map_list->Bind(wxEVT_CHOICE, &RoadDistancesDlg::OnLayerSelect, this);
    ok_btn->Bind(wxEVT_BUTTON, &RoadDistancesDlg::OnOK, this);
    
    InitMapList();
    field_st->Disable();
    field_list->Disable();
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

        vector<OGRFeature*> roads = project->layer_proxy->data;
        vector<OGRFeature*> query_points = ml->layer_proxy->data;
        EndDialog(wxID_OK);
    }
}

