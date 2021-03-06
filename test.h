/******************************************************/
/*                                                    */
/* test.h - test patterns and functions               */
/*                                                    */
/******************************************************/
/* Copyright 2012,2013,2014,2015,2017 Pierre Abbat.
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

#include "document.h"

/* Test patterns:
   1. Asteraceous pattern. r=sqrt(n+0.5), θ=2πφn.
   2. Square root spiral.
   3. Square lattice.
   4. A row of points on one line.
   5. All points except two lie on one line; the last two are on opposite sides
      of the line. This tests that the triangulator works properly when it cannot
      make any triangles until the last two points.
   6. Hexagonal lattice. The Delaunay triangulation is ambiguous.
   7. Points on a circle, angles are multiples of 2πφ. Ditto.
   8. Points on a slightly eccentric ellipse. This usually takes O(n²) flips.
   9. Points on a circle, equally spaced. 
   Test operations:
   1. Rotate points by (0.6,0.8) n times. This introduces LSB errors
      to test numerical stability.
   
   For worst-case time of the Delaunay algorithm, do this:
   cir 1 rot 1 cir 3 rot 1 cir 5 rot 1 ... cir 1999 rot 1
   This produces points in a circle, displaced by a few picometers by the rotation.
   The mean of the points, however, is off by a micrometer. They will be added to
   the triangulation in order of distance from point 1, then all interior lines
   will be flipped many times.
   */

#define RUGAE 0
#define HYPAR 1
#define CIRPAR 2
#define FLATSLOPE 3
#define HASH 4
void setsurface(int surf);
void dumppoints();
void dumppointsvalence(document &doc);
void aster(document &doc,int n);
void ring(document &doc,int n);
void regpolygon(document &doc,int n);
void ellipse(document &doc,int n);
void longandthin(document &doc,int n);
void straightrow(document &doc,int n);
void lozenge(document &doc,int n);
void wheelwindow(document &doc,int n);
void rotate(document &doc,int n);
void movesideways(document &doc,double sw);
void moveup(document &doc,double sw);
void enlarge(document &doc,double sc);
extern xy (*testsurfacegrad)(xy pnt);
