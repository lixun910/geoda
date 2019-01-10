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

#ifndef __GEODA_CENTER_NETWORK_MAP_CANVAS_H__
#define __GEODA_CENTER_NETWORK_MAP_CANVAS_H__

#include <wx/wx.h>
#include <ogrsf_frmts.h>

#include "../osm/TravelTool.h"
#include "../GdaConst.h"
#include "MapNewView.h"

class NetworkMapCanvas : public MapCanvas
{
    DECLARE_CLASS(NetworkMapCanvas)
public:
    NetworkMapCanvas(wxWindow *parent,
                     TemplateFrame* frame,
                     Project* project,
                     double radius,
                     double default_speed,
                     double penalty,
                     std::map<wxString, double> speed_limit_dict,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize);
    virtual ~NetworkMapCanvas();

    /** The function handles all mouse events. */
    virtual void OnMouseEvent(wxMouseEvent& event);
    virtual void DrawLayer2();
    virtual void PopulateCanvas();
    virtual void DisplayRightClickMenu(const wxPoint& pos);
    virtual void UpdateStatusBar();
    
    //virtual void DisplayRightClickMenu(const wxPoint& pos);
    //virtual wxString GetCanvasTitle();
    //virtual wxString GetVariableNames();
    //virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
    //                           SmoothingType new_map_smoothing);
    //virtual void SetCheckMarks(wxMenu* menu);
    //virtual void TimeChange();
    //virtual void CreateAndUpdateCategories();
    //virtual void TimeSyncVariableToggle(int var_index);
    //virtual void UpdateStatusBar();
    //virtual void SetWeightsId(boost::uuids::uuid id) { weights_id = id; }

protected:
    OSMTools::GradientColor* gradient_color;
    OSMTools::TravelTool* travel;
    OGRPoint from_pt;
    OGRPoint to_pt;
    bool has_start_loc;
    std::vector<OGRLineString> path;
    int paint_path_thickness;
    std::vector<std::vector<OGRPolygon> > hexagons;
    std::vector<std::vector<int> > costs;
    wxBitmap marker_img;
    bool b_draw_hex_map;
    bool b_draw_travel_path;

    double radius;
    double default_speed;
    double penalty;

    int num_cats;
    std::vector<wxColour> color_vec;
    std::vector<wxColour> color_labels;
    CatClassification::ColorScheme color_type;

    void CreateHexMap();
    void DrawTravelPath();

    wxColour GetColorByCost(int cost);
    void OnToggleRoadNetwork(wxCommandEvent& event);
    void OnSetStartLocation(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

class NetworkMapFrame : public MapFrame
{
    DECLARE_CLASS(NetworkMapFrame)
public:

    NetworkMapFrame(wxFrame *parent, Project* project,
                    double radius,
                    double default_speed,
                    double penalty,
                    std::map<wxString, double> speed_limit_dict,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = GdaConst::map_default_size,
                 const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~NetworkMapFrame();

    void OnActivate(wxActivateEvent& event);
    //virtual void MapMenus();
    //virtual void UpdateOptionMenuItems();
    //virtual void UpdateContextMenuItems(wxMenu* menu);
    //virtual void update(WeightsManState* o){}

protected:


    DECLARE_EVENT_TABLE()
};


#endif
