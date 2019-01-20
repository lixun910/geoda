//
//  TravelMapConfigureDlg
//  GeoDa
//
//  Created by Xun Li on 9/4/18.
//

#ifndef __GEODA_TRAVEL_MAP_CONFIGURE_DLG__
#define __GEODA_TRAVEL_MAP_CONFIGURE_DLG__

#include <vector>
#include <map>
#include <boost/unordered_map.hpp>
#include <wx/dialog.h>
#include <wx/choice.h>
#include "../SpatialIndTypes.h"

class Project;

class TravelMapConfigureDlg : public wxDialog
{
public:
    TravelMapConfigureDlg(wxWindow* parent, Project* project,
                   const wxString& title = _("Travel Map Configure Dialog"));
    
    void OnOK(wxCommandEvent& e);

    void InitGrid();

    void InitDataSetup();
    
    std::map<wxString, double> GetSpeedLimitDict();

    double GetRadius();

    double GetDefaultSpeed();

    double GetSpeedPenalty();

    wxString GetHighwayTypeField();

    wxString GetMaxSpeedField();

    wxString GetOneWayField();
    
protected:
    Project* project;

    wxChoice* co_map_type;
    wxTextCtrl* tc_radius;

    wxCheckBox* cb_field_highway;
    wxChoice* co_field_highway;
    wxCheckBox* cb_field_speed;
    wxChoice* co_field_speed;
    wxCheckBox* cb_field_oneway;
    wxChoice* co_field_oneway;
    wxGrid* gd_speed;
    wxTextCtrl* tc_default_speed;
    wxTextCtrl* tc_speed_penalty;
};

#endif
