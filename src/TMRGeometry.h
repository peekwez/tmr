#ifndef TMR_GEOMETRY_H
#define TMR_GEOMETRY_H

/*
  The following header file contains the interface for the geometry/
  topology for the TMR objects. These vertex/edge/surface and volume
  objects are used to map the 

  The vertex, edge, face and volume classes are used in conjunction
  with the TMROct(Quad)Forest class to evaluate nodal locations with
  the mesh. These interfaces are designed to be overriden with an
  external geometry engine.
*/

#include "TMRBase.h"

/*
  The parametrization for a curve
*/
class TMRCurve : public TMREntity {
 public:
  // Get the parameter range for this edge
  virtual void getRange( double *tmin, double *tmax ) = 0;
  
  // Given the parametric point, evaluate the x,y,z location
  virtual int evalPoint( double t, TMRPoint *X ) = 0;

  // Given the point, find the parametric location
  virtual int invEvalPoint( TMRPoint X, double *t );

  // Given the parametric point, evaluate the derivative 
  virtual int evalDeriv( double t, TMRPoint *Xt );
  
  // Write the object to the VTK file
  void writeToVTK( const char *filename );
  
 private:
  // Finite-difference step size
  static double deriv_step_size;
};

/*
  The parametrization of a surface
*/
class TMRSurface : public TMREntity {
 public:
  // Get the parameter range for this surface
  virtual void getRange( double *umin, double *vmin,
                         double *umax, double *vmax ) = 0;
 
  // Given the parametric point, compute the x,y,z location
  virtual int evalPoint( double u, double v, TMRPoint *X ) = 0;
  
  // Perform the inverse evaluation
  virtual int invEvalPoint( TMRPoint p, double *u, double *v ) = 0;

  // Given the parametric point, evaluate the first derivative 
  virtual int evalDeriv( double u, double v, 
                         TMRPoint *Xu, TMRPoint *Xv );

  // Write the object to the VTK file
  void writeToVTK( const char *filename );

 private:
  // Finite-difference step size
  static double deriv_step_size;
};

/*
  Abstract base class for a pametric curve (u(t), v(t)) which can
  be used to define a curve on a surface
*/
class TMRPcurve : public TMREntity {
 public:
  // Get the parameter range for this edge
  virtual void getRange( double *tmin, double *tmax ) = 0;
  
  // Given the parametric point, evaluate the x,y,z location
  virtual int evalPoint( double t, double *u, double *v ) = 0;

  // Given the parametric point, evaluate the derivative 
  virtual int evalDeriv( double t, double *ut, double *vt ) = 0;
};

#endif // TMR_GEOMETRY_H
