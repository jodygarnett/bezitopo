/******************************************************/
/*                                                    */
/* cogo.cpp - coordinate geometry                     */
/*                                                    */
/******************************************************/

#include "cogo.h"
#include "bezitopo.h"
#include "random.h"
#include <stdarg.h>
#include <stdlib.h>
#include <cmath>
using namespace std;

int debugdel;

char intstable[3][3][3][3]=
/* NOINT  don't intersect
   ACXBD  intersection is in the midst of both AC and BD
   BDTAC  one end of BD is in the midst of AC
   ACTBD  one end of AC is in the midst of BD
   ACVBD  one end of AC is one end of BD
   COINC  A=C or B=D
   COLIN  all four points are collinear
   IMPOS  impossible, probably caused by roundoff error
   */
//  -     -     -     0     0     0     +     +     +   B
//  -     0     +     -     0     +     -     0     +   D   A C
{ ACXBD,BDTAC,NOINT,BDTAC,IMPOS,IMPOS,NOINT,IMPOS,IMPOS, // - -
  ACTBD,ACVBD,NOINT,ACVBD,IMPOS,IMPOS,NOINT,IMPOS,IMPOS, // - 0
  NOINT,NOINT,NOINT,NOINT,COINC,NOINT,NOINT,NOINT,NOINT, // - +
  ACTBD,ACVBD,NOINT,ACVBD,IMPOS,IMPOS,NOINT,IMPOS,IMPOS, // 0 -
  IMPOS,IMPOS,COINC,IMPOS,COLIN,IMPOS,COINC,IMPOS,IMPOS, // 0 0
  IMPOS,IMPOS,NOINT,IMPOS,IMPOS,ACVBD,NOINT,ACVBD,ACTBD, // 0 +
  NOINT,NOINT,NOINT,NOINT,COINC,NOINT,NOINT,NOINT,NOINT, // + -
  IMPOS,IMPOS,NOINT,IMPOS,IMPOS,ACVBD,NOINT,ACVBD,ACTBD, // + 0
  IMPOS,IMPOS,NOINT,IMPOS,IMPOS,BDTAC,NOINT,BDTAC,ACXBD  // + +
  };

double area3(xy a,xy b,xy c)
{int i,j;
 double surface,area[COLIN];
 bool cont;
 area[0]=a.east()*b.north();
 area[1]=-b.east()*a.north();
 area[2]=b.east()*c.north();
 area[3]=-c.east()*b.north();
 area[4]=c.east()*a.north();
 area[5]=-a.east()*c.north();
 do {cont=false; // Sort the six areas into absolute value order for numerical stability.
     for (i=0;i<5;i++)
         if (fabs(area[i+1])<fabs(area[i]))
            {surface=area[i];
             area[i]=area[i+1];
             area[i+1]=surface;
             cont=true;
             }
     } while (cont);
 for (j=5;j>0;j-=2) // Make signs of equal-absolute-value areas alternate.
   for (i=0;i+j<6;i++)
     if (area[i]+area[i+j]==0 && (area[i]<0 ^ (i&1)))
     {
       area[i]=-area[i];
       area[i+j]=-area[i+j];
     }
 for (surface=i=0;i<6;i++)
     surface+=area[i];
 surface/=2;
 return surface;
 }

xy intersection (xy a,xy c,xy b,xy d)
//Intersection of lines ac and bd.
{double A,B,C,D;
 A=area3(b,c,d);
 B=area3(c,d,a);
 C=area3(d,a,b);
 D=area3(a,b,c);
 return ((a*A+c*C)+(b*B+d*D))/((A+C)+(B+D));
 }

#define setmaxabs(a,b) a=(fabs(b)>a)?fabs(b):a

int intstype (xy a,xy c,xy b,xy d,double &maxarea,double &maxcoord)
//Intersection type - one of 81 numbers, not all possible.
{double A,B,C,D;
 A=area3(b,c,d);
 B=area3(c,d,a);
 C=area3(d,a,b);
 D=area3(a,b,c);
 maxarea=maxcoord=0;
 setmaxabs(maxarea,A);
 setmaxabs(maxarea,B);
 setmaxabs(maxarea,C);
 setmaxabs(maxarea,B);
 setmaxabs(maxcoord,a.east());
 setmaxabs(maxcoord,a.north());
 setmaxabs(maxcoord,b.east());
 setmaxabs(maxcoord,b.north());
 setmaxabs(maxcoord,c.east());
 setmaxabs(maxcoord,c.north());
 setmaxabs(maxcoord,d.east());
 setmaxabs(maxcoord,d.north());
 return (27*sign(A)+9*sign(C)+3*sign(B)+sign(D));
 }

inttype intersection_type(xy a,xy c,xy b,xy d)
{
  double maxarea,maxcoord;
  int itype=intstype(a,c,b,d,maxarea,maxcoord)+40;
  itype=intstable[itype/27][itype%27/9][itype%9/3][itype%3];
  if (itype==IMPOS && maxarea<maxcoord*maxcoord*1e-15)
    itype=COLIN;
  return (inttype)itype;
}

double pldist(xy a,xy b,xy c)
/* Signed distance from a to the line bc. */
{return area3(a,b,c)/dist(b,c)*2;
 }

xy rand2p(xy a,xy b)
/* A random point in the circle with diameter ab. */
{xy mid((a+b)/2);
 unsigned short n;
 xy pnt;
 double angle=(sqrt(5)-1)*M_PI;
 n=rng.usrandom();
 pnt=xy(cos(angle*n)*sqrt(n+0.5)/256,sin(angle*n)*sqrt(n+0.5)/256);
 pnt=pnt*dist(mid,a)+mid;
 return pnt;
 }

bool delaunay(xy a,xy c,xy b,xy d)
//Returns true if ac satisfies the criterion in the quadrilateral abcd.
//If false, the edge should be flipped to bd.
//The computation is based on the theorem that the two diagonals of
//a quadrilateral inscribed in a circle cut each other into parts
//whose products are equal.
{xy ints;
 double dista,distc,distb,distd,distac,distbd;
 ints=intersection(a,c,b,d);
 distac=dist(a,c);
 distbd=dist(b,d);
 if (std::isnan(ints.north()))
    {//printf("delaunay:No intersection, distac=%a, distbd=%a\n",distac,distbd);
     return distac<=distbd;
     }
 else
    {dista=dist(a,ints);
     distb=dist(b,ints);
     distc=dist(c,ints);
     distd=dist(d,ints);
     if (dista>distac || distc>distac) dista=-dista;
     if (distb>distbd || distd>distbd) distb=-distb;
     if (debugdel && dista*distc>distb*distd)
        printf("delaunay:dista*distc=%a, distb*distd=%a\n",dista*distc,distb*distd);
     if (dista*distc == distb*distd)
        return distac<=distbd;
     else
        return dista*distc<=distb*distd;
     }
 }
