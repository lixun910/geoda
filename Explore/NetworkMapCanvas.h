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

#include "../OSMTools/TravelTool.h"
#include "../GdaConst.h"
#include "MapNewView.h"

class NetworkMapCanvas : public MapCanvas
{
    DECLARE_CLASS(NetworkMapCanvas)
public:
    enum BreakMethod {natural_breaks, quantile_breaks, equal_breaks};

    NetworkMapCanvas(wxWindow *parent,
                     TemplateFrame* frame,
                     Project* project,
                     double radius,
                     double default_speed,
                     double penalty,
                     const std::map<wxString, double>& speed_limit_dict,
                     const wxString& highway_type_field,
                     const wxString& max_speed_field,
                     const wxString& one_way_field,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize);
    virtual ~NetworkMapCanvas();

    virtual void OnMouseEvent(wxMouseEvent& event);
    virtual void DrawLayer2();
    virtual void PopulateCanvas();
    virtual void DisplayRightClickMenu(const wxPoint& pos);
    virtual void UpdateStatusBar();

    void UpdateCategoriesByLength(BreakMethod method, int num_cats = 10);
    void UpdateCategoriesByTime();

protected:
    //OSMTools::GradientColor* gradient_color;
    OSMTools::TravelHeatMap* travel;
    OGRPoint from_pt;
    OGRPoint to_pt;
    bool has_start_loc;
    std::vector<OGRLineString> path;
    int paint_path_thickness;
    std::vector<std::vector<OGRPolygon> > hexagons;
    std::vector<std::vector<int> > costs;
    std::vector<int> selectable_costs;
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
    CatClassifDef default_time_cat;
    std::vector<double> breaks;

    void CreateHexMap();
    void DrawTravelPath();

    wxColour GetColorByCost(int cost);
    void OnToggleRoadNetwork(wxCommandEvent& event);
    void OnSetStartLocation(wxCommandEvent& event);
    void OnSaveHeatmap(wxCommandEvent& event);
    void OnDefaultTimeCategories(wxCommandEvent& event);
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
                    const std::map<wxString, double>& speed_limit_dict,
                    const wxString& highway_type_field,
                    const wxString& max_speed_field,
                    const wxString& one_way_field,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = GdaConst::map_default_size,
                 const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~NetworkMapFrame();

    void OnActivate(wxActivateEvent& event);
    virtual void OnNaturalBreaks(int num_cats);
    virtual void OnEqualIntervals(int num_cats);
    virtual void OnQuantile(int num_cats);

protected:


    DECLARE_EVENT_TABLE()
};


#endif
