/******************************************************/
/*                                                    */
/* geoidboundary.cpp - geoid boundaries               */
/*                                                    */
/******************************************************/
/* Copyright 2016,2017 Pierre Abbat.
 * This file is part of Bezitopo.
 *
 * Bezitopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Bezitopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License and Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and Lesser General Public License along with Bezitopo. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cassert>
#include <iostream>
#include <cfloat>
#include "geoidboundary.h"
#include "spolygon.h"
#include "manysum.h"
#include "projection.h"
#include "random.h"
#include "ellipsoid.h"
#include "relprime.h"
using namespace std;

#define VBTOLER (20*DBL_EPSILON)
// DBL_EPSILON in vball coordinates is 0.707 nm at the center of a face.
#define cmpeq(a,b) (fabs(a-b)<VBTOLER)

char vballcompare[8][8]=
{
  {00,77,77,77,77,77,77,77},
  {77,66,12,21,14,36,77,77},
  {77,21,66,12,36,77,14,77},
  {77,12,21,66,77,14,36,77},
  {77,41,63,77,66,45,54,77},
  {77,63,77,41,54,66,45,77},
  {77,77,41,63,45,54,66,77},
  {77,77,77,77,77,77,77,77}
};

bool operator==(const vball &a,const vball &b)
{
  int edgetype=vballcompare[a.face][b.face];
  bool ret=false;
  switch (edgetype)
  {
    case 00:
      ret=true;
      break;
    case 12:
      ret=a.x==1 && a.y==b.x && b.y==1;
      break;
    case 21:
      ret=a.y==1 && a.x==b.y && b.x==1;
      break;
    case 14:
      ret=a.y==-1 && a.x==-b.y && b.x==1;
      break;
    case 41:
      ret=a.x==1 && a.y==-b.x && b.y==-1;
      break;
    case 36:
      ret=a.x==-1 && a.y==b.x && b.y==-1;
      break;
    case 63:
      ret=a.y==-1 && a.x==b.y && b.x==-1;
      break;
    case 45:
      ret=a.y==1 && a.x==-b.y && b.x==-1;
      break;
    case 54:
      ret=a.x==-1 && a.y==-b.x && b.y==1;
      break;
    case 66:
      ret=cmpeq(a.x,b.x) && cmpeq(a.y,b.y);
      break;
  }
  return ret;
}

bool sameEdge(const vball &a,const vball &b)
{
  int edgetype=vballcompare[a.face][b.face];
  bool ret=false;
  switch (edgetype)
  {
    case 00:
      ret=true;
      break;
    case 12:
      ret=a.x==1 && b.y==1;
      break;
    case 21:
      ret=a.y==1 && b.x==1;
      break;
    case 14:
      ret=a.y==-1 && b.x==1;
      break;
    case 41:
      ret=a.x==1 && b.y==-1;
      break;
    case 36:
      ret=a.x==-1 && b.y==-1;
      break;
    case 63:
      ret=a.y==-1 && b.x==-1;
      break;
    case 45:
      ret=a.y==1 && b.x==-1;
      break;
    case 54:
      ret=a.x==-1 && b.y==1;
      break;
    case 66:
      ret=a.x==b.x || a.y==b.y;
      break;
  }
  return ret;
}

char log29[]={
  63,
  0,1,5,2,22,6,12,
  3,10,23,25,7,18,13,
  27,4,21,11,9,24,17,
  26,20,8,16,19,15,14
};

vball vsegment::midpoint()
/* This always returns a point on the great circle segment between the ends,
 * but usually not the exact midpoint. If the segment crosses a face boundary,
 * it computes the exact midpoint, which takes longer.
 */
{
  vball ret;
  int i;
  for (i=0;i<9 && start.face!=end.face;i++)
    if (i%3)
      start.switchFace();
    else
      end.switchFace();
  if (start.face==end.face)
  {
    ret.face=start.face;
    ret.x=(start.x+end.x)/2;
    ret.y=(start.y+end.y)/2;
  }
  else
    ret=encodedir(decodedir(start)+decodedir(end));
  return ret;
}

int splitLevel(double coord)
/* Returns the number of times a geoquad has to be split to produce
 * (coord,coord) as a boundary point. This is used when merging boundaries,
 * as only those segments with the lowest level need be considered.
 */
{
  int i,n,ret;
  if (coord==rint(coord))
    ret=coord==0;
  else
  {
    coord=fabs(coord);
    for (n=-1,i=0;coord!=n;i++)
    {
      coord=(coord-trunc(coord))*16777216;
      n=rint(coord);
    }
    n=n&-n;
    ret=i*24-log29[n%29]+1;
  }
  return ret;
}

int splitLevel(vball v)
{
  int xLevel=splitLevel(v.x),yLevel=splitLevel(v.y);
  if (xLevel<yLevel)
    return xLevel;
  else
    return yLevel;
}

int splitLevel(vsegment v)
{
  if (v.start.face==v.end.face)
    if (v.start.x==v.end.x)
      return splitLevel(v.start.x);
    else if (v.start.y==v.end.y)
      return splitLevel(v.start.y);
    else
      return -1;
  else if (sameEdge(v.start,v.end))
    return 0;
  else
    return -1;
}

bool g1boundary::isempty()
{
  return !bdy.size();
}

bool g1boundary::isInner()
{
  return inner;
}

void g1boundary::setInner(bool i)
{
  inner=i;
}

int g1boundary::size()
{
  return bdy.size();
}

void g1boundary::clear()
{
  bdy.clear();
}

void g1boundary::push_back(vball v)
/* A g1boundary is initialized with four points, the corners of a geoquad
 * in counterclockwise order. A clockwise g1boundary is the boundary
 * of a hole in a region.
 */
{
  bdy.push_back(v);
}

vball g1boundary::operator[](int n)
{
  assert(bdy.size());
  n%=bdy.size();
  if (n<0)
    n+=bdy.size();
  return bdy[n];
}

vsegment g1boundary::seg(int n)
{
  vsegment ret;
  assert(bdy.size());
  n%=(signed)bdy.size();
  if (n<0)
    n+=bdy.size();
  ret.start=bdy[n];
  ret.end=bdy[(n+1)%bdy.size()];
  return ret;
}

vector<int> g1boundary::segmentsAtLevel(int l)
/* This returns indices, not segments, because the indices will be necessary
 * for surgery.
 */
{
  int i;
  vector<int> ret;
  for (i=0;i<bdy.size();i++)
    if (l<0 || splitLevel(seg(i))==l)
      ret.push_back(i);
  return ret;
}

vector<int> g1boundary::nullSegments()
{
  int i;
  vector<int> ret;
  vsegment vseg;
  for (i=0;i<bdy.size();i++)
  {
    vseg=seg(i);
    if (vseg.start==vseg.end)
      ret.push_back(i);
  }
  return ret;
}

void g1boundary::positionSegment(int n)
/* Rolls the vector of vballs so that the one at n becomes last and the one
 * at n+1 becomes 0th, so that boundaries can be easily spliced.
 */
{
  int m=n+1;
  vector<vball> bdy1(bdy);
  if (bdy.size())
  {
    n%=bdy.size();
    if (n<0)
      n+=bdy.size();
    m%=bdy.size();
    if (m<0)
      m+=bdy.size();
    if (m>n)
    {
      memmove(&bdy1[0],&bdy[m],(bdy.size()-m)*sizeof(vball));
      memmove(&bdy1[bdy.size()-m],&bdy[0],m*sizeof(vball));
      swap(bdy,bdy1);
    }
  }
}

void g1boundary::splice(g1boundary &b)
{
  int oldsize=bdy.size();
  bdy.resize(oldsize+b.bdy.size());
  memmove(&bdy[oldsize],&b.bdy[0],b.bdy.size()*sizeof(vball));
  b.bdy.clear();
}

void g1boundary::split(int n,g1boundary &b)
{
  n%=(signed int)bdy.size();
  /* "n%=bdy.size()" is wrong. If size()=10, and n=-4, this results in 2,
   * because (4294967296-4)%10=2.
   */
  if (n<0)
    n+=bdy.size();
  b.bdy.resize(bdy.size()-n);
  memmove(&b.bdy[0],&bdy[n],(bdy.size()-n)*sizeof(vball));
  bdy.resize(n);
}

void g1boundary::splice(int m,g1boundary &b,int n)
/* Splice together this, at its mth segment, and b, at its nth segment.
 * this is left with one of the resulting segments between the back and front.
 * b is left empty.
 */
{
  positionSegment(m);
  b.positionSegment(n);
  splice(b);
}

void g1boundary::split(int m,int n,g1boundary &b)
/* Splits this into two loops, cutting segments m and n and making new ones.
 * Any previous content of b is overwritten.
 */
{
  positionSegment(m);
  split(n-m,b);
}

void g1boundary::halve(int n)
{
  positionSegment(n);
  bdy.push_back(seg(-1).midpoint());
}

void g1boundary::deleteCollinear()
{
  int i,sz;
  bool found;
  do
  {
    found=false;
    sz=bdy.size();
    for (i=0;i<sz && !found;i++)
      if (sameEdge(bdy[i],bdy[(i+1)%sz]) && sameEdge(bdy[(i+1)%sz],bdy[(i+2)%sz]) && sameEdge(bdy[(i+2)%sz],bdy[i]))
      {
        found=true;
        positionSegment(i+1);
        bdy.resize(sz-1);
      }
  } while (found);
}

void g1boundary::deleteRetrace()
{
  int i,sz;
  bool found;
  do
  {
    found=false;
    sz=bdy.size();
    for (i=0;i<sz && !found;i++)
      if (bdy[i]==bdy[(i+2)%sz] || bdy[i]==bdy[(i+1)%sz] || bdy[(i+1)%sz]==bdy[(i+2)%sz])
      {
        found=true;
        positionSegment(i+1);
        bdy.resize(sz-1);
      }
  } while (found);
}

vector<xyz> g1boundary::surfaceCorners()
{
  vector<xyz> ret;
  int i;
  ret.resize(bdy.size());
  for (i=0;i<bdy.size();i++)
  {
    ret[i]=decodedir(bdy[i]);
    if (i && dist(ret[i-1],ret[i])<1)
      cout<<"Adjacent points very close"<<endl;
  }
  return ret;
}

vector<xyz> g1boundary::surfaceMidpoints()
{
  vector<xyz> ret;
  int i;
  ret.resize(bdy.size());
  for (i=0;i<bdy.size();i++)
  {
    ret[i]=(decodedir(bdy[i])+decodedir(bdy[(i+1)%bdy.size()]));
    ret[i]*=EARTHRAD/ret[i].length();
  }
  return ret;
}

double g1boundary::perimeter(bool midpt)
{
  return surfacePerimeter(midpt?surfaceMidpoints():surfaceCorners());
}

int g1boundary::area()
{
  return iSurfaceArea(surfaceCorners());
}

double g1boundary::cubeArea()
/* Area on the cube projection. This will give garbage if the
 * path crosses an edge.
 */
{
  int i,sz=bdy.size();
  vector<double> xmul;
  for (i=0;i<sz;i++)
    xmul.push_back(bdy[(i+1)%sz].y*bdy[i].x-
                   bdy[(i+1)%sz].x*bdy[i].y);
  return pairwisesum(xmul)/2;
}

void moveToFace(vball &v,int f)
/* Moves v to face f, assuming that it's on face f (in which case it does
 * nothing) or on the edge of an adjacent face.
 */
{
  int edgetype=vballcompare[v.face][f];
  assert(edgetype<77 && edgetype>00);
  switch (edgetype)
  {
    case 12:
      v.x=v.y;
      v.y=1;
      break;
    case 21:
      v.y=v.x;
      v.x=1;
      break;
    case 14:
      v.y=-v.x;
      v.x=1;
      break;
    case 41:
      v.x=-v.y;
      v.y=-1;
      break;
    case 36:
      v.x=v.y;
      v.y=-1;
      break;
    case 63:
      v.y=v.x;
      v.x=-1;
      break;
    case 45:
      v.y=-v.x;
      v.x=-1;
      break;
    case 54:
      v.x=-v.y;
      v.y=1;
      break;
  }
  v.face=f;
}

bool operator==(const g1boundary l,const g1boundary r)
/* If one is rotated from the other, returns false. They have to start at
 * the same place for it to return true. This is used in kml to compare
 * a gboundary with a copy of itself. Ignores the inner bit.
 */
{
  int i,minsize;
  minsize=l.bdy.size();
  if (r.bdy.size()<minsize)
    minsize=r.bdy.size();
  for (i=0;i<minsize && l.bdy[i]==r.bdy[i];i++);
  return i==l.bdy.size() && i==r.bdy.size();
}

bool overlap(vsegment a,vsegment b)
/* Returns true if the two segments are part of the same line and overlap.
 * The segments are assumed to go in opposite directions. If a segment
 * has one end but not the other on an edge, but that end is represented
 * as being on the adjacent face, it will fail.
 */
{
  bool ret=false;
  if (sameEdge(a.start,b.start) &&
      sameEdge(a.start,b.end) &&
      sameEdge(a.end,b.start) &&
      sameEdge(a.end,b.end))
  {
    moveToFace(b.start,a.start.face);
    moveToFace(b.end,a.start.face);
    moveToFace(a.end,a.start.face);
    ret=fabs(a.start.diag()-a.end.diag())+fabs(b.start.diag()-b.end.diag())>
        fabs(a.start.diag()-b.end.diag())+fabs(b.start.diag()-a.end.diag());
  }
  ret=ret || (a.start==b.end && b.start==a.end);
  return ret;
}

void g1boundary::transpose(ellipsoid *from,ellipsoid *to)
{
  int i;
  for (i=0;i<bdy.size();i++)
    bdy[i]=::transpose(bdy[i],from,to);
}

void gboundary::push_back(g1boundary g1)
{
  bdy.push_back(g1);
}

g1boundary gboundary::operator[](int n)
{
  return bdy[n];
}

polyarc gboundary::getFlatBdy(int n)
{
  return flatBdy[n];
}

int gboundary::size() const
{
  return bdy.size();
}

int gboundary::totalSegments()
{
  int i,total;
  for (i=total=0;i<bdy.size();i++)
    total+=bdy[i].size();
  return total;
}

vsegment gboundary::seg(int n)
{
  vsegment ret;
  int i;
  for (i=0;i<bdy.size() && n>=0;i++)
  {
    if (n>=0 && n<bdy[i].size())
      ret=bdy[i].seg(n);
    n-=bdy[i].size();
  }
  return ret;
}

vsegment gboundary::someSeg()
// Returns a different segment each time; eventually returns all segments.
{
  int t=totalSegments();
  if (t)
  {
    segNum=(segNum+relprime(t))%t;
    if (segNum<0)
      segNum+=t;
  }
  return seg(segNum);
}

xyz gboundary::nearPoint()
/* Picks an arbitrary segment, then rotates one end around the middle
 * by a random angle.
 */
{
  vsegment aseg=someSeg();
  xyz start,mid,end;
  start=decodedir(aseg.start);
  end=decodedir(aseg.end);
  mid=start+end;
  return versor(mid,rng.usrandom()*32768+20252).rotate(end);
}

void gboundary::clear()
{
  bdy.clear();
}

void gboundary::setInner(int n,bool i)
{
  bdy[n].setInner(i);
}

void gboundary::consolidate(int l)
{
  int i=0,j=1,m,n,m0,n0,sz=bdy.size(),cnt=1;
  vector<int> iseg,jseg;
  bool found;
  while (cnt<sqr(sz))
  {
    iseg=bdy[i].segmentsAtLevel(l);
    jseg=bdy[j].segmentsAtLevel(l);
    found=false;
    for (m=0;m<iseg.size();m++)
      for (n=0;n<jseg.size();n++)
        if (overlap(bdy[i].seg(iseg[m]),bdy[j].seg(jseg[n])))
        {
          found=true;
          m0=m;
          n0=n;
        }
    if (found)
    {
      cnt=0;
      bdy[i].splice(iseg[m0],bdy[j],jseg[n0]);
    }
    else
      cnt++;
    j=(j+1)%sz;
    if (i==j)
      i=(i+sz-1)%sz;
  }
}

void gboundary::splitoff(int l)
{
  int i,j,k;
  bool found;
  vector<int> iseg;
  //if (l<0)
    //cout<<"splitoff: l<0"<<endl;
  for (i=0;i<bdy.size();i++)
  {
    do
    {
      iseg=bdy[i].segmentsAtLevel(l);
      found=false;
      for (j=0;j<iseg.size();j++)
      {
        for (k=0;k<j;k++)
          if (overlap(bdy[i].seg(iseg[j]),bdy[i].seg(iseg[k])))
          {
            found=true;
            break;
          }
        if (found)
          break;
      }
      if (found)
      {
        bdy.resize(bdy.size()+1);
        bdy[i].split(iseg[j],iseg[k],bdy.back());
      }
    } while (found);
  }
}

void gboundary::deleteCollinear()
/* Do this after consolidate and splitoff. At level 0, it can leave the
 * boundary in a state where sameEdge incorrectly returns false, so overlapping
 * segments aren't recognized.
 */
{
  int i;
  for (i=0;i<bdy.size();i++)
    bdy[i].deleteCollinear();
}

void gboundary::deleteRetrace()
/* For cylinterval boundaries with area 0 or 510 (full).
 */
{
  int i;
  for (i=0;i<bdy.size();i++)
    bdy[i].deleteRetrace();
}

void gboundary::deleteNullSegments()
{
  int i;
  vector<int> iseg;
  g1boundary tmp;
  for (i=0;i<bdy.size();i++)
  {
    while (true)
    {
      iseg=bdy[i].nullSegments();
      if (!iseg.size())
        break;
      bdy[i].split(iseg[0]+1,iseg[0],tmp);
    }
  }
}

void gboundary::deleteEmpty()
/* Do this after deleteCollinear.
 */
{
  int i,j;
  for (i=0,j=size()-1;i<=j;)
  {
    while (i<size() && bdy[i].size()>0)
      i++;
    while (j>=0 && bdy[j].size()==0)
      j--;
    if (i<j)
      swap(bdy[i],bdy[j]);
    //cout<<"i="<<i<<" j="<<j<<endl;
  }
  bdy.resize(i);
}

void gboundary::erase(int n)
// When erasing many g1boundaries, erase them in reverse order.
{
  if (n<bdy.size()-1 && n>=0)
    swap(bdy[n],bdy[bdy.size()-1]);
  if (n<bdy.size() && n>=0)
    bdy.resize(bdy.size()-1);
}

double gboundary::perimeter(bool midpt)
{
  vector<double> perim;
  int i;
  perim.resize(bdy.size());
  for (i=0;i<bdy.size();i++)
    perim[i]=bdy[i].perimeter(midpt);
  return pairwisesum(perim);
}

int gboundary::area()
{
  int i,total;
  for (total=i=0;i<bdy.size();i++)
    total+=bdy[i].area();
  return total;
}

double gboundary::cubeArea()
{
  int i;
  vector<double> total;
  for (i=0;i<bdy.size();i++)
    total.push_back(bdy[i].cubeArea());
  return pairwisesum(total);
}

gboundary operator+(const gboundary &l,const gboundary &r)
{
  gboundary ret;
  int i;
  for (i=0;i<l.size();i++)
    ret.push_back(l.bdy[i]);
  for (i=0;i<r.size();i++)
    ret.push_back(r.bdy[i]);
  return ret;
}

void gboundary::flattenBdy()
/* Project the g1boundaries onto a plane, so that we can tell whether
 * points are inside or outside them. Used in kml.
 */
{
  int i;
  if (flatBdy.size()!=bdy.size())
  {
    flatBdy.clear();
    areaSign.clear();
    for (i=0;i<bdy.size();i++)
    {
      flatBdy.push_back(flatten(bdy[i]));
      areaSign.push_back(signbit(flatBdy.back().area()));
      //cout<<"bdy#"<<i<<" signbit "<<areaSign.back()<<" bdy size "<<bdy[i].size()<<
        //" around origin "<<flatBdy.back().in(xy(0,0))<<endl;
    }
  }
}

unsigned int gboundary::in(xyz pnt)
/* Returns a bit vector telling whether pnt is inside each of the g1boundaries.
 * pnt must be on the spherical earth's surface. The number of g1boundaries
 * must be at most 32, else it will lose information.
 */
{
  int i;
  unsigned int ret=0;
  double bdyin;
  xy pntproj=sphereStereoArabianSea.geocentricToGrid(pnt);
  flattenBdy();
  for (i=0;i<flatBdy.size();i++)
  {
    bdyin=flatBdy[i].in(pntproj)+areaSign[i];
    if (bdyin>0.5)
      ret|=1<<i;
  }
  return ret;
}

unsigned int gboundary::in(latlong pnt)
{
  return in(Sphere.geoc(pnt,0));
}

unsigned int gboundary::in(vball pnt)
{
  return in(decodedir(pnt));
}

void gboundary::transpose(ellipsoid *from,ellipsoid *to)
{
  int i;
  for (i=0;i<bdy.size();i++)
    bdy[i].transpose(from,to);
}

bool gpolyline::isempty()
{
  return pln.size()==0;
}

int gpolyline::size()
{
  return pln.size();
}

void gpolyline::clear()
{
  pln.clear();
}

void gpolyline::push_back(vball v)
{
  pln.push_back(v);
}

vball gpolyline::operator[](int n)
{
  assert(pln.size());
  n%=pln.size();
  if (n<0)
    n+=pln.size();
  return pln[n];
}

vsegment gpolyline::seg(int n)
{
  vsegment ret;
  assert(pln.size()>1);
  n%=(signed)pln.size()-1;
  if (n<0)
    n+=pln.size()-1;
  ret.start=pln[n];
  ret.end=pln[n+1];
  return ret;
}

void gpolyline::transpose(ellipsoid *from,ellipsoid *to)
{
  int i;
  for (i=0;i<pln.size();i++)
    pln[i]=::transpose(pln[i],from,to);
}
