#include "TMRGeometry.h"
#include "TMRMesh.h"
#include <math.h>
#include <stdio.h>

/*
  Compute the inverse: This is not always required. By default it is
  not implemented. Derived classes can implement it if needed.
*/
int TMRCurve::invEvalPoint( TMRPoint X, double *t ){
  int fail = 1;
  *t = 0.0;
  return fail;
}

/*
  Set the step size for the derivative
*/
double TMRCurve::deriv_step_size = 1e-6;

/*
  Evaluate the derivative using a finite-difference step size
*/
int TMRCurve::evalDeriv( double t, TMRPoint *Xt ){
  int fail = 1;

  // Retrieve the parameter bounds for the curve
  double tmin, tmax; 
  getRange(&tmin, &tmax);

  if (t >= tmin && t <= tmax){
    // Evaluate the point at the original 
    TMRPoint p;
    fail = evalPoint(t, &p);
    if (fail){ return fail; }
    
    // Compute the approximate derivative using a forward
    // difference
    if (t + deriv_step_size <= tmax){
      TMRPoint p2;
      fail = evalPoint(t + deriv_step_size, &p2);
      if (fail){ return fail; }

      Xt->x = (p2.x - p.x)/deriv_step_size;
      Xt->y = (p2.y - p.y)/deriv_step_size;
      Xt->z = (p2.z - p.z)/deriv_step_size;
    }
    else if (t >= tmin + deriv_step_size){
      TMRPoint p2;
      fail = evalPoint(t - deriv_step_size, &p2);
      if (fail){ return fail; }

      Xt->x = (p.x - p2.x)/deriv_step_size;
      Xt->y = (p.y - p2.y)/deriv_step_size;
      Xt->z = (p.z - p2.z)/deriv_step_size;
    }
  }

  return fail;
}

/*
  Write out a representation of the curve to a VTK file
*/
void TMRCurve::writeToVTK( const char *filename ){
  double t1, t2;
  getRange(&t1, &t2);

  const int npts = 100;

  // Write out the vtk file
  FILE *fp = fopen(filename, "w");
  if (fp){
    fprintf(fp, "# vtk DataFile Version 3.0\n");
    fprintf(fp, "vtk output\nASCII\n");
    fprintf(fp, "DATASET UNSTRUCTURED_GRID\n");
    
    // Write out the points
    fprintf(fp, "POINTS %d float\n", npts);
    for ( int k = 0; k < npts; k++ ){
      double u = 1.0*k/(npts-1);
      double t = (1.0-u)*t1 + u*t2;

      // Evaluate the point
      TMRPoint p;
      evalPoint(t, &p);
      
      // Write out the point
      fprintf(fp, "%e %e %e\n", p.x, p.y, p.z);
    }
    
    // Write out the cell values
    fprintf(fp, "\nCELLS %d %d\n", npts-1, 3*(npts-1));
    for ( int k = 0; k < npts-1; k++ ){
      fprintf(fp, "2 %d %d\n", k, k+1);
    }
    
    // Write out the cell types
    fprintf(fp, "\nCELL_TYPES %d\n", npts-1);
    for ( int k = 0; k < npts-1; k++ ){
      fprintf(fp, "%d\n", 3);
    }
    
    fclose(fp);
  } 
}

/*
  Set the step size for the derivative
*/
double TMRSurface::deriv_step_size = 1e-6;

/*
  Evaluate the derivative using a finite-difference step size
*/
int TMRSurface::evalDeriv( double u, double v, 
                           TMRPoint *Xu, TMRPoint *Xv ){
  int fail = 0;

  // Retrieve the parameter bounds for the curve
  double umin, vmin, umax, vmax;
  getRange(&umin, &vmin, &umax, &vmax);

  if (u >= umin && u <= umax &&
      v >= vmin && v <= vmax){
    // Evaluate the point at the original 
    TMRPoint p;
    fail = evalPoint(u, v, &p);

    // Compute the approximate derivative using a forward
    // difference or backward difference, depending on whether
    // the step is within the domain
    if (u + deriv_step_size <= umax){
      TMRPoint p2;
      fail = fail || evalPoint(u + deriv_step_size, v, &p2);

      Xu->x = (p2.x - p.x)/deriv_step_size;
      Xu->y = (p2.y - p.y)/deriv_step_size;
      Xu->z = (p2.z - p.z)/deriv_step_size;
    }
    else if (u >= umin + deriv_step_size){
      TMRPoint p2;
      fail = fail || evalPoint(u - deriv_step_size, v, &p2);

      Xu->x = (p.x - p2.x)/deriv_step_size;
      Xu->y = (p.y - p2.y)/deriv_step_size;
      Xu->z = (p.z - p2.z)/deriv_step_size;
    }
    else {
      fail = 1;
    }

    // Compute the approximate derivative using a forward
    // difference
    if (v + deriv_step_size <= vmax){
      TMRPoint p2;
      fail = fail || evalPoint(u, v + deriv_step_size, &p2);

      Xv->x = (p2.x - p.x)/deriv_step_size;
      Xv->y = (p2.y - p.y)/deriv_step_size;
      Xv->z = (p2.z - p.z)/deriv_step_size;
    }
    else if (v >= vmin + deriv_step_size){
      TMRPoint p2;
      fail = fail || evalPoint(u, v - deriv_step_size, &p2);

      Xv->x = (p.x - p2.x)/deriv_step_size;
      Xv->y = (p.y - p2.y)/deriv_step_size;
      Xv->z = (p.z - p2.z)/deriv_step_size;
    }
    else {
      fail = 1;
    }
  }

  return fail;
}

/*
  Write out a representation of the surface to a VTK file
*/
void TMRSurface::writeToVTK( const char *filename ){
  double umin, vmin, umax, vmax;
  getRange(&umin, &vmin, &umax, &vmax);

  const int npts = 100;

  // Write out the vtk file
  FILE *fp = fopen(filename, "w");
  if (fp){
    fprintf(fp, "# vtk DataFile Version 3.0\n");
    fprintf(fp, "vtk output\nASCII\n");
    fprintf(fp, "DATASET UNSTRUCTURED_GRID\n");
    
    // Write out the points
    fprintf(fp, "POINTS %d float\n", npts*npts);
    for ( int j = 0; j < npts; j++ ){
      for ( int i = 0; i < npts; i++ ){
        double u = 1.0*i/(npts-1);
        double v = 1.0*j/(npts-1);
        u = (1.0 - u)*umin + u*umax;
        v = (1.0 - v)*vmin + v*vmax;

        // Evaluate the point
        TMRPoint p;
        evalPoint(u, v, &p);
        
        // Write out the point
        fprintf(fp, "%e %e %e\n", p.x, p.y, p.z);
      }
    } 
    
    // Write out the cell values
    fprintf(fp, "\nCELLS %d %d\n", (npts-1)*(npts-1), 5*(npts-1)*(npts-1));
    for ( int j = 0; j < npts-1; j++ ){
      for ( int i = 0; i < npts-1; i++ ){
        fprintf(fp, "4 %d %d %d %d\n", 
                i + j*npts, i+1 + j*npts, 
                i+1 + (j+1)*npts, i + (j+1)*npts);
      }
    }
    
    // Write out the cell types
    fprintf(fp, "\nCELL_TYPES %d\n", (npts-1)*(npts-1));
    for ( int k = 0; k < (npts-1)*(npts-1); k++ ){
      fprintf(fp, "%d\n", 9);
    }
    
    fclose(fp);
  } 
}

