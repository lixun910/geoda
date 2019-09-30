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

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <time.h>
#include <vector>
#include <set>

#include "../GeoDaSet.h"
#include "PolysToContigWeights.h"

using namespace std;


/*
 BasePartition
 */
void BasePartition::alloc(const int els, const int cls, const double range)
{
	elements= els;
	cells= cls;
	step= range / cls;
	cell= new int [ cells ];
	next= new int [ elements ];
	if (cell && next)
		for (int cnt= 0; cnt < cells; ++cnt) cell [ cnt ] = EMPTY;
	else elements= cells= 0;
}

/*
 BasePartition
 */
BasePartition::BasePartition(const int els, const int cls, const double range)
: elements(els), cells(cls), cell(0), next(0)
{
	if (elements > 0) alloc(els, cls, range);
}


/*
 BasePartition
 */
BasePartition::~BasePartition()
{
	if (cell) delete [] cell; cell = 0;
	if (next) delete [] next; next = 0;
	elements = 0;
	cells = 0;
}


/*
 PartitionP
 */
void PartitionP::alloc(const int els, const int cls, const double range)  {
	BasePartition::alloc(els, cls, range);
	cellIndex= new int [elements ];
	previous= new int [ elements ];
	if (!cellIndex || !previous)  elements= cells= 0;
}

/*
 PartitionP
 */
PartitionP::PartitionP(const int els, const int cls, const double range) :
BasePartition(els, cls), cellIndex(NULL), previous(NULL)
{
	if (elements > 0)  alloc(els, cls, range);
}

/*
 PartitionP
 */
PartitionP::~PartitionP()
{
	if (cellIndex) delete [] cellIndex;
	if (previous) delete [] previous;
	cellIndex= previous= NULL;
}

/*
 PartitionP::include
 Overloaded function to include an element in the partition.
 Assumes that cellIndex has been initialized.
 */
void PartitionP::include(const int incl)  {
	int where = cellIndex [ incl ];
	//        if (where < 0 || where >= cells || incl < 0 || incl >= elements)
	//          cout << "including " << incl << " at " << where << endl;
	int old= cell [ where ];
	cell [ where ] = incl;
	if (old != EMPTY)
		previous [ old ] = incl;
	next [ incl ] = old;    // OLD becomes the 2nd element in the list
	previous [ incl ] = EMPTY;       // there are no elements prior to incl
	return;
}

/*
 PartitionP
 */
void PartitionP::remove(const int del)  {
	int   thePrevious= previous[ del ], theNext= next[ del ];
	if ( thePrevious == EMPTY )                // this is the 1st element in the list
		cell [ cellIndex[del] ] = theNext;
    else
		next[ thePrevious ] = theNext;
	if ( theNext != EMPTY )                   // this is not the last element in thelist
		previous[ theNext ] = thePrevious;
	previous[ del ] = next [ del ] = EMPTY;  // probably this is not necessary
}



/*
 PolygonPartition:: destructor
 */
PolygonPartition::~PolygonPartition()   {
	if (nbrPoints)  {
        delete [] nbrPoints;
        nbrPoints= NULL;
    }
}



/** Method for detecting if an edge is shared between a host and guest polygon.
 */
bool PolygonPartition::edge(PolygonPartition &p, const int host,
							const int guest, double precision_threshold)
{
    OGRPoint* guestPrev = p.GetPoint(p.prev(guest));
	OGRPoint* hostPoint = this->GetPoint(succ(host));
	
    if (IsSamePoint(hostPoint, guestPrev, precision_threshold)) return true;
	
	OGRPoint* guestSucc= p.GetPoint(p.succ(guest));
    
    if (IsSamePoint(hostPoint, guestSucc, precision_threshold)) return true;
	
	hostPoint= this->GetPoint( prev(host) );
	
    if (IsSamePoint(hostPoint, guestSucc, precision_threshold)) return true;
    if (IsSamePoint(hostPoint, guestPrev, precision_threshold)) return true;
    
	return false;
}

void PolygonPartition::MakePartition(int mX, int mY)  {
	if (mX == 0) mX = NumPoints/4 + 2;
	if (mY == 0) mY = (int)(sqrt((long double)NumPoints) + 2);
	pX.alloc(NumPoints, mX, GetMaxX() - GetMinX());
	pY.alloc(NumPoints, mY, GetMaxY() - GetMinY());
	double xStart= GetMinX(), yStart= GetMinY();
	for (int cnt= 0; cnt < NumPoints; ++cnt)  {
		pX.include(cnt, GetPoint(cnt)->getX() - xStart);
		pY.initIx(cnt, GetPoint(cnt)->getY() - yStart);
	};
	MakeNeighbors();
}

void PolygonPartition::MakeNeighbors()  
{
	nbrPoints= new int [ NumPoints ];
	if (nbrPoints == NULL) return;
	for (int cnt= 0; cnt < NumPoints; ++cnt) {
		nbrPoints [ cnt ] = cnt+1;
	}
	int first= 0, last;
	for (int part= 1; part <= NumParts; ++part) {
		last= (part == NumParts) ? NumPoints : GetPart(part);
		nbrPoints [ first ] = -(last-2);
		nbrPoints [ last-1 ] = first+1;
		first= last;
	}	
}

void PolygonPartition::MakeSmallPartition(const int mX, const double Start,
										  const double Stop)
{
	pX.alloc(NumPoints, mX, Stop-Start);
	for (int cnt= 0; cnt < NumPoints; ++cnt) {
		OGRPoint* pt= GetPoint(cnt);
        if (pt->getX() >= Start && pt->getX() <= Stop) {
            pX.include(cnt, pt->getX() - Start);
        }
	}
	MakeNeighbors();
}

/*
 PolygonPartition::sweep
 Determines if two polygons are neighbors. The host is assumed to be
 partitioned.
 Uses two criteria to establish neighborhood:
 is_queen == true then: common point, else: common boundary
 */
int PolygonPartition::sweep(PolygonPartition & guest, bool is_queen,
                            double precision_threshold)
{
	int host, dot, cly, cell;
	double yStart= GetMinY(), yStop= GetMaxY();
	OGRPoint* pt;
	guest.MakeSmallPartition(pX.Cells(), GetMinX(), GetMaxX());
    
	for (cell = 0; cell < pX.Cells(); ++cell) {
        for (host= pX.first(cell); host != EMPTY;
             host= pX.tail(host))
        {
            pY.include(host);
        }
		for (dot=guest.pX.first(cell); dot != EMPTY;
             dot=guest.pX.tail(dot))
        {
			pt= guest.GetPoint(dot);
			cly= pY.inTheRange(pt->getY() - yStart);
			if (cly != -1) {
				for (host= pY.first(cly); host != EMPTY;
					 host= pY.tail(host))
                {
                    if (IsSamePoint(pt, GetPoint(host), precision_threshold)) {
						if (is_queen ||
                            edge(guest, host, dot, precision_threshold))
                        {
							pY.cleanup(pX, cell);
							return 1;  
						}
					}
				}
			}
		}
		pY.cleanup(pX, cell);
	}
	return 0;
	
}



/*
 PartitionM
 Constructor to initialize the partition
 */
PartitionM::PartitionM(const int els, const int cls, const double range) :
elements(els), cells(cls)  {
	cell= new int[ cells ];
	cellIndex= new int [ elements ];
	lastIndex= new int [ elements ];
	int cnt;
	for (cnt= 0; cnt < cells; ++cnt)
		cell [ cnt ] = EMPTY;
	Refs= new RefPtr [ elements ];
	for (cnt= 0; cnt < elements; ++cnt)
		Refs[cnt]= NULL;
	step= range / cells;
	return;
}

PartitionM::~PartitionM()  {
	if (cell)  {
		delete [] cell;
		cell= NULL;
	};
	if (cellIndex)  {
		delete [] cellIndex;
		cellIndex= NULL;
	};
	if (lastIndex)  {
		delete [] lastIndex;
		lastIndex= NULL;
	};
	if (Refs)  {
		for (int ref= 0; ref < elements; ++ref)
			if (Refs[ref]) delete [] Refs[ref];
		delete [] Refs;
		Refs= NULL;
	};
	cells= 0;
	elements= 0;
	return;
}

int PartitionM::Sum() const {
	int sum= 0;
	for (int cnt= 0; cnt < elements; ++cnt)
		sum += (lastIndex[cnt] - cellIndex[cnt] + 1);
	return sum;
};

void PartitionM::initIx(const int incl, const double lwr, const double upr)  {
    int lower= (int)floor(lwr/step);
    int upper= (int)floor(upr/step);
    
    
    if (lower < 0) {
        lower= 0;
    } else if (lower >= cells) {
        lower= cells-1;
    }
    
    if (upper >= cells) {
        upper= cells-1;
    }
    else if (upper < 0) {
        upper= 0;
    }
    
    if (lower < 0 || upper > cells || incl < 0 || incl >= elements)
    {
        //     cout << "PartM: incl= " << incl << " l= " << lwr << "  " << upr;
        exit(1);
    }

    
	cellIndex [ incl ] = lower;
	lastIndex [ incl ] = upper;
	return;
}

/*
 PartitionM::include
 Include element incl with duration [lower, upper] into the partition.
 */

void PartitionM::include(const int incl)  {
	int cnt, lower= cellIndex [ incl ], upper= lastIndex [ incl ];
	RefPtr rptr= new RefStruct[ upper - lower + 1 ];
	Refs[incl]= rptr;
	for (cnt= upper-lower; cnt >= 0; --cnt)
		rptr[cnt]= Ref();
	for (cnt= lower; cnt <= upper; ++cnt)  {
		int old= cell [ cnt ];            // first element in the cell
		cell [ cnt ] = incl;              // new first element in the cell
		if (old != EMPTY)  {  // the cell was not empty
			rptr[cnt-lower].next = old; // OLD is the next element after incl in the list
			Refs[old][cnt-cellIndex[old]].prev= incl;    // incl is preceeding OLD in the list
		};
	};
	return;
};

/*
 PartitionM::remove
 Remove an element del from the partition.
 */
void PartitionM::remove(const int del)  {
	int   lower= cellIndex[ del ], upper= lastIndex [ del ], cnt;
	for (cnt= lower; cnt <= upper; ++cnt)  {
		RefStruct  cRef= Refs[del][cnt-lower];
		if (cRef.prev < 0)       // this is the first element in the list
			cell[ cnt ]= cRef.next;
		else
			Refs[cRef.prev][ cnt-cellIndex[cRef.prev] ].next= cRef.next;
		if (cRef.next != EMPTY)  // this is not the last element in the list
			Refs[cRef.next][cnt-cellIndex[cRef.next]].prev= cRef.prev;
	};
	delete [] Refs[del];
	Refs[del]= NULL;
	return;
}

GalElement* PolysToContigWeights(OGRLayer* layer, bool is_queen,
                                 double precision_threshold)
{
    OGREnvelope pEnvelope;
    if (layer->GetExtent(&pEnvelope) != OGRERR_NONE) return NULL;
    OGRFeature* feature = NULL;
    vector<OGRFeature*> features;
    layer->ResetReading();
    while ((feature = layer->GetNextFeature()) != NULL) {
        features.push_back(feature);
    }
    
    // # of records in the Shapefile == dimesion of the weights matrix
    long gRecords= layer->GetFeatureCount();
    // partition constructed on lower(x) and upper(x) for each polygon
    BasePartition gMinX, gMaxX;
    // partition constructed on y for each polygon
    PartitionM* gY;
   
    double shp_min_x = (double)pEnvelope.MinX;
    double shp_max_x = (double)pEnvelope.MaxX;
    double shp_min_y = (double)pEnvelope.MinY;
    double shp_max_y = (double)pEnvelope.MaxY;
    double shp_x_len = shp_max_x - shp_min_x;
    double shp_y_len = shp_max_y - shp_min_y;
    
    long gx, gy, cnt, total=0;
    gx = gRecords / 8 + 2;
    
    gMinX.alloc(gRecords, gx, shp_x_len);
    gMaxX.alloc(gRecords, gx, shp_x_len);
    
    for (cnt= 0; cnt < gRecords; ++cnt) {
        feature = features[cnt];
        OGRGeometry* pGeom = feature->GetGeometryRef();
        if (pGeom) {
            OGREnvelope env;
            pGeom->getEnvelope(&env);
            gMinX.include( cnt, env.MinX - shp_min_x );
            gMaxX.include( cnt, env.MaxX - shp_min_x );
        }
    }
    
    gy= (int)(sqrt((long double)gRecords) + 2);
    do {
        gY= new PartitionM(gRecords, gy, shp_y_len );
        for (cnt= 0; cnt < gRecords; ++cnt) {
            feature = features[cnt];
            OGRGeometry* pGeom = feature->GetGeometryRef();
            if (pGeom) {
                OGREnvelope env;
                pGeom->getEnvelope(&env);
                double lwr = env.MinY - shp_min_y;
                double upr = env.MaxY - shp_min_y;
                gY->initIx(cnt, lwr, upr);
            }
        }
        total= gY->Sum();
        if (total > gRecords * 8) {
            delete gY;
            gy = gy/2 + 1;
            total= 0;
        }
    } while ( total == 0);
    
    //GalElement * gl= MakeContiguity(main, is_queen, precision_threshold);
    int curr;
    GalElement * gl= new GalElement [ gRecords ];
    
    if (!gl) return NULL;
    GeoDaSet Neighbors(gRecords), Related(gRecords);
    //  cout << "total steps= " << gMinX.Cells() << endl;
    
    for (int step= 0; step < gMinX.Cells(); ++step) {
        // include all elements from xmin[step]
        for (curr= gMinX.first(step); curr != EMPTY;
             curr= gMinX.tail(curr))
        {
            gY->include(curr);
        }
        
        // test each element in xmax[step]
        for (curr= gMaxX.first(step); curr != EMPTY;
             curr= gMaxX.tail(curr))
        {
            feature = features[curr];
            OGRGeometry* pGeom = feature->GetGeometryRef();
            OGREnvelope env;
            pGeom->getEnvelope(&env);
            PolygonPartition testPoly(pGeom);
            testPoly.MakePartition();
            
            // form a list of neighbors
            for (int cell=gY->lowest(curr); cell <= gY->upmost(curr); ++cell) {
                int potential = gY->first( cell );
                while (potential != EMPTY) {
                    if (potential != curr) Neighbors.Push( potential );
                    potential = gY->tail(potential, cell);
                }
            }
            
            // test each potential neighbor
            for (int nbr = Neighbors.Pop(); nbr != EMPTY;
                 nbr = Neighbors.Pop()) {
                feature = features[nbr];
                OGRGeometry* pNbrGeom = feature->GetGeometryRef();
                OGREnvelope envNbr;
                pNbrGeom->getEnvelope(&envNbr);
                //if (pGeom->Intersects(pNbrGeom)) {
                if (env.Intersects(envNbr)) {
                    
                    PolygonPartition nbrPoly(pNbrGeom);

                    if (curr == 0 && nbr == 0) {
                        
                    }
                    // run sweep with testPoly as a host and nbrPoly as a guest
                    int related = testPoly.sweep(nbrPoly, is_queen, precision_threshold);
                    if (related) Related.Push(nbr);
                }
            }
            
            
            if (size_t sz = Related.Size()) {
                gl[curr].SetSizeNbrs(sz);
                for (size_t i=0; i<sz; ++i) {
                    gl[curr].SetNbr(i, Related.Pop());
                }
            }
            
            gY->remove(curr);       // remove from the partition
        }
    }
    // end MakeContiguity(main, is_queen, precision_threshold);
    
    if (gY) delete gY; gY = 0;

    //MakeFull(gl, gRecords);
    std::vector<std::set<long> > G(gRecords);
    for (size_t i=0; i<gRecords; ++i) {
        for (size_t j=0, sz=gl[i].Size(); j<sz; ++j) {
            G[i].insert(gl[i][j]);
            G[gl[i][j]].insert(i);
        }
    }
    for (size_t i=0; i<gRecords; ++i) {
        if (gl[i].Size() == G[i].size()) continue;
        gl[i].SetSizeNbrs(G[i].size());
        size_t cnt = 0;
        for (set<long>::iterator it=G[i].begin(); it!=G[i].end(); ++it) {
            gl[i].SetNbr(cnt++, *it);
        }
        gl[i].SortNbrs();
    }
    
    return gl;
}
