#ifndef TMR_OCTANT_TREE_H
#define TMR_OCTANT_TREE_H

#include <stdio.h>
#include "TMROctant.h"

/*
  The main octree class. 

  This is used to create, balance, and coarsen an octree as well as
  create a mesh from the octree data. This data can be used to
  construct meshes in TACS (or other finite-element codes). The octree
  is well-suited for creating adaptive meshes for topology
  optimization problems on regular domains.

  The octree object provides a means to construct inter-grid
  operators. The inter-grid operations are constructed from the nodes
  on the coarse mesh. For each node, we store the largest adjoining
  element level. Taking the next-nearest neighbours for the largest
  adjoining element length gives an intermesh operator that is
  guaranteed to be the independent node set. The regular full
  weighting multigrid scheme can be used. The fine and coarse meshes
  are given as follows:

  Fine:                         Coarse:

  o -- o -- o -- o -- o         o ------- o ------- o
  |    |    |    |    |         |         |         |
  o -- o -- o -- o -- o         |         |         |
  |    |    |    |    |         |         |         |
  o -- o -- o -- o -- o         o ------- o ------- o
  |         |    |    |         |         |         |
  |         o -- o -- o         |         |         |
  |         |    |    |         |         |         |
  o ------- o -- o -- o         o ------- o ------- o
  |         |         |         |                   |
  |         |         |         |                   |
  |         |         |         |                   |
  o ------- o ------- o         |                   |
  |         |         |         |                   |
  |         |         |         |                   |
  |         |         |         |                   |
  o ------- o ------- o         o ----------------- o

  Note that these meshes are 2-1 balanced, otherwise this type of
  construction scheme could not be used. There will be at most a
  single level of refinement difference between adjacent cells.
*/
class TMROctree {
 public:
  TMROctree( int refine_level, 
             TMROctant *_domain=NULL, int ndomain=0 );
  TMROctree( int nrand, int min_level, int max_level );
  TMROctree( TMROctantArray *_list,
             TMROctant *_domain=NULL, int ndomain=0 );
  ~TMROctree();

  // Check if the provided octant is within the domain
  // -------------------------------------------------
  int inDomain( TMROctant *p );
  int onBoundary( TMROctant *p );

  // Refine the octree
  // -----------------
  void refine( int refinement[],
	       int min_level=0, int max_level=TMR_MAX_LEVEL );

  // Balance the tree to ensure 2-1 balancing
  // ----------------------------------------
  void balance( int balance_corner=0 );

  // Coarsen the tree uniformly
  // --------------------------
  TMROctree *coarsen();

  // Find an octant that completely encloses the provided octant
  // -----------------------------------------------------------
  TMROctant *findEnclosing( TMROctant *oct );
  void findEnclosingRange( TMROctant *oct,
			   int *low, int *high );

  // Create the connectivity information for the mesh
  // ------------------------------------------------
  void createMesh( int _order );

  // Order the nodes but do not create the connectivity
  // --------------------------------------------------
  void createNodes( int _order );

  // Retrieve the mesh information
  // -----------------------------
  void getMesh( int *_num_nodes, 
                int *_num_elements, 
                const int **_elem_ptr, 
                const int **_elem_conn );

  // Retrieve the depednent node information
  // ---------------------------------------
  void getDependentMesh( int *_num_dep_nodes, 
                         const int **_dep_ptr,
                         const int **_dep_conn,
                         const double **_dep_weights );

  // Create the interpolation from a coarser mesh
  // --------------------------------------------
  void createInterpolation( TMROctree *coarse,
                            int **_interp_ptr,
                            int **_interp_conn,
                            double **_interp_weights );
  void createRestriction( TMROctree *coarse,
                          int **_interp_ptr,
                          int **_interp_conn,
                          double **_interp_weights );

  // Print a representation of the tree to a file
  // --------------------------------------------
  void printOctree( const char *filename );

  // Retrieve the octree elements
  // ----------------------------
  void getElements( TMROctantArray **_elements ){
    if (_elements){ *_elements = elements; }
  }

  // Retrieve the octree nodes
  // -------------------------
  void getNodes( TMROctantArray **_nodes ){
    if (_nodes){ *_nodes = nodes; }
  }

 private:
  // The list octants representing the elements
  TMROctantArray *elements; 

  // The nodes within the element mesh
  TMROctantArray *nodes; 

  // A list of octants that define the domain
  int ndomain;
  TMROctant *domain;

  // Store the order of the mesh
  int order;

  // The number of elements and independent and dependent nodes
  int num_elements;
  int num_nodes, num_dependent_nodes;

  // The mesh connectivity
  int *elem_ptr, *elem_conn;
  
  // The dependent mesh connectivity
  int *dep_ptr, *dep_conn;
  double *dep_weights;
};

#endif // TMR_OCTANT_TREE_H
