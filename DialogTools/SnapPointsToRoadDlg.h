//
//  SnapPointsToRoadDlg.hpp
//  GeoDa
//
//  Created by Xun Li on 1/10/19.
//

#ifndef SnapPointsToRoadDlg_hpp
#define SnapPointsToRoadDlg_hpp

#include <stdio.h>
#include <wx/wx.h>

class Project;

class SnapPointsToRoadDlg : public wxDialog
{
    Project* project;
    wxChoice* map_list;
    wxChoice* field_list;
    wxStaticText* field_st;
    wxBoxSizer* vbox;
    wxBoxSizer* cbox;
    wxPanel* panel;

    void UpdateFieldList(wxString name);

public:
    SnapPointsToRoadDlg(wxWindow* parent, Project* project);

    void OnOK(wxCommandEvent& e);
    void OnLayerSelect(wxCommandEvent& e);

    void InitMapList();
};

#endif /* SnapPointsToRoadDlg_hpp */
