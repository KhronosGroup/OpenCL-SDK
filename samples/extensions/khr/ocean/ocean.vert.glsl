/*
 * Copyright (c) 2024 Mobica Limited, Marcin Hajder
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#version 450

layout(location = 0) out vec2 frag_tex_coord;
layout(location = 1) out vec4 ec_pos;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coords;

layout(binding = 0) uniform sampler2D u_displacement_map;
layout(std140, binding = 2) uniform ViewData {
    uniform mat4    view_mat;
    uniform mat4    proj_mat;
    uniform vec3    sun_dir;
    uniform float   choppiness;
    uniform float   alt_scale;
} view;

void main()
{
    vec3 displ = texture(u_displacement_map, in_tex_coords).rbg;
    displ.xy *= view.choppiness;
    displ.z *= view.alt_scale;
    vec3 ocean_vert = in_position + displ;
    ec_pos = view.view_mat * vec4(ocean_vert, 1.0);
    gl_Position = view.proj_mat * ec_pos;
    frag_tex_coord = in_tex_coords;
}
