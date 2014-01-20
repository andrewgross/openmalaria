/* This file is part of OpenMalaria.
 * 
 * Copyright (C) 2005-2014 Swiss Tropical and Public Health Institute
 * Copyright (C) 2005-2014 Liverpool School Of Tropical Medicine
 * 
 * OpenMalaria is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "Debug.h"

int Debug :: ints[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
float Debug :: floats[12] = {	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,
															0.0f,0.0f,0.0f,0.0f,0.0f,0.0f	};
double Debug :: doubles[12] = {	30.0,0.0,0.0,0.0,0.0,0.0,
																0.0,0.0,0.0,0.0,0.0,0.0	};
