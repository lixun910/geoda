//
// Created by Xun Li on 12/26/18.
//

#ifndef OSMTOOLSPROJECT_UITRAVELDISTANCES_H
#define OSMTOOLSPROJECT_UITRAVELDISTANCES_H

#include <vector>
#include <wx/wx.h>

namespace OSMTools {

    class uiTravelDistances {
    public:
        uiTravelDistances(const wxString &title = _("Travel Distances Tool"));

        uiTravelDistances(std::vector<wxString> ds_names,
                const wxString &title = _("Travel Distances Tool"));
    };
}


#endif //OSMTOOLSPROJECT_UITRAVELDISTANCES_H
