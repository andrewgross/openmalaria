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


#ifndef SCENE_H
#define SCENE_H

#include "SceneController.h"
#include "float3.h"
#include "Display.h"
#include "Anopheles.h"
#include "ViewController.h"
#include <vector>

typedef std::vector<Anopheles*>	anophelesVector;

class SkyBox;

class Scene
{
	public:

		Scene();
    		
		bool								switches[3], overlayOn;
		unsigned int				screenshotIndex, anophelesCount, frames;
		double							phi, theta, r, rDot, fov;
		float3							deltaS, deltaSDot, light;
		float								occlusion, fps, overlayPresence;
		SceneController			controller;
		SkyBox*							skyBox;
		DisplayMM*						dataDisplay;
		ViewController*			viewController;
		anophelesVector			anopheles;

		void render();
		void setPerspectiveMatrix(float aspect);
		void saveScreenshot();
};

#endif

