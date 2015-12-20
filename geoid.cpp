/******************************************************/
/*                                                    */
/* geoid.cpp - geoidal undulation                     */
/*                                                    */
/******************************************************/
#include <cmath>
#include "geoid.h"
#include "angle.h"
using namespace std;

/* face=0: point is the center of the earth
 * face=1: in the Benin face; x=+y, y=+z
 * face=2: in the Bengal face; x=+z, y=+x
 * face=3: in the Arctic face; x=+x, y=+y
 * face=4: in the Antarctic face; x=+x, y=-y
 * face=5: in the Galápagos face; x=+z, y=-x
 * face=6: in the Howland face; x=+y, y=-z
 * face=7: a coordinate is NaN or at least two are infinite
 */
/* Format of Bezitopo's geoid files:
 * Start Len
 * 0000 0008 literal string "boldatni"
 * 0008 0008 hash identifier of this geoid file
 * 0008 0002 0000 file refers to the earth (other planets/moons have different
 *           sizes, so the limit of subdivision and smallest island are
 *           relatively different)
 * 0012 0001 00 type of data is geoidal undulation (others are not defined but
 *           include deflection of vertical or variation of gravity)
 * 0013 0001 01 encoding (00 is 4-byte big endian, 01 is variable length)
 * 0014 0001 01 data are scalar (order of data if there are more components
 *           is not yet defined)
 * 0015 0002 fff0 scale factor as binary exponent is -16, one ulp is 1/65536 m
 * 0017 0008 tolerance of conversion
 * 001f 0008 limit of subdivision. If a geoquad is partly NaN and partly number,
 *           it will not be subdivided if it's smaller than this.
 * 0027 0008 smallest island or lacuna of data that won't be missed
 * 002f 0002 number of source files × 2
 * 0031 vary names of source files alternating with names of formats, each
 *           null-terminated
 * vary vary six quadtrees of geoquads
 * 
 * Quadtrees look like this:
 * An empty face of the earth:
 * 00 8000
 * A face with just one geoquad:
 * 00 1e0943 fff382 002583 01ba38 000302 fffeed
 * Three quarters undivided, the upper right subdivided in quarters, all NaN:
 * 01 8000 00 8000 00 8000 01 8000 00 8000 00 8000 00 8000
 */

vball::vball()
{
  face=0;
  x=y=0;
}

vball::vball(int f,xy p)
{
  face=f;
  x=p.getx();
  y=p.gety();
}

xy vball::getxy()
{
  return xy(x,y);
}

vball encodedir(xyz dir)
{
  vball ret;
  double absx,absy,absz;
  absx=fabs(dir.getx());
  absy=fabs(dir.gety());
  absz=fabs(dir.getz());
  if (absx==0 && absy==0 && absz==0)
  {
    ret.face=0;
    ret.x=ret.y=0;
  }
  else if (std::isnan(absx) || std::isnan(absy) || std::isnan(absz) ||
           ((std::isinf(absx)+std::isinf(absy)+std::isinf(absz))>1))
  {
    ret.face=7;
    ret.x=ret.y=NAN;
  }
  else
  {
    if (absx>=absy && absx>=absz)
    {
      ret.face=1;
      ret.x=dir.gety()/absx;
      ret.y=dir.getz()/dir.getx();
      if (dir.getx()<0)
	ret.face=6;
    }
    if (absy>=absz && absy>=absx)
    {
      ret.face=2;
      ret.x=dir.getz()/absy;
      ret.y=dir.getx()/dir.gety();
      if (dir.gety()<0)
	ret.face=5;
    }
    if (absz>=absx && absz>=absy)
    {
      ret.face=3;
      ret.x=dir.getx()/absz;
      ret.y=dir.gety()/dir.getz();
      if (dir.getz()<0)
	ret.face=4;
    }
  }
  return ret;
}

xyz decodedir(vball code)
{
  xyz ret;
  switch (code.face&7)
  {
    case 0:
      ret=xyz(0,0,0);
      break;
    case 1:
      ret=xyz(1,code.x,code.y);
      break;
    case 2:
      ret=xyz(code.y,1,code.x);
      break;
    case 3:
      ret=xyz(code.x,code.y,1);
      break;
    case 4:
      ret=xyz(code.x,-code.y,-1);
      break;
    case 5:
      ret=xyz(-code.y,-1,code.x);
      break;
    case 6:
      ret=xyz(-1,code.x,-code.y);
      break;
    case 7:
      ret=xyz(NAN,NAN,NAN);
      break;
  }
  if ((code.face&7)%7)
    ret=ret*(6371e3/ret.length());
  return ret;
}

bool geoquad::subdivided()
/* Unlike qindex, this is architecture-dependent.
 * On 64-bit Intel/AMD architecture, pointer is 8 bytes and int is 4.
 * SSSSSSSSssssssssSSSSSSSSssssssss
 * UUUUuuuuUUUUuuuuUUUUuuuu
 * The last sub is NULL if it is not subdivided and points to the upper right
 * quadrant if it is.
 * If int is 8 bytes (which will require rewriting writegeint):
 * SSSSSSSSssssssssSSSSSSSSssssssss
 * UUUUUUUUuuuuuuuuUUUUUUUUuuuuuuuuUUUUUUUUuuuuuuuu
 * The last und is set to 0x8000000000000000 (which means NAN) if it is subdivided
 * and some small value if it isn't. To indicate that the undulation is unknown,
 * set the first und, but not the last, to NAN.
 * If int is 4 bytes and pointers are 6 bytes, this won't work.
 */
{
  if (sizeof(geoquad *)*3>=sizeof(int)*6)
    return sub[3]!=nullptr;
  if (sizeof(int)*5>=sizeof(geoquad *)*4)
    return und[5]>8850*256 || und[5]<-11000*256;
}

bool geoquad::isnan()
{
  return und[0]>8850*256 || und[0]<-11000*256;
}

geoquad::geoquad()
{
  int i;
  for (i=0;i<4;i++)
    sub[i]=nullptr;
  for (i=1;i<6;i++)
    und[i]=0;
  und[0]=0x80000000;
  scale=1;
}

geoquad::~geoquad()
{
  int i;
  if (subdivided())
    for (i=0;i<4;i++)
      delete(sub[i]);
}

void geoquad::clear()
{
  int i;
  if (subdivided())
    for (i=0;i<4;i++)
      delete(sub[i]);
  for (i=0;i<4;i++)
    sub[i]=nullptr;
  for (i=1;i<6;i++)
    und[i]=0;
  und[0]=0x80000000;
}

vball geoquad::vcenter()
{
  return vball(face,center);
}

void geoquad::subdivide()
/* This makes no attempt to subdivide the surface.
 * The four subsquares are initialized to NAN.
 */
{
  int i,j;
  und[5]=0x80000000;
  for (i=0;i<4;i++)
  {
    sub[i]=new(geoquad);
    sub[i]->scale=scale/2;
    sub[i]->face=face;
    sub[i]->center=xy(center.east()+scale/((i&1)?2:-2),center.north()+scale/((i&2)?2:-2));
    for (j=0;j<nans.size();j++)
      if (sub[i]->in(nans[j]))
	sub[i]->nans.push_back(nans[j]);
    for (j=0;j<nums.size();j++)
      if (sub[i]->in(nums[j]))
	sub[i]->nums.push_back(nums[j]);
  }
  nans.clear();
  nums.clear();
}

bool geoquad::in(xy pnt)
{
  return fabs(pnt.east()-center.east())<=scale && fabs(pnt.north()-center.north())<=scale;
}

bool geoquad::in(vball pnt)
{
  return face==pnt.face && in(xy(pnt.x,pnt.y));
}

double geoquad::undulation(double x,double y)
{
  int xbit,ybit;
  double u;
  if (subdivided())
  {
    xbit=x>=0;
    ybit=y>=0;
    x=2*(x-(xbit-0.5));
    y=2*(y-(ybit-0.5));
    u=sub[(ybit<<1)|xbit]->undulation(x,y);
  }
  else
  {
    u=(und[0]+und[1]*x+und[2]*y+und[3]*x*x+und[4]*x*y+und[5]*y*y)/65536;
    if (u>8850 || u<-11000)
      u=NAN;
  }
  return u;
}

xyz geoquad::centeronearth()
{
  return decodedir(vball(face,center));
}

double geoquad::length()
{
  double r;
  r=xyz(center,1).length();
  return 6371e3*2*scale/r;
}

double geoquad::width()
{
  double r;
  r=xyz(center,1).length();
  return 6371e3*2*scale/sqr(r);
}

double geoquad::apxarea()
/* apxarea is 6/π (1.9099) times as big as area for a whole face;
 * for a quarter face it is 4% too big;
 * for anything else it is within 1%.
 */
{
  return length()*width();
}

double anglexs(xy pnt)
{
  return asin(pnt.getx()/sqrt(sqr(pnt.getx())+1)*pnt.gety()/sqrt(sqr(pnt.gety())+1));
}

double geoquad::area()
{
  xy ne(scale,scale),nw(-scale,scale);
  return ((anglexs(center+ne)+anglexs(center-ne))-(anglexs(center+nw)+anglexs(center-nw)))*4.0589641e13;
}

int geoquad::isfull()
/* Returns -1 if the square has been interrogated and all points found to have no geoid data.
 * Returns 0 if some points have geoid data and some do not, or if no points have been tested.
 * Returns 1 if all points tested have geoid data.
 */
{
  return (nums.size()>0)-(nans.size()>0);
}

unsigned byteswap(unsigned n)
{
  return ((n&0xff000000)>>24)|((n&0xff0000)>>8)|((n&0xff00)<<8)|((n&0xff)<<24);
}

array<unsigned,2> geoquad::hash()
{
  array<unsigned,2> ret,subhash;
  array<unsigned,8> subhashes;
  int i;
  if (subdivided())
  {
    for (i=0;i<4;i++)
    {
      subhash=sub[i]->hash();
      subhashes[2*i]=subhash[0];
      subhashes[2*i+1]=subhash[1];
    }
    for (i=ret[0]=ret[1]=0;i<8;i++)
    {
      ret[0]=byteswap((ret[0]^subhashes[i])*1657);
      ret[1]=byteswap((ret[1]^subhashes[7-i])*6371);
    }
  }
  else
    for (i=ret[0]=ret[1]=0;i<6;i++)
    {
      ret[0]=byteswap((ret[0]^und[i])*99421);
      ret[1]=byteswap((ret[1]^und[5-i])*47935);
    }
  return ret;
}

cubemap::cubemap()
{
  int i;
  for (i=0;i<6;i++)
    faces[i].face=i+1;
}