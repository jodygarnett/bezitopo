/******************************************************/
/*                                                    */
/* convertgeoid.cpp - convert geoidal undulation data */
/*                                                    */
/******************************************************/
#include <iostream>
#include "geoid.h"
#include "sourcegeoid.h"
#include "document.h"
#include "raster.h"
#include "hlattice.h"
#include "relprime.h"
using namespace std;

document doc;
cubemap cube;

/* The factors used when setting the six components of a geoquad are
 * 0: 1/1
 * 1: 256/85
 * 2: 256/85
 * 3: 65536/12937
 * 4: 65536/7225
 * 5: 65536/12937
 */

/* Check the square for the presence of geoid data by interrogating it with a
 * hexagonal lattice. The size of the hexagon is sqrt(2/3) times the length
 * of the square (sqrt(1/2) to get the half diagonal of the square, sqrt(4/3)
 * to get the radius from the apothem), except for the whole face, where it
 * is (1+sqrt(1/3))/2 times the length of the square, since two sides of the
 * hexagon are parallel to two sides of the square. The process continues
 * until the entire square has been interrogated or there are at least one
 * point in nan and one point in num.
 * 
 * This procedure doesn't return anything. Use geoquad::isfull. It is possible
 * that interrogating finds a square full, but one of the 256 points used to
 * compute the coefficients is NaN.
 */
void interroquad(geoquad &quad,double spacing)
{
  xyz corner(3678298.565,3678298.565,3678298.565),ctr,xvec,yvec,tmp,pt;
  vball v;
  hvec h;
  int radius,i,n,rp;
  double qlen,hradius;
  ctr=quad.centeronearth();
  xvec=corner*ctr;
  yvec=xvec*ctr;
  xvec/=xvec.length();
  yvec/=yvec.length();
  tmp=(2+M_SQRT_3)*yvec+xvec;
  xvec-=yvec;
  yvec=tmp/tmp.length();
  xvec/=xvec.length();
  // xvec and yvec are now at 120° to match the components of hvec
  qlen=quad.length();
  if (qlen>1e7)
    hradius=qlen*(1+M_SQRT_1_3)/2;
  else
    hradius=qlen*M_SQRT_2_3;
  if (spacing<1)
    spacing=1;
  radius=rint(hradius/spacing);
  if (radius>26754)
  {
    radius=26754; // largest hexagon with <2147483648 points
    spacing=hradius/radius;
  }
  hlattice hlat(radius);
  xvec*=spacing;
  yvec*=spacing;
  rp=relprime(hlat.nelts);
  for (i=n=0;i<hlat.nelts && !(quad.nums.size() && quad.nans.size());i++)
  {
    h=hlat.nthhvec(n);
    v=encodedir(ctr+h.getx()*xvec+h.gety()*yvec);
    pt=decodedir(v);
    if (quad.in(v))
    {
      if (std::isfinite(avgelev(pt)))
	quad.nums.push_back(v.getxy());
      else
	quad.nans.push_back(v.getxy());
    }
    n-=rp;
    if (n<0)
      n+=hlat.nelts;
  }
}

void refine(geoquad &quad,double tolerance,double sublimit,double spacing)
{
  int i;
  double area;
  area=quad.apxarea();
  //cout<<"Area: exact "<<quad.area()<<" approx "<<area<<" ratio "<<quad.area()/area<<endl;
  if (area>=sqr(sublimit))
  {
    if (quad.nans.size()+quad.nums.size()==0 || (quad.isfull() && area/(quad.nans.size()+quad.nums.size())>sqr(spacing)))
      interroquad(quad,spacing);
    if (quad.isfull()==0)
    {
      quad.subdivide();
      for (i=0;i<4;i++)
	refine(*quad.sub[i],tolerance,sublimit,spacing);
    }
  }
}

void outund(string loc,int lat,int lon)
{
  int i;
  cout<<"Undulation in "<<loc<<" is"<<endl;
  for (i=0;i<geo.size();i++)
    cout<<i<<": "<<geo[i].elev(lat,lon)<<endl;
}

int main(int argc, char *argv[])
{
  int i;
  geo.resize(6);
  readusngsbin(geo[0],"../g2012bu0.bin");
  readusngsbin(geo[1],"../g2012ba0.bin");
  readusngsbin(geo[2],"../g2012bh0.bin");
  readusngsbin(geo[3],"../g2012bg0.bin");
  readusngsbin(geo[4],"../g2012bp0.bin");
  readusngsbin(geo[5],"../g2012bs0.bin");
  outund("Green Hill",degtobin(35.4),degtobin(-82.05));
  outund("Charlotte",degtobin(35.22),degtobin(-80.84));
  outund("Kitimat",degtobin(54.0547),degtobin(-128.6578)); // in the overlap of two files
  outund("Denali",degtobin(63.0695),degtobin(-151.0074));
  outund("Haleakala",degtobin(20.7097),degtobin(-156.2533));
  drawglobecube(1024,62,-7,1,0,"geoid.ppm");
  for (i=0;i<6;i++)
  {
    cout<<"Face "<<i+1;
    cout.flush();
    interroquad(cube.faces[i],1e5);
    if (cube.faces[i].isfull()>=0)
      cout<<" has data"<<endl;
    else
      cout<<" is empty"<<endl;
    refine(cube.faces[i],0.01,1e5,1e5);
  }
  return 0;
}
