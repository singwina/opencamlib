/*  $Id$
 * 
 *  Copyright 2010 Anders Wallin (anders.e.e.wallin "at" gmail.com)
 *  
 *  This file is part of OpenCAMlib.
 *
 *  OpenCAMlib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCAMlib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OpenCAMlib.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OCTREE2_H
#define OCTREE2_H

#include <iostream>
#include <list>

#include "point.h"
#include "volume.h"
#include "triangle.h"
#include "bbox.h"

namespace ocl
{

/// octree node
class Octnode {
    public:
        Octnode(){};
        /// create suboctant idx of parent with scale nodescale and depth nodedepth
        Octnode(Octnode* parent, unsigned int idx, double nodescale, unsigned int nodedepth);
        virtual ~Octnode();
        /// create all eight children of this node
        void subdivide(); // create children
        /// return center of child with index n
        Point* childcenter(int n); // return position of child centerpoint
        /// set the vertices of this node
        void setvertices(); // set vertices[]
        /// evaluate the vol.dist() function for this node
        void evaluate(const OCTVolume* vol);
        /// a list of triangles for this node generated by marching-cubes
        std::vector<Triangle> mc_triangles();
        /// a list of side-triangles
        std::vector<Triangle> side_triangles();
        /// interpolate a point between vertex idx1 and idx2. used by marching-cubes
        Point interpolate(int idx1, int idx2);
        
        // python interface
        /// return vertices to python
        boost::python::list py_get_vertices() const;
        /// return center of this node
        Point py_get_center() const;
        
    // DATA
        /// pointers to child nodes
        std::vector<Octnode*> child;
        /// number of children
        unsigned int childcount;
        /// pointer to parent node
        Octnode* parent;
        /// The eight corners of this node
        std::vector<Point*> vertex; 
        /// value of implicit function at vertex
        std::vector<double> f; 
        /// flag set true if this node is outside
        bool outside;
        /// flag for inside node
        bool inside;
        /// bool-array to indicate surface status of the faces
        std::vector<bool> surface; // flag for surface triangles FIXME!
        /// the center point of this node
        Point* center; // the centerpoint of this node
        /// the tree-dept of this node
        unsigned int depth; // depth of node
        /// the index of this node [0,7]
        unsigned int idx; // index of node
        /// the scale of this node, i.e. distance from center out to corner vertices
        double scale; // distance from center to vertices
        /// flag for checking if evaluate() has run
        bool evaluated;
        /// bounding-box corresponding to this node
        Bbox bb;
        /// vector for storing marching-cubes triangles
        std::vector<Triangle> mc_tris;
        /// flag for telling if mc-triangles have been calculated and are valid
        bool mc_tris_valid;
        /// the direction to the vertices, from the center 
        static Point direction[8];
        /// Marching-Cubes edge table
        static const unsigned int edgeTable[256];
        /// Marching-Cubes triangle table
        static const int triTable[256][16];
        /// string repr
        friend std::ostream& operator<<(std::ostream &stream, const Octnode &o);
        /// string repr
        std::string str() const;
    private:
        /// print out surfaces
        void print_surfaces();
        /// inherit the surface-property from a parent 
        void set_surfaces();
};

/// octree class which stores the root-node and allows operations on the tree
class Octree {
    public:
        Octree() { assert(0); };
        virtual ~Octree();
        /// create an octree with a root node with scale=root_scale, maximum
        /// tree-depth of max_depth and centered at centerp.
        Octree(double root_scale, unsigned int max_depth, Point& centerp);
        /// subtract vol from tree
        void diff_negative_root(const OCTVolume* vol);
        /// return the leaf-nodes
        void get_leaf_nodes(Octnode* current, std::vector<Octnode*>& nodelist) const;
        /// return all nodes in tree
        void get_all_nodes(Octnode* current, std::vector<Octnode*>& nodelist) const;
        /// run marching-cubes on the tree
        std::vector<Triangle> mc();
        /// generate the side-triangles
        std::vector<Triangle> side_triangles();
        /// initialize by recursively calling subdivide() on all nodes n times
        void init(const unsigned int n);
        /// return the maximum depth of the tree
        unsigned int get_max_depth() const;
        /// return the root scale
        double get_root_scale() const;
        /// return the leaf scale (the minimum resolution of the tree)
        double leaf_scale() const;
        /// string output
        std::string str() const;
        
    // python interface
        /// return python-list of leaf nodes
        boost::python::list py_get_leaf_nodes() const;
        /// return python-list of marching-cubes triangles
        boost::python::list py_mc_triangles(); 
        /// return python-list of side-trianges
        boost::python::list py_s_triangles(); 
        
    
    private:
        /// recursively traverse the tree subtracting vol
        void diff_negative(Octnode* current, const OCTVolume* vol);
    // DATA
        /// the root scale
        double root_scale;
        /// the maximum tree-depth
        unsigned int max_depth;
        /// pointer to the root node
        Octnode* root;
};

} // end namespace
#endif
// end file octree2.h
