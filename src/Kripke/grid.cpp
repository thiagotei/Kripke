/*--------------------------------------------------------------------------
 * Utility functions for the Grid_Data structure.
 *--------------------------------------------------------------------------*/

#include <Kripke/grid.h>
#include <Kripke/input_variables.h>
#include <cmath>
#include <float.h>






/*--------------------------------------------------------------------------
 * GenGrid : Creates a new Grid_Data structure and allocates
 *                 memory for its data.
 *
 * Currently, the spatial grid is calculated so that cells are a uniform
 * length = (xmax - xmin) / nx
 * in each spatial direction.
 *
 *--------------------------------------------------------------------------*/

Grid_Data::Grid_Data(Input_Variables *input_vars, int num_dirs, int num_grps,
                   MPI_Comm comm)
{
  int npx = input_vars->npx;
  int npy = input_vars->npy;
  int npz = input_vars->npz;
  int nx_g = input_vars->nx;
  int ny_g = input_vars->ny;
  int nz_g = input_vars->nz;

  /* Compute the local coordinates in the processor decomposition */
  int myid;
  MPI_Comm_rank(comm, &myid);

  int isub_ref = myid % npx;
  int jsub_ref = ((myid - isub_ref) / npx) % npy;
  int ksub_ref = (myid - isub_ref - npx*jsub_ref) / (npx * npy);

  /* Compute the processor neighbor array assuming a lexigraphic ordering */
  if(isub_ref == 0){
    mynbr[0][0] = -1;
  }
  else {
    mynbr[0][0] = myid - 1;
  }

  if(isub_ref == npx-1){
    mynbr[0][1] = -1;
  }
  else {
    mynbr[0][1] = myid + 1;
  }

  if(jsub_ref == 0){
    mynbr[1][0] = -1;
  }
  else {
    mynbr[1][0] = myid - npx;
  }

  if(jsub_ref == npy-1){
    mynbr[1][1] = -1;
  }
  else {
    mynbr[1][1] = myid + npx;
  }

  if(ksub_ref == 0){
    mynbr[2][0] = -1;
  }
  else {
    mynbr[2][0] = myid - npx * npy;
  }

  if(ksub_ref == npz-1){
    mynbr[2][1] = -1;
  }
  else {
    mynbr[2][1] = myid + npx * npy;
  }
  
  computeGrid(0, npx, nx_g, isub_ref, input_vars->xmin, input_vars->xmax);
  computeGrid(1, npy, ny_g, jsub_ref, input_vars->ymin, input_vars->ymax);
  computeGrid(2, npz, nz_g, ksub_ref, input_vars->zmin, input_vars->zmax);


  /* Compute the volumes */
  int num_zones = nzones[0]*nzones[1]*nzones[2];
  volume.resize(num_zones);
  for(int k = 0; k < nzones[2]; k++){
    for(int j = 0; j < nzones[1]; j++){
      for(int i = 0; i < nzones[0]; i++){
        int ijk = i + nzones[0]*j + nzones[0]*nzones[1]*k;
        volume[ijk] = deltas[0][i+1]*deltas[1][j+1]*deltas[2][k+1];
      }
    }
  }

 
  tmp_sigma_tot.resize(num_zones, 0.0);

  // Allocate Storage for sweeps
  num_directions = num_dirs;
  num_groups = num_grps;

}


void Grid_Data::computeGrid(int dim, int npx, int nx_g, int isub_ref, double xmin, double xmax){
 /* Calculate unit roundoff and load into grid_data */
  double eps = DBL_EPSILON;
  double thsnd_eps = 1000.e0*(eps);
 
  // Compute subset of global zone indices
  int nx_l = nx_g / npx;
  int rem = nx_g % npx;
  int ilower, iupper;
  if(rem != 0){
    if(isub_ref < rem){
      nx_l++;
      ilower = isub_ref * nx_l;
    }
    else {
      ilower = rem + isub_ref * nx_l;
    }
  }
  else {
    ilower = isub_ref * nx_l;
  }

  iupper = ilower + nx_l - 1;

  // allocate grid deltas
  deltas[dim].resize(nx_l+2);
  
  // Compute the spatial grid 
  double dx = (xmax - xmin) / nx_g;
  double coord_lo = xmin + (ilower) * dx;
  double coord_hi = xmin + (iupper+1) * dx;
  for(int i = 0; i < nx_l+2; i++){
    deltas[dim][i] = dx;
  }
  if(std::abs(coord_lo - xmin) <= thsnd_eps*std::abs(xmin)){
    deltas[dim][0] = 0.0;
  }
  if(std::abs(coord_hi - xmax) <= thsnd_eps*std::abs(xmax)){
    deltas[dim][nx_l+1] = 0.0;
  }
  
  nzones[dim] = nx_l; 
}
