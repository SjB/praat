/* ParamCurve.cpp
 *
 * Copyright (C) 1992-2012,2014,2015 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ParamCurve.h"

#include "oo_DESTROY.h"
#include "ParamCurve_def.h"
#include "oo_COPY.h"
#include "ParamCurve_def.h"
#include "oo_EQUAL.h"
#include "ParamCurve_def.h"
#include "oo_DESCRIPTION.h"
#include "ParamCurve_def.h"
#include "oo_CAN_WRITE_AS_ENCODING.h"
#include "ParamCurve_def.h"

Thing_implement (ParamCurve, Function, 2);

void structParamCurve :: v_info () {
	double xmin = 1e300, xmax = -1e300, ymin = 1e300, ymax = -1e300;
	for (long i = 1; i <= x -> nx; i ++) {
		double value = x -> z [1] [i];
		if (value < xmin) xmin = value;
		if (value > xmax) xmax = value;
	}
	for (long i = 1; i <= y -> nx; i ++) {
		double value = y -> z [1] [i];
		if (value < ymin) ymin = value;
		if (value > ymax) ymax = value;
	}
	structDaata :: v_info ();
	MelderInfo_writeLine (U"Domain:");
	MelderInfo_writeLine (U"   tmin: ", xmin);
	MelderInfo_writeLine (U"   tmax: ", xmax);
	MelderInfo_writeLine (U"x sampling:");
	MelderInfo_writeLine (U"   Number of values of t in x: ", x -> nx);
	MelderInfo_writeLine (U"   t step in x: ", x -> dx, U" (sampling rate ", 1.0 / x -> dx, U")");
	MelderInfo_writeLine (U"   First t in x: ", x -> x1);
	MelderInfo_writeLine (U"x values:");
	MelderInfo_writeLine (U"   Minimum x: ", xmin);
	MelderInfo_writeLine (U"   Maximum x: ", xmax);
	MelderInfo_writeLine (U"y sampling:");
	MelderInfo_writeLine (U"   Number of values of t in y: ", y -> nx);
	MelderInfo_writeLine (U"   t step in y: ", y -> dx, U" (sampling rate ", 1.0 / y -> dx, U")");
	MelderInfo_writeLine (U"   First t in y: ", y -> x1);
	MelderInfo_writeLine (U"y values:");
	MelderInfo_writeLine (U"   Minimum y: ", ymin);
	MelderInfo_writeLine (U"   Maximum y: ", ymax);
}

void structParamCurve :: v_writeText (MelderFile file) {
	Data_writeText (x, file);
	Data_writeText (y, file);
}

void structParamCurve :: v_readText (MelderReadText text, int formatVersion) {
	x = Thing_new (Sound);
	y = Thing_new (Sound);
	Data_readText (x, text, formatVersion);
	Data_readText (y, text, formatVersion);
	xmin = x -> xmin > y -> xmin ? x -> xmin : y -> xmin;
	xmax = x -> xmax < y -> xmax ? x -> xmax : y -> xmax;
}

void structParamCurve :: v_writeBinary (FILE *f) {
	Data_writeBinary (x, f);
	Data_writeBinary (y, f);
}

void structParamCurve :: v_readBinary (FILE *f, int /*formatVersion*/) {
	x = Thing_new (Sound);
	y = Thing_new (Sound);
	Data_readBinary (x, f, 2);
	Data_readBinary (y, f, 2);
	xmin = x -> xmin > y -> xmin ? x -> xmin : y -> xmin;
	xmax = x -> xmax < y -> xmax ? x -> xmax : y -> xmax;
}

void ParamCurve_init (ParamCurve me, Sound x, Sound y) {
	if (x -> xmax <= y -> xmin || x -> xmin >= y -> xmax)
		Melder_throw (U"Domains do not overlap.");
	my x = Data_copy (x);
	my y = Data_copy (y);
	my xmin = x -> xmin > y -> xmin ? x -> xmin : y -> xmin;
	my xmax = x -> xmax < y -> xmax ? x -> xmax : y -> xmax;
}

ParamCurve ParamCurve_create (Sound x, Sound y) {
	try {
		autoParamCurve me = Thing_new (ParamCurve);
		ParamCurve_init (me.peek(), x, y);
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"ParamCurve not created.");
	}
}

void ParamCurve_draw (ParamCurve me, Graphics g, double t1, double t2, double dt,
	double x1, double x2, double y1, double y2, int garnish)
{
	if (t2 <= t1) {
		double tx1 = my x -> x1;
		double ty1 = my y -> x1;
		double tx2 = my x -> x1 + (my x -> nx - 1) * my x -> dx;
		double ty2 = my y -> x1 + (my y -> nx - 1) * my y -> dx;
		t1 = tx1 > ty1 ? tx1 : ty1;
		t2 = tx2 < ty2 ? tx2 : ty2;
	}
	if (x2 <= x1) Matrix_getWindowExtrema (my x, 0, 0, 1, 1, & x1, & x2);
	if (x1 == x2) { x1 -= 1.0; x2 += 1.0; }
	if (y2 <= y1) Matrix_getWindowExtrema (my y, 0, 0, 1, 1, & y1, & y2);
	if (y1 == y2) { y1 -= 1.0; y2 += 1.0; }
	if (dt <= 0.0)
		dt = my x -> dx < my y -> dx ? my x -> dx : my y -> dx;
	long numberOfPoints = (long) ceil ((t2 - t1) / dt) + 1;
	if (numberOfPoints > 0) {
		autoNUMvector <double> x (1, numberOfPoints);
		autoNUMvector <double> y (1, numberOfPoints);
		for (long i = 1; i <= numberOfPoints; i ++) {
			double t = i == numberOfPoints ? t2 : t1 + (i - 1) * dt;
			double index = Sampled_xToIndex (my x, t);
			x [i] = NUM_interpolate_sinc (my x -> z [1], my x -> nx, index, 50);
			index = Sampled_xToIndex (my y, t);
			y [i] = NUM_interpolate_sinc (my y -> z [1], my y -> nx, index, 50);
		}
		Graphics_setWindow (g, x1, x2, y1, y2);
		Graphics_setInner (g);
		Graphics_polyline (g, numberOfPoints, & x [1], & y [1]);
		Graphics_unsetInner (g);
	}
	if (garnish) {
		Graphics_drawInnerBox (g);
		Graphics_marksBottom (g, 2, 1, 1, 0);
		Graphics_marksLeft (g, 2, 1, 1, 0);
	}
}

void ParamCurve_swapXY (ParamCurve me) {
	Sound help = my x;
	my x = my y;
	my y = help;
}

/* End of file ParamCurve.cpp */
