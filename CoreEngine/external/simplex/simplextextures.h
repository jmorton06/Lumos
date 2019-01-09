/* Copyright (c) 2007-2012 Eliot Eshelman
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TEXTURES_H_
#define TEXTURES_H_

 // Composite procedural textures built from noise functions.
float marble_noise_2d(const float octaves,
	const float persistence,
	const float scale,
	const float x,
	const float y);
float marble_noise_3d(const float octaves,
	const float persistence,
	const float scale,
	const float x,
	const float y,
	const float z);
float marble_noise_4d(const float octaves,
	const float persistence,
	const float scale,
	const float x,
	const float y,
	const float z,
	const float w);

#endif /*TEXTURES_H_*/
