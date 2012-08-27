/******************************************************/
/*                                                    */
/* segment.h - 3d line segment                        */
/* base class of arc and spiral                       */
/*                                                    */
/******************************************************/

#include "point.h"
#define START 1
#define END 2

class segment
{
private:
  xyz start,end;
  double control1,control2;
public:
  segment();
  segment(xyz kra,xyz fam);
  double length();
  void setslope(int which,double s);
  double elev(double along);
  double slope(double along);
  xyz station(double along);
};
