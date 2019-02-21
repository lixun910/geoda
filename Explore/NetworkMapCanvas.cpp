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

#include "CatClassifManager.h"
#include "../Explore/Basemap.h"
#include "../Algorithms/DataClassify.h"
#include "../DialogTools/ExportDataDlg.h"
#include "../DataViewer/OGRTable.h"
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
                                   const std::map<wxString, double>& speed_limit_dict,
                                   const wxString& highway_type_field,
                                   const wxString& max_speed_field,
                                   const wxString& one_way_field,
                                   const wxPoint& pos,
                                   const wxSize& size)
: MapCanvas(parent, t_frame, project, std::vector<GdaVarTools::VarInfo>(0),
            std::vector<int>(0), CatClassification::no_theme, no_smoothing,
            1, boost::uuids::nil_uuid(), pos, size),
b_draw_hex_map(false), b_draw_travel_path(false), radius(_radius),
default_speed(_default_speed), penalty(_penalty)
{
    selectable_shps_type = polygons; // for hex polygons
    
    vector<OGRFeature*>& roads = project->layer_proxy->data;

    //wxString gradient_png_path = GenUtils::GetSamplesDir();
    //gradient_png_path << "gradient-fire.png";
    //gradient_color = new OSMTools::GradientColor(gradient_png_path);


    // color/labels categories
    // 5, 10, 15, 20, 25, 30, 40, 50, 60, >60
    num_cats  = 10;
    color_type = CatClassification::sequential_color_scheme;
    CatClassification::PickColorSet(color_vec, color_type, num_cats, false);
    
    // map marker
    wxString map_marker_path = GenUtils::GetSamplesDir();
    map_marker_path << "map_marker.png";
    marker_img.LoadFile(map_marker_path, wxBITMAP_TYPE_PNG);

    travel = new OSMTools::TravelHeatMap(roads, default_speed, penalty,
                                         speed_limit_dict,
                                         highway_type_field,
                                         max_speed_field,
                                         one_way_field);

    // set map center as start location for a hex drive map
    OGREnvelope extent;
    project->GetMapExtent(extent);
    bool create_hexagons = true;
    from_pt.setX(extent.MinX/2.0 + extent.MaxX/2.0);
    from_pt.setY(extent.MinY/2.0 + extent.MaxY/2.0);
    b_draw_hex_map = true;
    has_start_loc = true;

    // default time cost categories
    /*
    wxString default_time_cost_cat = _("Default Travel Time Category");
    CatClassifManager* cat_classif_manager = project->GetCatClassifManager();
    CatClassifState* cc = cat_classif_manager->FindClassifState(default_time_cost_cat);
    if (cc == NULL && travel->IsOpenStreeMap()) {
        default_time_cat.title = default_time_cost_cat;
        default_time_cat.cat_classif_type = CatClassification::custom;
        default_time_cat.break_vals_type = CatClassification::custom_break_vals;
        default_time_cat.num_cats = 10;
        default_time_cat.names.resize(10);
        default_time_cat.names[0] = _("5 mins");
        default_time_cat.names[1] = _("10 mins");
        default_time_cat.names[2] = _("15 mins");
        default_time_cat.names[3] = _("20 mins");
        default_time_cat.names[4] = _("25 mins");
        default_time_cat.names[5] = _("30 mins");
        default_time_cat.names[6] = _("40 mins");
        default_time_cat.names[7] = _("50 mins");
        default_time_cat.names[8] = _("60 mins");
        default_time_cat.names[9] = _("> 60 mins");
        default_time_cat.assoc_db_fld_name = "";
        default_time_cat.breaks.resize(9);
        default_time_cat.breaks[0] = 300; // seconds
        default_time_cat.breaks[1] = 600;
        default_time_cat.breaks[2] = 900;
        default_time_cat.breaks[3] = 1200;
        default_time_cat.breaks[4] = 1500;
        default_time_cat.breaks[5] = 1800;
        default_time_cat.breaks[6] = 2400;
        default_time_cat.breaks[7] = 3000;
        default_time_cat.breaks[8] = 3600;
        default_time_cat.color_scheme = CatClassification::sequential_color_scheme;
        cat_classif_manager->CreateNewClassifState(default_time_cat);
    }
     */
    tran_unhighlighted = (1-0.4) * 255; // change default transparency
    isDrawBasemap = true;
    // use carto
    basemap_item = Gda::GetBasemapSelection(1, GdaConst::gda_basemap_sources);
    DrawBasemap(true, basemap_item);
    
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
    wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_ROAD_CHANGE_CLASSIFICATION");
    optMenu->Append(XRCID("NETWORKMAP_TIME_CATEGORIES"),
                    _("Default Travel Time Categories"));
    popupMenu->AppendSubMenu(optMenu, _("Change Map Classification"));
    popupMenu->Append(XRCID("NETWORKMAP_SET_START"), _("Set Start Location Here"));
    //popupMenu->Append(XRCID("NETWORKMAP_SHOW_MAP"), _("Toggle Road/Network"));
    popupMenu->Append(XRCID("NETWORKMAP_SAVE_MAP"), _("Save Road/Network Heatmap"));

    //Connect(XRCID("NETWORKMAP_SHOW_MAP"), wxEVT_COMMAND_MENU_SELECTED,
    //        wxCommandEventHandler(NetworkMapCanvas::OnToggleRoadNetwork));
    Connect(XRCID("NETWORKMAP_SET_START"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(NetworkMapCanvas::OnSetStartLocation));
    Connect(XRCID("NETWORKMAP_SAVE_MAP"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(NetworkMapCanvas::OnSaveHeatmap));
    Connect(XRCID("NETWORKMAP_TIME_CATEGORIES"), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(NetworkMapCanvas::OnDefaultTimeCategories));

    PopupMenu(popupMenu, pos);
}

void NetworkMapCanvas::OnDefaultTimeCategories(wxCommandEvent& event)
{
    UpdateCategoriesByTime();
    ReDraw();
    if (template_frame) {
        TemplateLegend* legend = template_frame->GetTemplateLegend();
        if (legend) {
            legend->Recreate();
        }
    }
}

void NetworkMapCanvas::OnToggleRoadNetwork(wxCommandEvent& event)
{
}

wxColour NetworkMapCanvas::GetColorByCost(int cost)
{
    if (cost == INT_MAX) return wxColour(255,255,255,0);

    if (travel->IsOpenStreeMap()) {
        int time[9] = {5, 10, 15, 20, 25, 30, 40, 50, 60};
        for (size_t i=0; i<9; ++i) {
            if (cost < time[i] * 60) {
                return color_vec[i];
            }
        }
        return color_vec[9];
    } else {
        for (size_t i=0; i<breaks.size(); ++i) {
            if (cost < breaks[i]) {
                return color_vec[i];
            }
        }
        return color_vec[breaks.size() -1];
    }
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

void NetworkMapCanvas::OnSaveHeatmap(wxCommandEvent& event)
{
    int n_rows = hexagons.size() * hexagons.size();
    if (n_rows <= 0) {
        wxString msg = _("Please specify a proper start location on map to create a heatmap first.");
        wxMessageDialog dlg(this, msg, _("Info"),wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }

    std::vector<GdaShape*> new_geoms;
    size_t cnt = 0;
    for (size_t i=0; i<hexagons.size(); ++i) {
        for (size_t j=0; j< hexagons[i].size(); ++j) {
            if (costs[i][j] == INT_MAX) continue;
            GdaPolygon* gda_poly = OGRLayerProxy::OGRGeomToGdaShape(&hexagons[i][j]);
            new_geoms.push_back(gda_poly);
            cnt += 1;
        }
    }

    OGRTable* mem_table = new OGRTable(cnt);
    vector<bool> undefs(cnt, false);
    int int_len = 20;
    OGRColumnInteger* row_col = new OGRColumnInteger("row", int_len, 0, cnt);
    OGRColumnInteger* col_col = new OGRColumnInteger("col", int_len, 0, cnt);
    OGRColumnInteger* cost_col = new OGRColumnInteger("cost", int_len, 0, cnt);
    cnt = 0;
    for (size_t i=0; i<hexagons.size(); ++i) {
        for (size_t j=0; j< hexagons[i].size(); ++j) {
            if (costs[i][j] == INT_MAX) continue;
            row_col->SetValueAt(cnt, (wxInt64)i);
            col_col->SetValueAt(cnt, (wxInt64)j);
            cost_col->SetValueAt(cnt, (wxInt64)costs[i][j]);
            cnt += 1;
        }
    }
    mem_table->AddOGRColumn(row_col);
    mem_table->AddOGRColumn(col_col);
    mem_table->AddOGRColumn(cost_col);

    OGRSpatialReference spatial_ref;
    spatial_ref.importFromEPSG(4326); // always use lat/lon
    Shapefile::ShapeType shape_type = Shapefile::POLYGON;
    ExportDataDlg export_dlg(this, shape_type, new_geoms, &spatial_ref, mem_table);
    if (export_dlg.ShowModal() == wxID_OK) {
        wxMessageDialog dlg(this, _("Heatmap saved successfully."),
                            _("Success"), wxOK);
        dlg.ShowModal();
    }
    delete mem_table;
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

    // Get selectable objects and its costs
    selectable_costs.clear();
    for (size_t i=0; i<hexagons.size(); ++i) {
        for (size_t j=0; j< hexagons[i].size(); ++j) {
            if (costs[i][j] == INT_MAX) continue;
            selectable_costs.push_back(costs[i][j]);
        }
    }

    // Use selectable Costs to create categories
    if (travel->IsOpenStreeMap()) {
        UpdateCategoriesByTime();
    } else {
        UpdateCategoriesByLength(natural_breaks);
    }

    // Create Selectable Shapes
    for (size_t i=0; i<selectable_shps.size(); ++i) delete selectable_shps[i];
    selectable_shps.clear();
    for (size_t i=0; i<hexagons.size(); ++i) {
        for (size_t j=0; j< hexagons[i].size(); ++j) {
            if (costs[i][j] == INT_MAX) continue;
            OGRPolygon poly = hexagons[i][j];
            GdaPolygon* gda_poly = OGRLayerProxy::OGRGeomToGdaShape(&poly);
            wxColour color = GetColorByCost(costs[i][j]);
            wxBrush brush(color);
            gda_poly->setBrush(brush);
            gda_poly->setPen(*wxTRANSPARENT_PEN);
            selectable_shps.push_back(gda_poly);
        }
    }
}

void NetworkMapCanvas::UpdateCategoriesByTime()
{
    // default cat is 10
    num_cats  = 10;
    color_type = CatClassification::sequential_color_scheme;
    CatClassification::PickColorSet(color_vec, color_type, num_cats, false);

    cat_data.CreateEmptyCategories(num_time_vals, selectable_costs.size());
    cat_data.CreateCategoriesAtCanvasTm(num_cats, 0);

    int time[9] = {5, 10, 15, 20, 25, 30, 40, 50, 60};
    for (size_t i=0; i<selectable_costs.size(); ++i) {
        bool added = false;
        for (size_t j=0; j<9; j++) {
            if (selectable_costs[i] < time[j] * 60) {
                cat_data.AppendIdToCategory(0, j, i);
                added = true;
                break;
            }
        }
        if (added == false) cat_data.AppendIdToCategory(0, 9, i);
    }
    wxString labels[10] = {"5 min", "10 min", "15 min", "20 min", "25 min",
        "30 min", "40 min", "50 min", "60 min", ">60 min"
    };

    for (size_t i=0; i< num_cats; ++i) {
        cat_data.SetCategoryLabel(0, i, labels[i]);
        cat_data.SetCategoryColor(0, i, color_vec[i]);
        int count = cat_data.GetNumObsInCategory(0, i);
        cat_data.SetCategoryCount(0, i, count);
    }
}

void NetworkMapCanvas::UpdateCategoriesByLength(BreakMethod method, int num_cats)
{
    std::vector<double> sel_costs(selectable_costs.size());
    for (size_t i=0; i<selectable_costs.size(); ++i) {
        sel_costs[i] = selectable_costs[i];
    }
    std::vector<bool> undefs(sel_costs.size(), false);
    int n_breaks = num_cats;
    breaks.clear();
    if (method == natural_breaks)
        breaks = ClassifyUtils::NaturalBreaks(sel_costs, undefs, n_breaks);
    else if (method == quantile_breaks)
        breaks = ClassifyUtils::QuantileBreaks(sel_costs, undefs, n_breaks);
    else
        breaks = ClassifyUtils::EqualBreaks(sel_costs, undefs, n_breaks);

    cat_data.CreateEmptyCategories(num_time_vals, sel_costs.size());
    cat_data.CreateCategoriesAtCanvasTm(n_breaks, 0);
    
    for (size_t i=0; i<sel_costs.size(); ++i) {
        bool added = false;
        for (size_t j=0; j<breaks.size(); j++) {
            if (sel_costs[i] < breaks[j]) {
                cat_data.AppendIdToCategory(0, j, i);
                added = true;
                break;
            }
        }
        if (added == false) cat_data.AppendIdToCategory(0, n_breaks, i);
    }

    color_type = CatClassification::sequential_color_scheme;
    CatClassification::PickColorSet(color_vec, color_type, n_breaks, false);

    wxString lbl_tpt = "[%.2f, %.2f]";
    for (size_t i=0; i< n_breaks; ++i) {
        wxString lbl;
        if (i == 0) {
            lbl << "< " << breaks[i];
        } else if (i < n_breaks - 1) {
            lbl = wxString::Format(lbl_tpt, breaks[i-1], breaks[i]);
        } else {
            lbl << "> " << breaks[i-1];
        }
        cat_data.SetCategoryLabel(0, i, lbl);
        cat_data.SetCategoryColor(0, i, color_vec[i]);
        int count = cat_data.GetNumObsInCategory(0, i);
        cat_data.SetCategoryCount(0, i, count);
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
                //foreground_shps.push_back(new GdaPoint(p.getX(),p.getY()));
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
        current_txt << "cost=" << cost;
        if (travel->IsOpenStreeMap()) current_txt << " seconds";
        sb->SetStatusText(current_txt);
    }
}

void NetworkMapCanvas::PopulateCanvas()
{
    if (basemap == NULL) InitBasemap();
    // create hex map and use it as working map
    CreateHexMap();
    MapCanvas::PopulateCanvas();
    // only draw the latest selected route/path
    if (b_draw_hex_map) {
        BOOST_FOREACH( GdaShape* shp, foreground_shps )  delete shp;
        foreground_shps.clear();
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
                                 const std::map<wxString, double>& speed_limit_dict,
                                 const wxString& highway_type_field,
                                 const wxString& max_speed_field,
                                 const wxString& one_way_field,
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
                                           speed_limit_dict,
                                           highway_type_field,
                                           max_speed_field,
                                           one_way_field);
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

    SetTitle(_("Road/Network HeatMap"));
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

void NetworkMapFrame::OnNaturalBreaks(int num_cats)
{
    NetworkMapCanvas* canvas = (NetworkMapCanvas*) template_canvas;
    canvas->UpdateCategoriesByLength(NetworkMapCanvas::natural_breaks, num_cats);
    canvas->ReDraw();
    template_legend->Recreate();
}

void NetworkMapFrame::OnEqualIntervals(int num_cats)
{
    NetworkMapCanvas* canvas = (NetworkMapCanvas*) template_canvas;
    canvas->UpdateCategoriesByLength(NetworkMapCanvas::equal_breaks, num_cats);
    canvas->ReDraw();
    template_legend->Recreate();
}

void NetworkMapFrame::OnQuantile(int num_cats)
{
    NetworkMapCanvas* canvas = (NetworkMapCanvas*) template_canvas;
    canvas->UpdateCategoriesByLength(NetworkMapCanvas::quantile_breaks, num_cats);
    canvas->ReDraw();
    template_legend->Recreate();
}
