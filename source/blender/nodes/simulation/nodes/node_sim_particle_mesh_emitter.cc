/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "node_simulation_util.h"

#include "float.h"

static bNodeSocketTemplate sim_node_particle_mesh_emitter_in[] = {
    {SOCK_OBJECT, N_("Object")},
    {SOCK_FLOAT, N_("Rate"), 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, FLT_MAX},
    {-1, ""},
};

static bNodeSocketTemplate sim_node_particle_mesh_emitter_out[] = {
    {SOCK_EMITTERS, N_("Emitter")},
    {-1, ""},
};

void register_node_type_sim_particle_mesh_emitter()
{
  static bNodeType ntype;

  sim_node_type_base(&ntype, SIM_NODE_PARTICLE_MESH_EMITTER, "Mesh Emitter", 0, 0);
  node_type_socket_templates(
      &ntype, sim_node_particle_mesh_emitter_in, sim_node_particle_mesh_emitter_out);
  nodeRegisterType(&ntype);
}
