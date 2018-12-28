//
//  SpatialJoinDlg.hpp
//  GeoDa
//
//  Created by Xun Li on 9/4/18.
//

#ifndef __GEODA_ROADDISTANCESDLG__
#define __GEODA_ROADDISTANCESDLG__

#include <vector>
#include <wx/dialog.h>
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

protected:
    Project* project;
    std::vector<wxString> point_layer_names;

    wxChoice* map_list;
    wxChoice* field_list;
    wxStaticText* field_st;
    wxBoxSizer* vbox;
    wxBoxSizer* cbox;
    wxPanel* panel;
};

#endif
