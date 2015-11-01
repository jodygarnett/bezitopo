/******************************************************/
/*                                                    */
/* document.cpp - main document class                 */
/*                                                    */
/******************************************************/
#include "bezitopo.h"
#include "pnezd.h"
#include "document.h"
using namespace std;

void document::copytopopoints(criteria crit)
{
  ptlist::iterator i;
  if (pl.size()<2)
    pl.resize(2);
  pl[1].clear();
  int j;
  bool include;
  for (i=pl[0].points.begin();i!=pl[0].points.end();i++)
  {
    include=false;
    for (j=0;j<crit.size();j++)
      if (i->second.note.find(crit[j].str)!=string::npos)
	include=crit[j].istopo;
    if (include)
      pl[1].addpoint(i->first,i->second);
  }
}

int document::readpnezd(string fname,bool overwrite)
{
  return ::readpnezd(this,fname,overwrite);
}

int document::writepnezd(string fname)
{
  ::writepnezd(this,fname);
}

void document::addobject(drawobj *obj)
// The drawobj must be created with new; it will be destroyed with delete.
{
  objrec o;
  if (curlayer<0 || curlayer>=layers.size())
  {
    curlayer=0;
    if (layers.size()==0)
    {
      layer l;
      l.colr=WHITE;
      l.visible=true;
      l.name="0";
      layers.push_back(l);
    }
  }
  o.layr=curlayer;
  o.ltype=o.colr=o.thik=SAMECOLOR;
  o.obj=obj;
  objlist.push_back(o);
}
