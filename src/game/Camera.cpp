/*
 * Copyright 2011-2016 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "game/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include "graphics/Math.h"
#include "graphics/Renderer.h"


MASTER_CAMERA_STRUCT MasterCamera;

static glm::mat4x4 createProjectionMatrix(const Vec2f & size, const Vec2f & center, Camera * cam) {

	float fov = focalToFov(cam->focal);
	
	const float nearDist = 1.f;
	const float farDist = cam->cdepth;
	const float frustumDepth = farDist - nearDist;
	
	float aspect = size.y / size.x;
	float w = aspect * (glm::cos(fov / 2) / glm::sin(fov / 2));
	float h =   1.0f  * (glm::cos(fov / 2) / glm::sin(fov / 2));
	float Q = farDist / frustumDepth;
	
	glm::mat4x4 projectionMatrix;
	projectionMatrix[0][0] = w;
	projectionMatrix[1][1] = -h;
	projectionMatrix[2][2] = Q;
	projectionMatrix[3][2] = -Q * nearDist;
	projectionMatrix[2][3] = 1.f;
	projectionMatrix[3][3] = 0.f;
	
	glm::mat4x4 centerShift(1);
	centerShift = glm::translate(centerShift, Vec3f(2.f * center.x / size.x - 1.f,
	                                                1.f - 2.f * center.y / size.y,
	                                                0.f));
	
	return centerShift * projectionMatrix;
}

void PrepareCamera(Camera * cam, const Rect & viewport, const Vec2i & projectionCenter) {
	
	cam->m_worldToView = glm::translate(toRotationMatrix(cam->angle), -cam->m_pos);
	GRenderer->SetViewMatrix(cam->m_worldToView);
	
	const Vec2f center = Vec2f(projectionCenter - viewport.topLeft());
	glm::mat4x4 projectionMatrix = createProjectionMatrix(Vec2f(viewport.size()), center, cam);
	GRenderer->SetProjectionMatrix(projectionMatrix);
	
	// Change coordinate system from [-1, 1] x [-1, 1] to [0, width] x [0, height] and flip the y axis
	glm::mat4x4 ndcToScreen(1);
	ndcToScreen = glm::scale(ndcToScreen, Vec3f(Vec2f(viewport.size()) / 2.f, 1.f));
	ndcToScreen = glm::translate(ndcToScreen, Vec3f(1.f, 1.f, 0.f));
	ndcToScreen = glm::scale(ndcToScreen, Vec3f(1.f, -1.f, 1.f));
	cam->m_viewToScreen = ndcToScreen * projectionMatrix;
	
	GRenderer->SetViewport(viewport);
	
}

Camera * ACTIVECAM = NULL;

void SetActiveCamera(Camera * cam) {
	if (ACTIVECAM != cam) ACTIVECAM = cam;
}
