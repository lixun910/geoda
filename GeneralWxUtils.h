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

#ifndef __GEODA_CENTER_GENERAL_WX_UTILS_H__
#define __GEODA_CENTER_GENERAL_WX_UTILS_H__

#include <wx/menu.h>
#include <wx/string.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/colour.h>
#include <wx/textwrapper.h>
#include <wx/gdicmn.h> // for wxPoint / wxRealPoint

#include "DialogTools/VariableSettingsDlg.h"

namespace GenUtils {
    wxString WrapText(wxWindow *win, const wxString& text, int widthMax);
    double distance(const wxRealPoint& p1, const wxRealPoint& p2);
    double distance(const wxRealPoint& p1, const wxPoint& p2);
    double distance(const wxPoint& p1, const wxRealPoint& p2);
    double distance(const wxPoint& p1, const wxPoint& p2);
    double distance_sqrd(const wxRealPoint& p1, const wxRealPoint& p2);
    double distance_sqrd(const wxRealPoint& p1, const wxPoint& p2);
    double distance_sqrd(const wxPoint& p1, const wxRealPoint& p2);
    double distance_sqrd(const wxPoint& p1, const wxPoint& p2);
    double pointToLineDist(const wxPoint& p0, const wxPoint& p1,
                           const wxPoint& p2);
    wxString PtToStr(const wxPoint& p);
    wxString PtToStr(const wxRealPoint& p);
    
    wxString DetectDateFormat(wxString s, vector<wxString>& date_items);
}

namespace GdaColorUtils {
    /** Returns colour in 6-hex-digit HTML format.
     Eg wxColour(255,0,0) -> "#FF0000" */
    wxString ToHexColorStr(const wxColour& c);
    /** change brightness of input_color and leave result in output color
     brightness = 75 by default, will slightly darken the input color.
     brightness = 0 is black, brightness = 200 is white. */
    wxColour ChangeBrightness(const wxColour& input_col, int brightness = 75);
    
    void GetUnique20Colors(vector<wxColour>& colors);
    
    void GetLISAColors(vector<wxColour>& colors);
    void GetLISAColorLabels(vector<wxString>& labels);
    
    void GetLocalGColors(vector<wxColour>& colors);
    void GetLocalGColorLabels(vector<wxString>& labels);
    
    void GetLocalJoinCountColors(vector<wxColour>& colors);
    void GetLocalJoinCountColorLabels(vector<wxString>& labels);
    
    void GetLocalGearyColors(vector<wxColour>& colors);
    void GetLocalGearyColorLabels(vector<wxString>& labels);
    
    void GetMultiLocalGearyColors(vector<wxColour>& colors);
    void GetMultiLocalGearyColorLabels(vector<wxString>& labels);
    
    void GetPercentileColors(vector<wxColour>& colors);
    void GetPercentileColorLabels(vector<wxString>& labels);
    
    void GetBoxmapColors(vector<wxColour>& colors);
    void GetBoxmapColorLabels(vector<wxString>& labels);
    
    void GetStddevColors(vector<wxColour>& colors);
    void GetStddevColorLabels(vector<wxString>& labels);
    
    void GetQuantile2Colors(vector<wxColour>& colors);
    void GetQuantile3Colors(vector<wxColour>& colors);
    void GetQuantile4Colors(vector<wxColour>& colors);
    void GetQuantile5Colors(vector<wxColour>& colors);
    void GetQuantile6Colors(vector<wxColour>& colors);
    void GetQuantile7Colors(vector<wxColour>& colors);
    void GetQuantile8Colors(vector<wxColour>& colors);
    void GetQuantile9Colors(vector<wxColour>& colors);
    void GetQuantile10Colors(vector<wxColour>& colors);
}

class GeneralWxUtils	{
public:
	static wxOperatingSystemId GetOsId();
	static wxString LogOsId();
	static bool isMac();
	static bool isMac106();
	static bool isWindows();
	static bool isUnix();
	static bool isXP();
	static bool isVista();
	static bool isX86();
	static bool isX64();
	static bool isDebug();
	static bool isBigEndian();
	static bool isLittleEndian();
	static bool ReplaceMenu(wxMenuBar* mb, const wxString& title,
							wxMenu* newMenu); 
	static bool EnableMenuAll(wxMenuBar* mb, const wxString& title,
							  bool enable);
	static void EnableMenuRecursive(wxMenu* menu, bool enable);
	static bool EnableMenuItem(wxMenuBar* mb, const wxString& menuTitle,
							   int id, bool enable);
	static bool EnableMenuItem(wxMenuBar* m, int id, bool enable);
	static bool EnableMenuItem(wxMenu* m, int id, bool enable);
	static bool CheckMenuItem(wxMenuBar* m, int id, bool check);
	static bool CheckMenuItem(wxMenu* menu, int id, bool check);
	static bool SetMenuItemText(wxMenu* menu, int id, const wxString& text);
	static wxMenu* FindMenu(wxMenuBar* mb, const wxString& menuTitle);
    static wxColour PickColor(wxWindow* parent, wxColour& col);
    static void SaveWindowAsImage(wxWindow* win, wxString title);
    //static std::set<wxString> GetFieldNamesFromTable(TableInterface* table);
};

class SimpleReportTextCtrl : public wxTextCtrl
{
public:
    SimpleReportTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value = "",
                         const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                         long style =  wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxTE_RICH2, const wxValidator& validator = wxDefaultValidator,
                         const wxString& name = wxTextCtrlNameStr)
    : wxTextCtrl(parent, id, value, pos, size, style, validator, name)
    {
        if (GeneralWxUtils::isWindows()) {
            wxFont font(8,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            SetFont(font);
        } else {
            wxFont font(12,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            SetFont(font);
        }
    }
protected:
    void OnContextMenu(wxContextMenuEvent& event);
    void OnSaveClick( wxCommandEvent& event );
    DECLARE_EVENT_TABLE()
};

class ScrolledDetailMsgDialog : public wxDialog
{
public:
    ScrolledDetailMsgDialog(const wxString & title, const wxString & msg, const wxString & details, const wxSize &size = wxSize(540, 280), const wxArtID & art_id =  wxART_WARNING);
   
    SimpleReportTextCtrl *tc;
    
    void OnSaveClick( wxCommandEvent& event );
};

class TransparentSettingDialog: public wxDialog
{
	double transparency;
    wxSlider* slider;
    wxStaticText* slider_text;
	void OnSliderChange(wxCommandEvent& event );
public:
    TransparentSettingDialog ();
    TransparentSettingDialog (wxWindow * parent,
		double trasp,
		wxWindowID id=wxID_ANY,
		const wxString & caption="Transparent Setting Dialog",
		const wxPoint & pos = wxDefaultPosition,
		const wxSize & size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE );
    virtual ~TransparentSettingDialog ();

	double GetTransparency();
};

#endif
