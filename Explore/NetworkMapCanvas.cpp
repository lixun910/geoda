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
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include <boost/foreach.hpp>

#include "../logger.h"
#include "../Project.h"

#include "NetworkMapCanvas.h"

IMPLEMENT_CLASS(NetworkMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(NetworkMapCanvas, MapCanvas)
EVT_PAINT(TemplateCanvas::OnPaint)
EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
EVT_MOUSE_EVENTS(NetworkMapCanvas::OnMouseEvent)
EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

NetworkMapCanvas::NetworkMapCanvas(wxWindow *parent,
                                   TemplateFrame* t_frame,
                                   Project* project,
                                   double _radius,
                                   double _default_speed,
                                   double _penalty,
                                   std::map<wxString, double> speed_limit_dict,
                                   const wxPoint& pos,
                                   const wxSize& size)
: MapCanvas(parent, t_frame, project, std::vector<GdaVarTools::VarInfo>(0),
            std::vector<int>(0), CatClassification::no_theme, no_smoothing,
            1, boost::uuids::nil_uuid(), pos, size),
b_draw_hex_map(false), b_draw_travel_path(false), radius(_radius),
default_speed(_default_speed), penalty(_penalty)
{
    is_hide = true; // hide main map
    tran_unhighlighted = (1-0.4) * 255; // change default transparency
    isDrawBasemap = true;
    basemap_item = GetBasemapSelection(1); // carto
    DrawBasemap(true, basemap_item);
    
    vector<OGRFeature*>& roads = project->layer_proxy->data;

    //wxString gradient_png_path = GenUtils::GetSamplesDir();
    //gradient_png_path << "gradient-fire.png";
    //gradient_color = new OSMTools::GradientColor(gradient_png_path);

    // color/labels categories
    // 5, 10, 15, 20, 25, 30, 40, 50, 60, >60
    num_cats  = 10;
    color_type = CatClassification::sequential_color_scheme;
    CatClassification::PickColorSet(color_vec, color_type, num_cats, false);

    cat_data.CreateEmptyCategories(num_time_vals, num_obs);
    cat_data.CreateCategoriesAtCanvasTm(num_cats + 1, 0);

    wxString labels[10] = {"5 min", "10 min", "15 min", "20 min", "25 min",
        "30 min", "40 min", "50 min", "60 min", ">60 min"
    };
    for (size_t i=0; i< num_cats; ++i) {
        cat_data.SetCategoryLabel(0, i, labels[i]);
        cat_data.SetCategoryColor(0, i, color_vec[i]);
    }
    cat_data.SetCategoryColor(0, 10, *wxBLACK);
    cat_data.SetCategoryLabel(0, 10, "Original Roads");
    for (size_t i=0; i< num_obs; ++i) {
        cat_data.AppendIdToCategory(0, 10, i);
    }
    cat_data.SetCategoryCount(0, 10, num_obs);

    // map marker
    wxString map_marker_path = GenUtils::GetSamplesDir();
    map_marker_path << "map_marker.png";
    marker_img.LoadFile(map_marker_path, wxBITMAP_TYPE_PNG);

    travel = new OSMTools::TravelTool(roads, default_speed, penalty,
                                      speed_limit_dict);
    travel->BuildCPUGraph();

    // set map center as start location for a hex drive map
    OGREnvelope extent;
    project->GetMapExtent(extent);
    bool create_hexagons = true;
    from_pt.setX(extent.MinX/2.0 + extent.MaxX/2.0);
    from_pt.setY(extent.MinY/2.0 + extent.MaxY/2.0);
    b_draw_hex_map = true;
    has_start_loc = true;

    PopulateCanvas();
}

NetworkMapCanvas::~NetworkMapCanvas()
{
    wxLogMessage("In NetworkMapCanvas::~NetworkMapCanvas");
    delete travel;
    //delete gradient_color;
}

void NetworkMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
    // Workaround for right-click not changing window focus in OSX / wxW 3.0
    wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
    MapFrame* f = dynamic_cast<MapFrame*>(template_frame);
    f->OnActivate(ae);

    sel1 = pos;

    wxMenu* popupMenu = new wxMenu(wxEmptyString);

    popupMenu->Append(XRCID("NETWORKMAP_SET_START"), "Set Start Location Here");
    popupMenu->Append(XRCID("NETWORKMAP_SHOW_MAP"), "Toggle Road Network");

    Connect(XRCID("NETWORKMAP_SHOW_MAP"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(NetworkMapCanvas::OnToggleRoadNetwork));
    Connect(XRCID("NETWORKMAP_SET_START"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(NetworkMapCanvas::OnSetStartLocation));
    PopupMenu(popupMenu, pos);
}

void NetworkMapCanvas::OnToggleRoadNetwork(wxCommandEvent& event)
{
    is_hide = !is_hide;
    layer0_valid = false;
    PopulateCanvas();
}

wxColour NetworkMapCanvas::GetColorByCost(int cost)
{
    int time[9] = {5, 10, 15, 20, 25, 30, 40, 50, 60};

    for (size_t i=0; i<9; ++i) {
        if (cost < time[i] * 60) {
            return color_vec[i];
        }
    }
    if (cost > 10000) return wxColour(255,255,255,0);
    return color_vec[9];
}

void NetworkMapCanvas::OnMouseEvent(wxMouseEvent& event)
{
    // Capture the mouse when left mouse button is down.
    if (event.LeftIsDown() && !HasCapture())
        CaptureMouse();

    if (event.LeftUp() && HasCapture())
        ReleaseMouse();

    if (mousemode == select) {
        int screen_x = event.GetX();
        int screen_y = event.GetY();
        wxPoint screen_pos(screen_x, screen_y);
        wxRealPoint map_pos;

        if (isDrawBasemap) {
            if (basemap == NULL) InitBasemap();
            basemap->ScreenToLatLng(screen_x,screen_y, map_pos.x, map_pos.y);
        } else {
            last_scale_trans.transform_back(screen_pos, map_pos);
        }

        if (event.LeftDClick()) {
            // pick "from location"
            from_pt.setX(map_pos.x);
            from_pt.setY(map_pos.y);
            has_start_loc = true;

            b_draw_hex_map = true;
            PopulateCanvas();

        } else if (has_start_loc && event.LeftDown()) {
            // "to location" of current mouse position
            to_pt.setX(map_pos.x);
            to_pt.setY(map_pos.y);
            if (travel && has_start_loc && from_pt.Equals(&to_pt) == false &&
                from_pt.getX() != 0 && from_pt.getY() != 0) {
                b_draw_travel_path = true;
                layer2_valid = false;
                DrawLayers();
            }
        } else {
            MapCanvas::OnMouseEvent(event);
        }
    } else {
        MapCanvas::OnMouseEvent(event);
    }
}

void NetworkMapCanvas::OnSetStartLocation(wxCommandEvent& event)
{
    wxPoint screen_pos = sel1;
    wxRealPoint map_pos;

    if (isDrawBasemap) {
        if (basemap == NULL) InitBasemap();
        basemap->ScreenToLatLng(screen_pos.x,screen_pos.y, map_pos.x, map_pos.y);
    } else {
        last_scale_trans.transform_back(screen_pos, map_pos);
    }
    // pick "from location"
    from_pt.setX(map_pos.x);
    from_pt.setY(map_pos.y);
    has_start_loc = true;

    b_draw_hex_map = true;
    PopulateCanvas();
}

void NetworkMapCanvas::UpdateStatusBar()
{
}

void NetworkMapCanvas::CreateHexMap()
{
    OGREnvelope extent;
    project->GetMapExtent(extent);
    double hexagon_radius = radius; // meter
    bool create_hexagons = true;

    travel->QueryHexMap(from_pt, extent, hexagon_radius, hexagons, costs,
                        create_hexagons);

    // draw hexagon distance map
    int max_cost = INT_MIN;
    for (size_t i=0; i<costs.size(); ++i) {
        for (size_t j=0; j< costs[i].size(); ++j) {
            if (costs[i][j] < INT_MAX && costs[i][j] > max_cost) {
                max_cost = costs[i][j];
            }
        }
    }

    for (size_t i=0; i<hexagons.size(); ++i) {
        for (size_t j=0; j< hexagons[i].size(); ++j) {
            OGRPolygon poly = hexagons[i][j];
            GdaPolygon* gda_poly = OGRLayerProxy::OGRGeomToGdaShape(&poly);
            wxColour color = GetColorByCost(costs[i][j]);
            wxBrush brush(color);
            gda_poly->setBrush(brush);
            gda_poly->setPen(*wxTRANSPARENT_PEN);
            background_shps.push_back(gda_poly);
        }
    }
}

void NetworkMapCanvas::DrawTravelPath()
{
    if ( (to_pt.getX() == 0 && to_pt.getY() == 0) ||
         (from_pt.getX() == 0 && from_pt.getY() == 0) ) {
        return;
    }
    path.clear();
    std::vector<int> way_ids;
    int cost = travel->Query(from_pt, to_pt, path, way_ids);
    if (way_ids.empty() == false && cost >= 0) {

        BOOST_FOREACH( GdaShape* shp, foreground_shps ) {
            delete shp;
        }
        foreground_shps.clear();

        for (size_t i=0; i<path.size(); ++i) {
            OGRLineString line = path[i];
            int num_pts = line.getNumPoints();
            Shapefile::PolyLineContents* pc = new Shapefile::PolyLineContents();
            pc->shape_type = Shapefile::POLY_LINE;
            pc->num_parts = 1;
            pc->num_points = num_pts;
            pc->points.resize(num_pts);
            OGRPoint p;
            for(int i = 0;  i < num_pts; i++) {
                line.getPoint(i, &p);
                pc->points[i].x = p.getX();
                pc->points[i].y = p.getY();
            }
            GdaPolyLine* polyline =  new GdaPolyLine(pc);
            foreground_shps.push_back(polyline);
        }

        // apply projection
        if (isDrawBasemap) {
            BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
                if (ms)  ms->projectToBasemap(basemap);
            }
        } else {
            BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
                if (ms) ms->applyScaleTrans(last_scale_trans);
            }
        }

        // update status text
        wxStatusBar* sb = 0;
        if (template_frame) sb = template_frame->GetStatusBar();
        wxString current_txt;
        current_txt << "cost=" << cost << " seconds";
        sb->SetStatusText(current_txt);
    }
}

void NetworkMapCanvas::PopulateCanvas()
{
    if (basemap == NULL) InitBasemap();
    
    MapCanvas::PopulateCanvas();

    if (b_draw_hex_map) {
        BOOST_FOREACH( GdaShape* shp, foreground_shps ) {
            delete shp;
        }
        foreground_shps.clear();
        CreateHexMap();
    }

    ReDraw();
}


void NetworkMapCanvas::DrawLayer2()
{
    DrawTravelPath(); // add travel path to foreground_shps

    MapCanvas::DrawLayer2();

    // draw map marker
    wxMemoryDC dc;
    dc.SelectObject(*layer2_bm);

    wxPen pen(*wxRED, 1);
    dc.SetPen(pen);

    wxPoint screen_pos;
    wxRealPoint map_pos(from_pt.getX(), from_pt.getY());

    if (isDrawBasemap) {
        GdaPoint pt(map_pos);
        pt.projectToBasemap(basemap);
        screen_pos.x = pt.center.x;
        screen_pos.y = pt.center.y;
    } else {
        last_scale_trans.transform(map_pos, &screen_pos);
    }

    dc.DrawBitmap(marker_img, screen_pos.x - 16, screen_pos.y -32);
}

IMPLEMENT_CLASS(NetworkMapFrame, MapFrame)
BEGIN_EVENT_TABLE(NetworkMapFrame, MapFrame)
EVT_ACTIVATE(NetworkMapFrame::OnActivate)
END_EVENT_TABLE()
NetworkMapFrame::NetworkMapFrame(wxFrame *parent, Project* project,
                                 double radius,
                                 double default_speed,
                                 double penalty,
                                 std::map<wxString, double> speed_limit_dict,
                                 const wxPoint& pos, const wxSize& size,
                                 const long style)
: MapFrame(parent, project, pos, size, style)
{
    int width, height;
    GetClientSize(&width, &height);
    wxSplitterWindow* splitter_win = new wxSplitterWindow(this, wxID_ANY,
                                            wxDefaultPosition, wxDefaultSize,
                                    wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
    splitter_win->SetMinimumPaneSize(10);

    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new NetworkMapCanvas(rpanel, this, project, radius,
                                           default_speed, penalty,
                                           speed_limit_dict);
    template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);

    wxPanel* lpanel = new wxPanel(splitter_win);
    template_legend = new MapNewLegend(lpanel, template_canvas, wxPoint(0,0), wxSize(0,0));
    wxBoxSizer* lbox = new wxBoxSizer(wxVERTICAL);
    template_legend->GetContainingSizer()->Detach(template_legend);
    lbox->Add(template_legend, 1, wxEXPAND);
    lpanel->SetSizer(lbox);

    splitter_win->SplitVertically(lpanel, rpanel,
                                  GdaConst::map_default_legend_width);

    wxPanel* toolbar_panel = new wxPanel(this,wxID_ANY, wxDefaultPosition);
    wxBoxSizer* toolbar_sizer= new wxBoxSizer(wxVERTICAL);
    toolbar = wxXmlResource::Get()->LoadToolBar(toolbar_panel, "ToolBar_MAP");
    SetupToolbar();
    toolbar_sizer->Add(toolbar, 0, wxEXPAND|wxALL);
    toolbar_panel->SetSizerAndFit(toolbar_sizer);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar_panel, 0, wxEXPAND|wxALL);
    sizer->Add(splitter_win, 1, wxEXPAND|wxALL);
    SetSizer(sizer);
    SetAutoLayout(true);

    SetTitle(_("Road Network Map"));
    DisplayStatusBar(true);
    Show(true);
}

NetworkMapFrame::~NetworkMapFrame()
{
}

void NetworkMapFrame::OnActivate(wxActivateEvent& event)
{
    wxLogMessage("In NetworkMapFrame::OnActivate");
    if (event.GetActive()) {
        RegisterAsActive("NetworkMapFrame", GetTitle());
    }
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}
