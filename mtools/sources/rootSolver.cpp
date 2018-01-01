/** @file rootSolver.cpp */
//
// Copyright 2015 Arvind Singh
//
// This file is part of the mtools library.
//
// mtools is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with mtools  If not, see <http://www.gnu.org/licenses/>.


#include "stdafx_mtools.h"

#include "misc/internal/mtools_export.hpp"
#include "maths/rootSolver.hpp"
#include "misc/misc.hpp"

#include <math.h>


namespace mtools
{


int gsl_poly_solve_quadratic (double a, double b, double c, double *x0, double *x1)
	{
	if (a == 0) /* Handle linear case */
		{
		if (b == 0) { return 0; } else { *x0 = -c / b; return 1; };
		}
    const double disc = b * b - 4 * a * c;    
    if (disc > 0)
		{
		if (b == 0)
			{
			const double r = sqrt (-c / a);
			*x0 = -r;
			*x1 =  r;
			}
        else
			{
			const double sgnb = (b > 0 ? 1 : -1);
			const double temp = -0.5 * (b + sgnb * sqrt (disc));
			const double r1 = temp / a ;
			const double r2 = c / temp ;
			if (r1 < r2) 
				{
				*x0 = r1 ;
				*x1 = r2 ;
				} 
            else 
				{
				*x0 = r2 ;
				*x1 = r1 ;
				}
			}
        return 2;
		}
    else if (disc == 0) 
		{
        *x0 = -0.5 * b / a ;
        *x1 = -0.5 * b / a ;
        return 2 ;
		}
    else return 0;
	}



int gsl_poly_solve_cubic(double k, double a, double b, double c, double *x0, double *x1, double *x2)
	{

	if (k == 0)
		{ // quadratic case 
		return gsl_poly_solve_quadratic(a, b, c, x0, x1);
		}
	// normalize 
	a /= k; 
	b /= k;
	c /= k;

	double q = (a * a - 3 * b);
	double r = (2 * a * a * a - 9 * a * b + 27 * c);

	double Q = q / 9;
	double R = r / 54;

	double Q3 = Q * Q * Q;
	double R2 = R * R;

	double CR2 = 729 * r * r;
	double CQ3 = 2916 * q * q * q;

	if (R == 0 && Q == 0)
		{
		*x0 = -a / 3;
		*x1 = -a / 3;
		*x2 = -a / 3;
		return 3;
		}
	else if (CR2 == CQ3)
		{
		/* this test is actually R2 == Q3, written in a form suitable
		for exact computation with integers */
		/* Due to finite precision some double roots may be missed, and
		considered to be a pair of complex roots z = x +/- epsilon i
		close to the real axis. */
		double sqrtQ = sqrt(Q);
		if (R > 0)
			{
			*x0 = -2 * sqrtQ - a / 3;
			*x1 = sqrtQ - a / 3;
			*x2 = sqrtQ - a / 3;
			}
		else
			{
			*x0 = -sqrtQ - a / 3;
			*x1 = -sqrtQ - a / 3;
			*x2 = 2 * sqrtQ - a / 3;
			}
		return 3;
		}
	else if (R2 < Q3)
		{
		double sgnR = (R >= 0 ? 1 : -1);
		double ratio = sgnR * sqrt(R2 / Q3);
		double theta = acos(ratio);
		double norm = -2 * sqrt(Q);
		*x0 = norm * cos(theta / 3) - a / 3;
		*x1 = norm * cos((theta + 2.0 * M_PI) / 3) - a / 3;
		*x2 = norm * cos((theta - 2.0 * M_PI) / 3) - a / 3;
		/* Sort *x0, *x1, *x2 into increasing order */
		if (*x0 > *x1) mtools::swap(*x0, *x1);
		if (*x1 > *x2)
			{
			mtools::swap(*x1, *x2);
			if (*x0 > *x1) mtools::swap(*x0, *x1);
			}	
		return 3;
		}
	else
		{
		double sgnR = (R >= 0 ? 1 : -1);
		double A = -sgnR * pow(fabs(R) + sqrt(R2 - Q3), 1.0 / 3.0);
		double B = Q / A;
		*x0 = A + B - a / 3;
		return 1;
		}
	}



}

/* end of file */

