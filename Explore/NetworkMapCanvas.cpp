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
EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

NetworkMapCanvas::NetworkMapCanvas(wxWindow *parent,
                                   TemplateFrame* t_frame,
                                   Project* project,
                                   const wxPoint& pos,
                                   const wxSize& size)
: MapCanvas(parent, t_frame, project, std::vector<GdaVarTools::VarInfo>(0),
            std::vector<int>(0), CatClassification::no_theme, no_smoothing,
            1, boost::uuids::nil_uuid(), pos, size)
{
    vector<OGRFeature*>& roads = project->layer_proxy->data;

    wxString gradient_png_path = GenUtils::GetSamplesDir();
    {
    gradient_png_path << "gradient-fire.png";
    }
    gradient_color = new OSMTools::GradientColor(gradient_png_path);

    wxString map_marker_path = GenUtils::GetSamplesDir();
    map_marker_path << "map_marker.png";
    marker_img.LoadFile(map_marker_path, wxBITMAP_TYPE_PNG);

    travel = new OSMTools::TravelTool(roads);
    travel->BuildCPUGraph();
}

NetworkMapCanvas::~NetworkMapCanvas()
{
    wxLogMessage("In NetworkMapCanvas::~NetworkMapCanvas");
    delete travel;
    delete gradient_color;
}

void NetworkMapCanvas::OnMouseEvent(wxMouseEvent& event)
{
    int screen_x = event.GetX();
    int screen_y = event.GetY();
    wxPoint screen_pos(screen_x, screen_y);
    wxRealPoint map_pos;
    last_scale_trans.transform_back(screen_pos, map_pos);

    if (event.LeftDClick()) {
        // pick "from location"
        from_pt.setX(map_pos.x);
        from_pt.setY(map_pos.y);
        has_start_loc = true;

        CreateHexMap();

    } else if (has_start_loc && event.LeftDown()) {
        // "to location" of current mouse position
        to_pt.setX(map_pos.x);
        to_pt.setY(map_pos.y);
        if (travel && has_start_loc && from_pt.Equals(&to_pt) == false &&
            from_pt.getX() != 0 && from_pt.getY() != 0) {
            DrawTravelPath();
        }
    } else {
        MapCanvas::OnMouseEvent(event);
    }
}

void NetworkMapCanvas::CreateHexMap()
{
    OGREnvelope extent;
    project->GetMapExtent(extent);
    double hexagon_radius = 160; // meter
    bool create_hexagons = true;

    travel->QueryHexMap(from_pt, extent, hexagon_radius, hexagons, costs,
                        create_hexagons);
    layer0_valid = false;
    DrawLayers();
    Refresh();
}

void NetworkMapCanvas::DrawTravelPath()
{
    path.clear();
    std::vector<int> way_ids;
    int cost = travel->Query(from_pt, to_pt, path, way_ids);
    if (way_ids.empty() == false && cost >= 0) {

        layer1_valid = false;
        DrawLayers();
        // update status text
        wxStatusBar* sb = 0;
        if (template_frame) sb = template_frame->GetStatusBar();
        wxString current_txt;
        current_txt << "cost=" << cost;
        sb->SetStatusText(current_txt);
    }
}

void NetworkMapCanvas::DrawLayer0()
{
    // draw hexagon distance map
    int max_cost = INT_MIN;
    for (size_t i=0; i<costs.size(); ++i) {
        for (size_t j=0; j< costs[i].size(); ++j) {
            if (costs[i][j] < INT_MAX && costs[i][j] > max_cost) {
                max_cost = costs[i][j];
            }
        }
    }
    BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
    background_shps.clear();

    for (size_t i=0; i<hexagons.size(); ++i) {
        for (size_t j=0; j< hexagons[i].size(); ++j) {
            OGRPolygon poly = hexagons[i][j];
            GdaPolygon* gda_poly = OGRLayerProxy::OGRGeomToGdaShape(&poly);
            double val = (double)costs[i][j] / (double)max_cost;
            if (val > 1) val = 1;
            wxColour color = gradient_color->GetColor(val);
            wxBrush brush(color);
            gda_poly->setBrush(brush);
            gda_poly->setPen(*wxTRANSPARENT_PEN);
            background_shps.push_back(gda_poly);
        }
    }

    MapCanvas::DrawLayer0();
}

void NetworkMapCanvas::DrawLayer2()
{
    MapCanvas::DrawLayer2();

    wxMemoryDC dc;
    dc.SelectObject(*layer2_bm);
    // draw foreground
    wxPen pen(*wxRED, 1);
    dc.SetPen(pen);
    wxRealPoint pt1, pt2;
    wxPoint screen_pt1, screen_pt2;
    OGRPoint pt;
    for (size_t i=0; i<path.size(); ++i) {
        OGRLineString line = path[i];
        for (size_t j=0; j<line.getNumPoints()-1; ++j) {
            line.getPoint(j, &pt);
            pt1.x = pt.getX();
            pt1.y = pt.getY();
            last_scale_trans.transform(pt1, &screen_pt1);
            line.getPoint(j+1, &pt);
            pt2.x = pt.getX();
            pt2.y = pt.getY();
            last_scale_trans.transform(pt2, &screen_pt2);
            dc.DrawLine(screen_pt1, screen_pt2);
        }
    }

    wxPoint screen_pos;
    wxRealPoint map_pos(from_pt.getX(), from_pt.getY());
    last_scale_trans.transform(map_pos, &screen_pos);
    dc.DrawBitmap(marker_img, screen_pos.x - 16, screen_pos.y -32);
}

IMPLEMENT_CLASS(NetworkMapFrame, MapFrame)
BEGIN_EVENT_TABLE(NetworkMapFrame, MapFrame)
END_EVENT_TABLE()
NetworkMapFrame::NetworkMapFrame(wxFrame *parent, Project* project,
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
    template_canvas = new NetworkMapCanvas(rpanel, this, project);
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
