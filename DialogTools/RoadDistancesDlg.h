//
//  RoadDistancesDlg
//  GeoDa
//
//  Created by Xun Li on 9/4/18.
//

#ifndef __GEODA_ROADDISTANCESDLG__
#define __GEODA_ROADDISTANCESDLG__

#include <vector>
#include <map>
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include "../SpatialIndTypes.h"

class Project;

class RoadDistancesDlg : public wxDialog
{
public:
    RoadDistancesDlg(wxWindow* parent, Project* project,
                   const wxString& title = _("Road Distances Dialog"));
    
    void OnOK(wxCommandEvent& e);

    void OnLayerSelect(wxCommandEvent& e);
    
    void InitMapList();

    void UpdateFieldList(wxString name);

    void InitGrid();

    void InitDataSetup();

    std::map<wxString, double> GetSpeedLimitDict();

    double GetRadius();

    double GetDefaultSpeed();

    double GetSpeedPenalty();
    
protected:
    Project* project;
    std::vector<wxString> point_layer_names;

    wxChoice* map_list;
    wxChoice* field_list;
    wxStaticText* field_st;
    wxBoxSizer* vbox;
    wxBoxSizer* cbox;
    wxPanel* panel;

    wxCheckBox* cb_field_highway;
    wxChoice* co_field_highway;
    wxCheckBox* cb_field_speed;
    wxChoice* co_field_speed;
    wxCheckBox* cb_field_oneway;
    wxChoice* co_field_oneway;
    wxGrid* gd_speed;
    wxTextCtrl* tc_default_speed;
    wxTextCtrl* tc_speed_penalty;
    wxCheckBox* cb_use_gpu;
    wxTextCtrl* tc_gpu_ratio;
};

#endif
