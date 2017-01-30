#include "TMRGeometry.h"
#include <math.h>
#include <stdio.h>

/*
  Build the curve without specifying the start/end vertices
*/
TMRCurve::TMRCurve(){
  v1 = NULL;
  v2 = NULL;
}

/*
  Build the curve the specified starting/ending vertices
*/
TMRCurve::TMRCurve( TMRVertex *_v1, TMRVertex *_v2 ){
  v1 = _v1;  v1->incref(); 
  v2 = _v2;  v2->incref();
}

/*
  Free the curve object
*/
TMRCurve::~TMRCurve(){
  if (v1){ v1->decref(); }
  if (v2){ v2->decref(); }
}

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
  Set the adjacent vertices
*/
void TMRCurve::setVertices( TMRVertex *_v1, TMRVertex *_v2 ){
  _v1->incref();
  _v2->incref();
  if (v1){ v1->decref(); }
  if (v2){ v2->decref(); }
  v1 = _v1;
  v2 = _v2;
}

/*
  Retrieve the adjacent vertices
*/
void TMRCurve::getVertices( TMRVertex **_v1, TMRVertex **_v2 ){
  if (_v1){ *_v1 = v1; }
  if (_v2){ *_v2 = v2; }
}

/*
  An integral entry for the linked list
*/
class IntegralPt {
 public:
  double t;
  double dist;
  IntegralPt *next;
};

/*
  Evaluate the distance between two points
*/
double pointDist( TMRPoint *a, TMRPoint *b ){
  return sqrt((a->x - b->x)*(a->x - b->x) + 
              (a->y - b->y)*(a->y - b->y) + 
              (a->z - b->z)*(a->z - b->z));
}

/*
  Recursive integration on an edge with an adaptive error control to
  ensure that the integral is computed with sufficient accuracy.

  input:
  t1, t2:  the limits of integration for this interval
  tol:     the absolute error measure
  ncalls:  the recursion depth
  pt:      pointer into the linked list
*/
void integrateEdge( TMRCurve *edge,
                    double t1, TMRPoint p1, double t2, double tol,
                    int ncalls, IntegralPt **_pt ){
  // Dereference the pointer to the integral point
  IntegralPt *pt = *_pt;

  // Find the mid point of the interval
  TMRPoint pmid;
  double tmid = 0.5*(t1 + t2);
  edge->evalPoint(tmid, &pmid);

  // Evaluate the point at the end of the interval
  TMRPoint p2; 
  edge->evalPoint(t2, &p2);
  
  // Evaluate the approximate integral contributions
  double int1 = (tmid - t1)*pointDist(&p1, &pmid);
  double int2 = (t2 - tmid)*pointDist(&pmid, &p2);
  double int3 = (tmid - t1)*pointDist(&p1, &p2);

  // Compute the integration error
  double error = fabs(int3 - int1 - int2);

  if (((ncalls > 5) && (error < tol)) || (ncalls > 20)){
    // Add the mid point
    pt->next = new IntegralPt;
    pt->next->dist = pt->dist + int1;
    pt->next->t = tmid;
    pt->next->next = NULL;
    pt = pt->next;

    // Add the final point p2
    pt->next = new IntegralPt;
    pt->next->dist = pt->dist + int2;
    pt->next->t = t2;
    pt->next->next = NULL;
    pt = pt->next;

    // Set the pointer to the end of the linked list
    *_pt = pt;
  }
  else {
    // Continue the recursive integration
    integrateEdge(edge, t1, p1, tmid, tol, ncalls+1, _pt);
    integrateEdge(edge, tmid, pmid, t2, tol, ncalls+1, _pt);
  }
}

/*
  Integrate along the edge adaptively, creating a list 
*/
double TMRCurve::integrate( double t1, double t2, double tol,
                            double **_tvals, double **_dist, 
                            int *_nvals ){
  *_tvals = NULL;
  *_dist = NULL;
  *_nvals = 0;

  // Allocate the entry in the linked list
  IntegralPt *root = new IntegralPt;
  root->next = NULL;
  root->dist = 0.0;
  root->t = t1;

  // Evaluate the first point
  TMRPoint p1;
  evalPoint(t1, &p1);

  // Integrate over the edge
  IntegralPt *pt = root;
  integrateEdge(this, t1, p1, t2, tol, 0, &pt);

  // Count up and allocate the num
  int count = 1;
  IntegralPt *curr = root;
  while (curr->next){
    curr = curr->next;
    count++;
  }

  // Allocate arrays to store the parametric location/distance data
  double *tvals = new double[ count ];
  double *dist = new double[ count ];

  // Scan through the linked list, read out the values of the
  // parameter and its integral and delete the entries as we go...
  count = 0;
  curr = root;
  tvals[count] = curr->t;
  dist[count] = curr->dist;
  count++;

  while (curr->next){
    IntegralPt *tmp = curr;
    curr = curr->next;
    tvals[count] = curr->t;
    dist[count] = curr->dist;
    count++;
    delete tmp;
  }

  double len = curr->dist;
  delete curr;

  // Set the pointers for the output
  *_nvals = count;
  *_tvals = tvals;
  *_dist = dist;

  return len;
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
  Write out a representation of the curve to a VTK file
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

/*
  Create a vertex from a point
*/
TMRVertexFromPoint::TMRVertexFromPoint( TMRPoint p ){
  pt = p;
}

/*
  Read out the point
*/
int TMRVertexFromPoint::evalPoint( TMRPoint *p ){
  *p = pt;
  return 0;
}

/*
  Create a vertex from curve
*/
TMRVertexFromCurve::TMRVertexFromCurve( TMRCurve *_curve, 
                                        double _t ){
  t = _t;
  curve = _curve;
  curve->incref(); 
}

/*
  Evaluate the vertex based on a node location
*/
TMRVertexFromCurve::TMRVertexFromCurve( TMRCurve *_curve,
                                        TMRPoint p ){
  curve = _curve;
  curve->incref();
  
  // Determine the parametric location of p using the initial
  // position
  curve->invEvalPoint(p, &t);
}

/*
  Free the object
*/
TMRVertexFromCurve::~TMRVertexFromCurve(){
  curve->decref();
}

/*
  Evaluate the point
*/
int TMRVertexFromCurve::evalPoint( TMRPoint *p ){
  return curve->evalPoint(t, p);
}

/*
  Determine the vertex location based on a surface location
*/
TMRVertexFromSurface::TMRVertexFromSurface( TMRSurface *_surface, 
                                            double _u, double _v ){
  surface = _surface;
  surface->incref();
  u = _u;
  v = _v;
}

/*
  First determine the parametric vertex locations by projecting
  the point onto the surface.
*/
TMRVertexFromSurface::TMRVertexFromSurface( TMRSurface *_surface, 
                                            TMRPoint p ){
  surface = _surface;
  surface->incref();
  surface->invEvalPoint(p, &u, &v);
}

/*
  Free the data
*/
TMRVertexFromSurface::~TMRVertexFromSurface(){
  surface->decref();
}

/*
  Evaluate the point on the surface
*/
int TMRVertexFromSurface::evalPoint( TMRPoint *p ){
  return surface->evalPoint(u, v, p);
}

TMRCurveFromSurfaceProjection::TMRCurveFromSurfaceProjection( TMRSurface *_surface, 
                                                              TMRCurve *_curve ){
  surface = _surface;
  curve = _curve;
  surface->incref();
  curve->incref();
}

TMRCurveFromSurfaceProjection::~TMRCurveFromSurfaceProjection(){
  curve->decref();
  surface->decref();
}
  
void TMRCurveFromSurfaceProjection::getRange( double *tmin, 
                                              double *tmax ){
  curve->getRange(tmin, tmax);
}
 
int TMRCurveFromSurfaceProjection::evalPoint( double t, TMRPoint *p ){
  // Evaluate the point on the curve
  TMRPoint pt;
  int fail = curve->evalPoint(t, &pt);
  
  if (!fail){
    // Find the parameters that are closest to the
    // point on the surface
    double u, v;
    fail = surface->invEvalPoint(pt, &u, &v);
      
    // Snap the point back to the surface
    if (!fail){
      fail = surface->evalPoint(u, v, p);
    }
  }

  return fail;
}
