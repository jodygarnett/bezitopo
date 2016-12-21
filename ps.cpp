/******************************************************/
/*                                                    */
/* ps.cpp - PostScript output                         */
/*                                                    */
/******************************************************/
/* Copyright 2012,2013,2014,2015,2016 Pierre Abbat.
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
 * 
 * Literal PostScript code in this file, which is written to Bezitopo's
 * output, is in the public domain.
 */
#include <cstdio>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>
#include <iomanip>
#include <unistd.h>
#include "ldecimal.h"
#include "config.h"
#include "ps.h"
#include "point.h"
#include "tin.h"
#include "pointlist.h"
#include "plot.h"
#include "document.h"
using namespace std;

char rscales[]={10,12,15,20,25,30,40,50,60,80};
const double PSPoint=25.4/72;
map<string,papersize> papersizes=
/* These mean the physical orientation of the paper in the printer. If you
 * want to print in landscape, but the paper is portrait in the printer,
 * set pageorientation to 1.
 */
{
  {"A4 portrait",{210000,297000}},
  {"A4 landscape",{297000,210000}},
  {"US Letter portrait",{215900,279400}},
  {"US Letter landscape",{279400,215900}},
  {"US Legal portrait",{215900,355600}},
  {"US Legal landscape",{355600,215900}}
};

int fibmod3(int n)
{
  int i,a,b;
  for (i=a=0,b=1;a<n;i++)
  {
    b+=a;
    a=b-a;
  }
  return (a==n)?(i%3):-1;
}

PostScript::PostScript()
{
  oldr=oldg=oldb=NAN;
  paper=xy(210,297);
  scale=1;
  orientation=pages=0;
  indocument=inpage=false;
  psfile=nullptr;
}

PostScript::~PostScript()
{
  if (psfile)
    close();
}

void PostScript::setpaper(papersize pap,int ori)
/* ori is 0 for no rotation, 1 for 90° rotation, making portrait
 * into landscape and vice versa. Do this before each page,
 * or before calling prolog if all pages are the same size.
 */
{
  paper=xy(pap.width/1e3,pap.height/1e3);
  pageorientation=ori;
}

void PostScript::open(string psfname)
{
  if (psfile)
    close();
  psfile=new ofstream(psfname);
}

void PostScript::prolog()
{
  if (psfile && !indocument)
  {
    *psfile<<"%!PS-Adobe-3.0\n%%BeginProlog\n%%%%Pages: (atend)"<<endl;
    *psfile<<"%%BoundingBox: 0 0 "<<rint(paper.getx()/PSPoint)<<' '<<rint(paper.gety()/PSPoint)<<endl;
    *psfile<<"\n/. % ( x y )\n{ newpath 0.1 0 360 arc fill } bind def\n\n";
    *psfile<<"/- % ( x1 y1 x2 y2 )\n\n{ newpath moveto lineto stroke } bind def\n\n";
    *psfile<<"/mmscale { 720 254 div dup scale } bind def\n";
    *psfile<<"%%EndProlog"<<endl;
    indocument=true;
    pages=0;
  }
}

void PostScript::startpage()
{
  if (psfile && indocument && !inpage)
  {
    ++pages;
    *psfile<<"%%Page: "<<pages<<' '<<pages<<"\ngsave mmscale 0.1 setlinewidth\n";
    *psfile<<paper.getx()/2<<' '<<paper.gety()/2<<' '<<"translate ";
    *psfile<<(pageorientation&3)*90<<" rotate ";
    *psfile<<paper.getx()/-2<<' '<<paper.gety()/-2<<' '<<"translate"<<endl;
    *psfile<<"/Helvetica findfont 3 scalefont setfont"<<endl;
    oldr=oldg=oldb=NAN;
    inpage=true;
  }
}

void PostScript::endpage()
{
  if (psfile && indocument && inpage)
  {
    *psfile<<"grestore showpage"<<endl;
    inpage=false;
  }
}

void PostScript::trailer()
{
  if (inpage)
    endpage();
  if (psfile && indocument)
  {
    *psfile<<"%%BeginTrailer\n%%Pages: "<<pages<<"\n%%EndTrailer"<<endl;
    indocument=false;
  }
}

void PostScript::close()
{
  if (indocument)
    trailer();
  delete(psfile);
  psfile=nullptr;
}

void PostScript::setDoc(document &docu)
{
  doc=&docu;
}

double PostScript::xscale(double x)
{
  return scale*(x-modelcenter.east())+paper.getx()/2;
}

double PostScript::yscale(double y)
{
  return scale*(y-modelcenter.north())+paper.gety()/2;
}

void PostScript::setcolor(double r,double g,double b)
{
  if (r!=oldr || g!=oldg || b!=oldb)
  {
    *psfile<<fixed<<setprecision(3)<<r<<' '<<g<<' '<<b<<" setrgbcolor"<<endl;
    oldr=r;
    oldg=g;
    oldb=b;
  }
}

void PostScript::setscale(double minx,double miny,double maxx,double maxy,int ori)
/* To compute minx etc. using dirbound on e.g. a pointlist pl:
 * minx=pl.dirbound(-ori);
 * miny=pl.dirbound(DEG90-ori);
 * maxx=-pl.dirbound(DEG180-ori);
 * maxy=-pl.dirbound(DEG270-ori);
 */
{
  double xsize,ysize;
  int i;
  orientation=ori;
  modelcenter=xy(minx+maxx,miny+maxy)/2;
  xsize=fabs(minx-maxx);
  ysize=fabs(miny-maxy);
  for (scale=1;scale*xsize/10<paper.east() && scale*ysize/10<paper.north();scale*=10);
  for (;scale*xsize/80>paper.east()*0.9 || scale*ysize/80>paper.north()*0.9;scale/=10);
  for (i=0;i<9 && (scale*xsize/rscales[i]>paper.east()*0.9 || scale*ysize/rscales[i]>paper.north()*0.9);i++);
  scale/=rscales[i];
  *psfile<<"% minx="<<minx<<" miny="<<miny<<" maxx="<<maxx<<" maxy="<<maxy<<" scale="<<scale<<endl;
}

void PostScript::dot(xy pnt,string comment)
{
  assert(psfile);
  pnt=turn(pnt,orientation);
  if (isfinite(pnt.east()) && isfinite(pnt.north()))
  {
    *psfile<<fixed<<setprecision(2)<<xscale(pnt.east())<<' '<<yscale(pnt.north())<<" .";
    if (comment.length())
      *psfile<<" %"<<comment;
    *psfile<<endl;
  }
}

void PostScript::circle(xy pnt,double radius)
{
  pnt=turn(pnt,orientation);
  *psfile<<fixed<<setprecision(2)<<xscale(pnt.east())<<yscale(pnt.north())
  <<" newpath "<<scale*radius<<" 0 360 arc fill %"<<radius*radius<<endl;
}

void PostScript::line(edge lin,int num,bool colorfibaster,bool directed)
/* Used in bezitest to show the 2D TIN before the 3D triangles are constructed on it.
 * In bezitopo, use spline(lin.getsegment().approx3d(x)) to show it in 3D.
 */
{
  xy mid,disp,base,ab1,ab2,a,b;
  char *rgb;
  a=*lin.a;
  b=*lin.b;
  a=turn(a,orientation);
  b=turn(b,orientation);
  if (lin.delaunay())
    if (colorfibaster)
      switch (fibmod3(abs(doc->pl[1].revpoints[lin.a]-doc->pl[1].revpoints[lin.b])))
      {
	case -1:
	  setcolor(0.3,0.3,0.3);
	  break;
	case 0:
	  setcolor(1,0.3,0.3);
	  break;
	case 1:
	  setcolor(0,1,0);
	  break;
	case 2:
	  setcolor(0.3,0.3,1);
	  break;
      }
    else
      setcolor(0,0,1);
  else
    setcolor(0,0,0);
  if (directed)
  {
    disp=b-a;
    base=xy(disp.north()/40,disp.east()/-40);
    ab1=a+base;
    ab2=a-base;
    *psfile<<"newpath "<<xscale(b.east())<<' '<<yscale(b.north())<<" moveto "<<xscale(ab1.east())<<' '<<yscale(ab1.north())<<" lineto "<<xscale(ab2.east())<<' '<<yscale(ab2.north())<<" lineto closepath fill"<<endl;
  }
  else
    *psfile<<xscale(a.east())<<' '<<yscale(a.north())<<' '<<xscale(b.east())<<' '<<yscale(b.north())<<" -"<<endl;
  mid=(a+b)/2;
  //fprintf(psfile,"%7.3f %7.3f moveto (%d) show\n",xscale(mid.east()),yscale(mid.north()),num);
}

void PostScript::line2p(xy pnt1,xy pnt2)
{
  pnt1=turn(pnt1,orientation);
  pnt2=turn(pnt2,orientation);
  if (isfinite(pnt1.east()) && isfinite(pnt1.north()) && isfinite(pnt2.east()) && isfinite(pnt2.north()))
    *psfile<<fixed<<setprecision(2)<<xscale(pnt1.east())<<' '<<yscale(pnt1.north())
    <<' '<<xscale(pnt2.east())<<' '<<yscale(pnt2.north())<<" -"<<endl;
}

void PostScript::spline(bezier3d spl)
{
  int i,j,n;
  vector<xyz> seg;
  xy pnt;
  n=spl.size();
  pnt=turn(xy(spl[0][0]),orientation);
  *psfile<<fixed<<setprecision(2)<<xscale(pnt.east())<<' '<<yscale(pnt.north())<<" moveto\n";
  for (i=0;i<n;i++)
  {
    seg=spl[i];
    for (j=1;j<4;j++)
    {
      pnt=turn(xy(seg[j]),orientation);
      if (pnt.isnan())
	cerr<<"NaN point"<<endl;
      *psfile<<xscale(pnt.east())<<' '<<yscale(pnt.north())<<' ';
    }
    *psfile<<"curveto\n";
  }
  *psfile<<"stroke"<<endl;
}

void PostScript::widen(double factor)
{
  *psfile<<"currentlinewidth "<<ldecimal(factor)<<" mul setlinewidth"<<endl;
}

void PostScript::write(xy pnt,string text)
{
  pnt=turn(pnt,orientation);
  *psfile<<fixed<<setprecision(2)<<xscale(pnt.east())<<' '<<yscale(pnt.north())
  <<" moveto ("<<text<<") show"<<endl;
}

void PostScript::comment(string text)
{
  *psfile<<'%'<<text<<endl;
}
