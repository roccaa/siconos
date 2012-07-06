/* Siconos-Numerics, Copyright INRIA 2005-2011.
 * Siconos is a program dedicated to modeling, simulation and control
 * of non smooth dynamical systems.
 * Siconos is a free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Siconos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Siconos; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact: Vincent ACARY, siconos-team@lists.gforge.inria.fr
 */
#include <stdio.h>
#include <stdlib.h>
#include "NonSmoothDrivers.h"
#include "frictionContact_test_function.h"


int main(void)
{
  int info = 0 ;
  int nc = 4;//Number of contacts
  int nb = 4;//Number of blocks

  double q[12] = { -1, 1, 3, -1, 1, 3, -1, 1, 3, -1, 1, 3};
  double mu[4] = {0.1, 0.1, 0.1, 0.1};

  unsigned int row[4] = {1, 2, 3, 4};
  unsigned int column[4] = {1, 2, 3, 4};
  double W[36] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1};


  double *reaction = (double*)malloc(3 * nc * sizeof(double));
  double *velocity = (double*)malloc(3 * nc * sizeof(double));
  for (int i = 0; i < 3 * nc; i++)
  {
    reaction[i] = 0.0;
    velocity[i] = 0.0;
  }

  int solver_id = SICONOS_FRICTION_3D_NSGS; // 500
  double tolerance = 1e-16;
  int itermax = 100;

  info = frictionContact3D_LmgcDriver(reaction,
                                      velocity,
                                      q,
                                      mu,
                                      W,
                                      row,
                                      column,
                                      nc,
                                      nb,
                                      solver_id,
                                      tolerance,
                                      itermax,
                                      0);

  free(reaction);
  free(velocity);

  return info;
}
