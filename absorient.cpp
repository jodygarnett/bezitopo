/******************************************************/
/*                                                    */
/* absorient.cpp - 2D absolute orientation            */
/*                                                    */
/******************************************************/
/* Copyright 2015,2018 Pierre Abbat.
 * This file is part of Bezitopo.
 * 
 * Bezitopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bezitopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bezitopo. If not, see <http://www.gnu.org/licenses/>.
 */
/* Given two pointlists and a list of matching points,
 * find the rotation and translation to match them with the
 * least sum of square distances. This is called the
 * absolute orientation problem.
 */
#include <assert.h>
#include "absorient.h"
#include "except.h"
#include "manysum.h"
using namespace std;

double sumsqdist(vector<xy> a,vector<xy> b)
{
  int i;
  vector<double> dists;
  assert(a.size()==b.size());
  for (i=0;i<a.size();i++)
    dists.push_back(sqr(dist(a[i],b[i])));
  return pairwisesum(dists);
}

xy pointCentroid(vector<xy> a)
{
  int i;
  vector<double> x,y;
  for (i=0;i<a.size();i++)
  {
    x.push_back(a[i].getx());
    y.push_back(a[i].gety());
  }
  return xy(pairwisesum(x)/i,pairwisesum(y)/i);
}

RoscatStruct absorient(vector<xy> a,vector<xy> b)
// Returns the way to rotate, scale, and translate a to best match b.
{
  int i;
  vector<xy> aslide,bslide,arot;
  RoscatStruct ret;
  if (a.size()<2 || b.size()<2)
    throw(badAbsOrient);
  ret.tfrom=pointCentroid(a);
  ret.tto=pointCentroid(b);
  for (i=0;i<a.size();i++)
    aslide.push_back(a[i]-ret.tfrom);
  for (i=0;i<b.size();i++)
    bslide.push_back(b[i]-ret.tto);
  ret.ro=0;
  ret.sca=1;
  return ret;
}

RoscatStruct absorient(pointlist &a,vector<int> ai,pointlist &b,vector<int> bi)
{
  vector<xy> axy,bxy;
  int i;
  for (i=0;i<ai.size();i++)
    axy.push_back(a.points[ai[i]]);
  for (i=0;i<bi.size();i++)
    bxy.push_back(b.points[bi[i]]);
  return absorient(axy,bxy);
}
