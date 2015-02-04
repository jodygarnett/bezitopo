/******************************************************/
/*                                                    */
/* spiral.cpp - Cornu or Euler spirals                */
/*                                                    */
/******************************************************/

/* The Cornu spiral is a complex-valued function of a real argument,
 * the graph in the complex plane of integral(cis(t²) dt).
 * The curvature at t equals 2t.
 * Evaluating the function in its curly tails should not be necessary
 * in surveying, but if it is, use the three-argument
 * cornu() function.
 */

#include <vector>
#include <cmath>
#include <cstdio>
#include <iostream>
#include "spiral.h"
#include "angle.h"
using namespace std;

xy cornu(double t)
/* If |t|>=6, it returns the limit points rather than a value with no precision.
 * The largest t useful in surveying is 1.430067.
 */
{
  vector<long double> realparts,imagparts;
  int i;
  long double facpower,rsum,isum,t2,bigpart;
  double precision;
  t2=t*t;
  for (i=0,facpower=t;0.9+facpower!=0.9 || !i;i++)
  {
    realparts.push_back(facpower/(8*i+1));
    facpower*=t2/(4*i+1);
    imagparts.push_back(facpower/(8*i+3));
    facpower*=t2/(4*i+2);
    realparts.push_back(-facpower/(8*i+5));
    facpower*=t2/(4*i+3);
    imagparts.push_back(-facpower/(8*i+7));
    facpower*=t2/(4*i+4);
  }
  for (i=realparts.size()-1,rsum=isum=bigpart=0;i>=0;i--)
  {
    rsum+=realparts[i];
    isum+=imagparts[i];
    if (fabsl(realparts[i])>bigpart)
      bigpart=fabsl(realparts[i]);
    if (fabsl(imagparts[i])>bigpart)
      bigpart=fabsl(imagparts[i]);
  }
  precision=nextafterl(bigpart,2*bigpart)-bigpart;
  //printf("precision %e\n",precision);
  if (precision>1e-6)
    rsum=isum=sqrt(M_PI/8)*(t/fabs(t));
  return xy(rsum,isum);
}

xy cornu(double t,double curvature,double clothance)
/* Evaluates the integral of cis(clothance×t²+curvature×t).
 * 1+(cl×t²+cu×t)i-(cl×t²+cu×t)²/2-(cl×t²+cu×t)³i/6+(cl×t²+cu×t)⁴/24+...
 * 1+cl×t²×i  +cu×t×i   -cl²×t⁴/2  -cl×cu×t³×2/2  -cu²×t²/2  -cl³×t⁶×i/6  -cl²×cu×t⁵×3i/6  -cl×cu²×t⁴×3i/6  -cu³×t³i/6  +cl⁴×t⁸/24  +cl³×cu×t⁷×4/24  +cl²×cu²×t⁶6/24  +cl×cu³×t⁵×4/24  +cu⁴×t⁴/24+...
 * t+cl×t³×i/3+cu×t²×i/2-cl²×t⁵/5/2-cl×cu×t⁴×2/4/2-cu²×t³/3/2-cl³×t⁷×i/7/6-cl³×cu×t⁶×3i/6/6-cl×cu²×t⁵×3i/5/6-cu³×t⁴i/4/6+cl⁴×t⁹/9/24+cl³×cu×t⁸×4/8/24+cl²×cu²×t⁷6/7/24+cl×cu³×t⁶×4/6/24+cu⁴×t⁵/5/24+...
 * If clothance=0, you get a circle of radius 1/curvature.
 * If curvature=0 and clothance=1, you get cornu(t).
 */
{
  vector<long double> realparts,imagparts,cupower,clpower;
  int i,j;
  long double facpower,rsum,isum,t2,bigpart,binom,clotht,term,bigterm;
  double precision;
  t2=t*t;
  clotht=clothance*t;
  cupower.push_back(1);
  clpower.push_back(1);
  for (i=0,facpower=t;0.9+bigterm!=0.9 || !i;i++)
  {
    for (bigterm=j=0,binom=1;j<=i;j++)
    {
      term=clpower[j]*cupower[i-j]*binom*facpower/(i+j+1);
      //cout<<"i="<<i<<" j="<<j<<" term="<<cupower[j]<<'*'<<clpower[i-j]<<'*'<<binom<<'*'<<facpower<<'/'<<i+j+1<<'='<<term<<endl;
      if (fabsl(term)>bigterm)
	bigterm=fabsl(term);
      switch (i&3)
      {
	case 0:
	  realparts.push_back(term);
	  break;
	case 1:
	  imagparts.push_back(term);
	  break;
	case 2:
	  realparts.push_back(-term);
	  break;
	case 3:
	  imagparts.push_back(-term);
	  break;
      }
      binom=binom*(i-j)/(j+1);
    }
    cupower.push_back(cupower.back()*curvature);
    clpower.push_back(clpower.back()*clotht);
    facpower*=t/(i+1);
  }
  for (i=realparts.size()-1,rsum=bigpart=0;i>=0;i--)
  {
    rsum+=realparts[i];
    if (fabsl(realparts[i])>bigpart)
      bigpart=fabsl(realparts[i]);
  }
  for (i=imagparts.size()-1,isum=0;i>=0;i--)
  {
    isum+=imagparts[i];
    if (fabsl(imagparts[i])>bigpart)
      bigpart=fabsl(imagparts[i]);
  }
  precision=nextafterl(bigpart,2*bigpart)-bigpart;
  //printf("precision %e\n",precision);
  if (precision>1e-6)
    rsum=isum=nan("cornu");
  return xy(rsum,isum);
}

/* It should be possible to fit a spiral to be tangent to two given circular
 * or straight curves by successive approximation using these functions.
 */

double spiralbearing(double t,double curvature=0,double clothance=1)
{
  return t*t*clothance+t*curvature;
}

int ispiralbearing(double t,double curvature=0,double clothance=1)
{
  return radtobin(t*t*clothance+t*curvature);
}

double spiralcurvature(double t,double curvature=0,double clothance=1)
{
  return 2*t*clothance+curvature;
}

spiralarc::spiralarc()
{
  mid=start=end=xyz(0,0,0);
  control1=control2=0;
  cur=clo=len=0;
  midbear=0;
}

spiralarc::spiralarc(xyz kra,xyz fam)
{
  start=kra;
  end=fam;
  control1=(2*start.elev()+end.elev())/3;
  control2=(start.elev()+2*end.elev())/3;
  mid=(start+end)/2;
  len=dist(xy(start),xy(end));
  cur=clo=0;
  midbear=atan2i(xy(end-start));
}

void spiralarc::_setdelta(int d,int s)
{
  cur=bintorad(d)/len;
  clo=2*bintorad(s)/len/len;
}
