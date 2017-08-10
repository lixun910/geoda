/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <map>
#include <algorithm>
#include <limits>

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/choice.h>

#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../Explore/MapNewView.h"
#include "../Project.h"
#include "../Algorithms/cluster.h"
#include "../Algorithms/maxp.h"

#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "SaveToTableDlg.h"
#include "MaxpDlg.h"


BEGIN_EVENT_TABLE( MaxpDlg, wxDialog )
EVT_CLOSE( MaxpDlg::OnClose )
END_EVENT_TABLE()

MaxpDlg::MaxpDlg(wxFrame* parent_s, Project* project_s)
: frames_manager(project_s->GetFramesManager()),
wxDialog(NULL, -1, _("Max-p Settings"), wxDefaultPosition, wxDefaultSize,
         wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxLogMessage("Open Max-p dialog.");
    
	SetMinSize(wxSize(360,750));

    parent = parent_s;
    project = project_s;
    
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    } else {
        CreateControls();
    }
    frames_manager->registerObserver(this);
}

MaxpDlg::~MaxpDlg()
{
    wxLogMessage("On MaxpDlg::~MaxpDlg");
    frames_manager->removeObserver(this);
}

bool MaxpDlg::Init()
{
    wxLogMessage("On MaxpDlg::Init");
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void MaxpDlg::update(FramesManager* o)
{
    
}

void MaxpDlg::CreateControls()
{
    wxLogMessage("On MaxpDlg::CreateControls");
    wxPanel *panel = new wxPanel(this);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    wxStaticText* st = new wxStaticText (panel, wxID_ANY, _("Select Variables (for intra-regional homogeneity)"),
                                         wxDefaultPosition, wxDefaultSize);
    
    wxListBox* box = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(250,250), 0, NULL,
                                   wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    wxCheckBox* cbox = new wxCheckBox(panel, wxID_ANY, _("Use Geometric Centroids"));
    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxVERTICAL, panel, "Input:");
    hbox0->Add(st, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    hbox0->Add(box, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    hbox0->Add(cbox, 0, wxLEFT | wxRIGHT, 10);
    
    if (project->IsTableOnlyProject()) {
        cbox->Disable();
    }
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(9,2,5,0);

    wxStaticText* st16 = new wxStaticText(panel, wxID_ANY, _("Weights:"),
                                          wxDefaultPosition, wxSize(128,-1));
    combo_weights = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                wxSize(200,-1));
    
    gbox->Add(st16, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_weights, 1, wxEXPAND);
    
    
    wxStaticText* st10 = new wxStaticText(panel, wxID_ANY, _("Minimum bound:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxTextCtrl  *box10 = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st10, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box10, 1, wxEXPAND);
    
    wxStaticText* st19 = new wxStaticText(panel, wxID_ANY, _("Set floor variable:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxBoxSizer *hbox19 = new wxBoxSizer(wxHORIZONTAL);
    chk_floor = new wxCheckBox(panel, wxID_ANY, "");
    combo_floor = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(160,-1));
    
    hbox19->Add(chk_floor,0, wxALIGN_CENTER_VERTICAL);
    hbox19->Add(combo_floor,0,wxALIGN_CENTER_VERTICAL);
    combo_floor->Disable();
    gbox->Add(st19, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox19, 1, wxEXPAND);


    wxStaticText* st18 = new wxStaticText(panel, wxID_ANY, _("Set initial groups:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxBoxSizer *hbox18 = new wxBoxSizer(wxHORIZONTAL);
    chk_lisa = new wxCheckBox(panel, wxID_ANY, "");
    combo_lisa = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(160,-1));
    
    hbox18->Add(chk_lisa,0, wxALIGN_CENTER_VERTICAL);
    hbox18->Add(combo_lisa,0,wxALIGN_CENTER_VERTICAL);
    combo_lisa->Disable();
    gbox->Add(st18, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox18, 1, wxEXPAND);
    
    wxStaticText* st11 = new wxStaticText(panel, wxID_ANY, _("Initialization #:"),
                                          wxDefaultPosition, wxSize(128,-1));
    m_iterations = new wxTextCtrl(panel, wxID_ANY, wxT("99"), wxDefaultPosition, wxSize(200,-1));
    gbox->Add(st11, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_iterations, 1, wxEXPAND);
    
    

    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Transformation:"),
                                          wxDefaultPosition, wxSize(120,-1));
    const wxString _transform[3] = {"Raw", "Demean", "Standardize"};
    wxChoice* box01 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(120,-1), 3, _transform);
    box01->SetSelection(0);
    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(box01, 1, wxEXPAND);
    
    wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Use specified seed:"),
                                          wxDefaultPosition, wxSize(128,-1));
    wxBoxSizer *hbox17 = new wxBoxSizer(wxHORIZONTAL);
    chk_seed = new wxCheckBox(panel, wxID_ANY, "");
    seedButton = new wxButton(panel, wxID_OK, wxT("Change Seed"));
    
    hbox17->Add(chk_seed,0, wxALIGN_CENTER_VERTICAL);
    hbox17->Add(seedButton,0,wxALIGN_CENTER_VERTICAL);
    seedButton->Disable();
    gbox->Add(st17, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox17, 1, wxEXPAND);
    
    if (GdaConst::use_gda_user_seed) {
        setrandomstate(GdaConst::gda_user_seed);
        chk_seed->SetValue(true);
        seedButton->Enable();
    }
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    
    // Output
    wxStaticText* st3 = new wxStaticText (panel, wxID_ANY, _("Save Cluster in Field:"),
                                         wxDefaultPosition, wxDefaultSize);
    wxTextCtrl  *box3 = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(158,-1));
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Output:");
    //wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(box3, 1, wxALIGN_CENTER_VERTICAL);
    
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    //wxButton *saveButton = new wxButton(panel, wxID_SAVE, wxT("Save"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    //hbox2->Add(saveButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox0, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 0, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    
    panel->SetSizer(container);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);

    
    Centre();

    // Content
    InitVariableCombobox(box);
    m_textbox = box3;
    combo_var = box;
    m_use_centroids = cbox;
  
    m_pass = box10;
    combo_tranform = box01;
    
    // init weights
    vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    
    size_t sel_pos=0;
    for (size_t i=0; i<weights_ids.size(); ++i) {
        combo_weights->Append(w_man_int->GetShortDispName(weights_ids[i]));
        if (w_man_int->GetDefault() == weights_ids[i])
            sel_pos = i;
    }
    if (weights_ids.size() > 0) combo_weights->SetSelection(sel_pos);
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &MaxpDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &MaxpDlg::OnClickClose, this);
    chk_seed->Bind(wxEVT_CHECKBOX, &MaxpDlg::OnSeedCheck, this);
    seedButton->Bind(wxEVT_BUTTON, &MaxpDlg::OnChangeSeed, this);
    chk_lisa->Bind(wxEVT_CHECKBOX, &MaxpDlg::OnLISACheck, this);
    chk_floor->Bind(wxEVT_CHECKBOX, &MaxpDlg::OnFloorCheck, this);

}

void MaxpDlg::OnFloorCheck(wxCommandEvent& event)
{
    wxLogMessage("On MaxpDlg::OnLISACheck");
    bool use_floor = chk_floor->GetValue();
    
    if (use_floor) {
        combo_floor->Enable();
    } else {
        combo_floor->Disable();
    }
}

void MaxpDlg::OnLISACheck(wxCommandEvent& event)
{
    wxLogMessage("On MaxpDlg::OnLISACheck");
    bool use_lisa_seed = chk_lisa->GetValue();
    
    if (use_lisa_seed) {
        combo_lisa->Enable();
    } else {
        combo_lisa->Disable();
    }
}

void MaxpDlg::OnSeedCheck(wxCommandEvent& event)
{
    wxLogMessage("On MaxpDlg::OnSeedCheck");
    bool use_user_seed = chk_seed->GetValue();
    
    if (use_user_seed) {
        seedButton->Enable();
        if (GdaConst::use_gda_user_seed == false && GdaConst::gda_user_seed == 0) {
            OnChangeSeed(event);
            return;
        }
        GdaConst::use_gda_user_seed = true;
        setrandomstate(GdaConst::gda_user_seed);
        
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        ogr_adapt.AddEntry("use_gda_user_seed", "1");
    } else {
        seedButton->Disable();
    }
}

void MaxpDlg::OnChangeSeed(wxCommandEvent& event)
{
    wxLogMessage("On MaxpDlg::OnChangeSeed");
    // prompt user to enter user seed (used globally)
    wxString m;
    m << "Enter a seed value for random number generator:";
    
    long long unsigned int val;
    wxString dlg_val;
    wxString cur_val;
    cur_val << GdaConst::gda_user_seed;
    
    wxTextEntryDialog dlg(NULL, m, "Enter a seed value", cur_val);
    if (dlg.ShowModal() != wxID_OK) return;
    dlg_val = dlg.GetValue();
    dlg_val.Trim(true);
    dlg_val.Trim(false);
    if (dlg_val.IsEmpty()) return;
    if (dlg_val.ToULongLong(&val)) {
        uint64_t new_seed_val = val;
        GdaConst::gda_user_seed = new_seed_val;
        GdaConst::use_gda_user_seed = true;
        setrandomstate(GdaConst::gda_user_seed);
        
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        wxString str_gda_user_seed;
        str_gda_user_seed << GdaConst::gda_user_seed;
        ogr_adapt.AddEntry("gda_user_seed", str_gda_user_seed.ToStdString());
        ogr_adapt.AddEntry("use_gda_user_seed", "1");
    } else {
        wxString m;
        m << "\"" << dlg_val << "\" is not a valid seed. Seed unchanged.";
        wxMessageDialog dlg(NULL, m, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        GdaConst::use_gda_user_seed = false;
        OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
        ogr_adapt.AddEntry("use_gda_user_seed", "0");
    }
}

void MaxpDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("On MaxpDlg::InitVariableCombobox");
    wxArrayString items;
    
    std::vector<int> col_id_map;
    table_int->FillNumericColIdMap(col_id_map);
    for (int i=0, iend=col_id_map.size(); i<iend; i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        if (table_int->IsColTimeVariant(id)) {
            for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
                wxString nm = name;
                nm << " (" << table_int->GetTimeString(t) << ")";
                name_to_nm[nm] = name;
                name_to_tm_id[nm] = t;
                items.Add(nm);
            }
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            items.Add(name);
        }
    }
    
    var_box->InsertItems(items,0);
    
    for (int i=0; i<items.size(); i++) {
        combo_lisa->Insert(items[i],i);
        combo_floor->Insert(items[i],i);
    }
}

void MaxpDlg::OnClickClose(wxCommandEvent& event )
{
    wxLogMessage("OnClickClose MaxpDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void MaxpDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close MaxpDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void MaxpDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click MaxpDlg::OnOK");
    
    bool use_centroids = m_use_centroids->GetValue();
    
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    
    int num_var = selections.size();
    if (num_var < 1 && !use_centroids) {
        // show message box
        wxString err_msg = _("Please select at least 1 variables.");
        wxMessageDialog dlg(NULL, err_msg, "Info", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    wxString str_floor = m_pass->GetValue();
    if (str_floor.IsEmpty()) {
        wxString err_msg = _("Please enter minimum bound value");
        wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    wxString str_initial = m_iterations->GetValue();
    if (str_initial.IsEmpty()) {
        wxString err_msg = _("Please enter initialization number");
        wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    wxString field_name = m_textbox->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving clustering results.");
        wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    col_ids.resize(num_var);
    var_info.resize(num_var);
    
    for (int i=0; i<num_var; i++) {
        int idx = selections[i];
        wxString nm = name_to_nm[combo_var->GetString(idx)];
        
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        
        int tm = name_to_tm_id[combo_var->GetString(idx)];
        
        col_ids[i] = col;
        var_info[i].time = tm;
        
        // Set Primary GdaVarTools::VarInfo attributes
        var_info[i].name = nm;
        var_info[i].is_time_variant = table_int->IsColTimeVariant(idx);
        
        // var_info[i].time already set above
        table_int->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
        var_info[i].sync_with_global_time = var_info[i].is_time_variant;
        var_info[i].fixed_scale = true;
    }
    
    // Call function to set all Secondary Attributes based on Primary Attributes
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
    
    int rows = project->GetNumRecords();
    int columns =  0;
    
    std::vector<d_array_type> data; // data[variable][time][obs]
    data.resize(col_ids.size());
    for (int i=0; i<var_info.size(); i++) {
        table_int->GetColData(col_ids[i], data[i]);
    }
    // get columns (if time variables show)
    for (int i=0; i<data.size(); i++ ){
        for (int j=0; j<data[i].size(); j++) {
            columns += 1;
        }
    }
    
    // if use centroids
    if (use_centroids) {
        columns += 2;
    }
    
    vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);

    int sel = combo_weights->GetSelection();
    if (sel < 0) sel = 0;
    if (sel >= weights_ids.size()) sel = weights_ids.size()-1;
    
    boost::uuids::uuid w_id = weights_ids[sel];
    GalWeight* gw = w_man_int->GetGal(w_id);

    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }

    int transform = combo_tranform->GetSelection();
    
    vector<vector<double> > z;
    // assign value
    int col_ii = 0;
    for (int i=0; i<data.size(); i++ ){ // col
        for (int j=0; j<data[i].size(); j++) { // time
            std::vector<double> vals;
            for (int k=0; k< rows;k++) { // row
                vals.push_back(data[i][j][k]);
            }
            if (transform == 2) {
                GenUtils::StandardizeData(vals);
            } else if (transform == 1 ) {
                GenUtils::DeviationFromMean(vals);
            }
            z.push_back(vals);
        }
    }
    vector<vector<double> > z_t;
    for (int i=0; i<rows; i++) {
        vector<double> vals;
        for (int j=0; j<z.size(); j++) { // cols
            vals.push_back(z[j][i]);
        }
        z_t.push_back(vals);
    }
    if (use_centroids) {
        std::vector<GdaPoint*> cents = project->GetCentroids();
        std::vector<double> cent_xs;
        std::vector<double> cent_ys;
        
        for (int i=0; i< rows; i++) {
            cent_xs.push_back(cents[i]->GetX());
            cent_ys.push_back(cents[i]->GetY());
        }
        
        if (transform == 2) {
            GenUtils::StandardizeData(cent_xs );
            GenUtils::StandardizeData(cent_ys );
        } else if (transform == 1 ) {
            GenUtils::DeviationFromMean(cent_xs );
            GenUtils::DeviationFromMean(cent_ys );
        }
        
        for (int i=0; i< rows; i++) {
            z_t[i].push_back(cent_xs[i]);
            z_t[i].push_back(cent_ys[i]);
        }
    }
    
    int floor = 1;
    long value_floor;
    if(str_floor.ToLong(&value_floor)) {
        floor = value_floor;
    }
    
    vector<double> floor_variable(rows, 1);
    bool use_floor = chk_floor->GetValue();
    if (use_floor) {
        int idx = combo_floor->GetSelection();
        wxString nm = name_to_nm[combo_lisa->GetString(idx)];
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        int tm = name_to_tm_id[combo_floor->GetString(idx)];
        
        table_int->GetColData(col, tm, floor_variable);
    }
    
    int initial = 99;
    long value_initial;
    if(str_initial.ToLong(&value_initial)) {
        initial = value_initial;
    }
    
    vector<size_t> seeds;
    bool use_lisa_seed = chk_lisa->GetValue();
    if (use_lisa_seed) {
        int idx = combo_lisa->GetSelection();
        wxString nm = name_to_nm[combo_lisa->GetString(idx)];
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        int tm = name_to_tm_id[combo_lisa->GetString(idx)];
        
        std::vector<wxInt64> _data;
        table_int->GetColData(col, tm, _data);
        GenUtils::sort(_data, _data, seeds);
    }
    
    int rnd_seed = -1;
    if (chk_seed->GetValue()) rnd_seed = GdaConst::gda_user_seed;
    
    Maxp maxp(gw->gal, z_t, floor, floor_variable, initial, seeds, rnd_seed);
    
    vector<vector<int> > cluster_ids = maxp.GetRegions();
    int ncluster = cluster_ids.size();
    
    vector<wxInt64> clusters(rows, 0);
    vector<bool> clusters_undef(rows, false);

    // sort result
    std::sort(cluster_ids.begin(), cluster_ids.end(), GenUtils::less_vectors);
    
    for (int i=0; i < ncluster; i++) {
        int c = i + 1;
        for (int j=0; j<cluster_ids[i].size(); j++) {
            int idx = cluster_ids[i][j];
            clusters[idx] = c;
        }
    }
    
    // save to table
    int time=0;
    int col = table_int->FindColId(field_name);
    if ( col == wxNOT_FOUND) {
        int col_insert_pos = table_int->GetNumberCols();
        int time_steps = 1;
        int m_length_val = GdaConst::default_dbf_long_len;
        int m_decimals_val = 0;
        
        col = table_int->InsertCol(GdaConst::long64_type, field_name, col_insert_pos, time_steps, m_length_val, m_decimals_val);
    } else {
        // detect if column is integer field, if not raise a warning
        if (table_int->GetColType(col) != GdaConst::long64_type ) {
            wxString msg = _("This field name already exists (non-integer type). Please input a unique name.");
            wxMessageDialog dlg(this, msg, "Warning", wxOK | wxICON_WARNING );
            dlg.ShowModal();
            return;
        }
    }
    
    if (col > 0) {
        table_int->SetColData(col, time, clusters);
        table_int->SetColUndefined(col, time, clusters_undef);
    }
    
    // show a cluster map
    if (project->IsTableOnlyProject()) {
        return;
    }
    std::vector<GdaVarTools::VarInfo> new_var_info;
    std::vector<int> new_col_ids;
    new_col_ids.resize(1);
    new_var_info.resize(1);
    new_col_ids[0] = col;
    new_var_info[0].time = 0;
    // Set Primary GdaVarTools::VarInfo attributes
    new_var_info[0].name = field_name;
    new_var_info[0].is_time_variant = table_int->IsColTimeVariant(col);
    table_int->GetMinMaxVals(new_col_ids[0], new_var_info[0].min, new_var_info[0].max);
    new_var_info[0].sync_with_global_time = new_var_info[0].is_time_variant;
    new_var_info[0].fixed_scale = true;

    
    MapFrame* nf = new MapFrame(parent, project,
                                new_var_info, new_col_ids,
                                CatClassification::unique_values,
                                MapCanvas::no_smoothing, 4,
                                boost::uuids::nil_uuid(),
                                wxDefaultPosition,
                                GdaConst::map_default_size);
}