#ifndef FEVV_GRAPH_TRAITS_EXTENSION_CGAL_SURFACE_MESH_H
#define FEVV_GRAPH_TRAITS_EXTENSION_CGAL_SURFACE_MESH_H

/*
 * Specialization of graph traits extension
 * for CGAL::Surface_mesh
 */

#include "Graph_traits_extension.h"
#include <CGAL/boost/graph/graph_traits_Surface_mesh.h>

namespace FEVV {


/**
 * \brief  Real current number of vertices of the mesh.
 *         Specialization for CGAL::Surface_mesh.
 *
 * \param  g   mesh
 *
 * \return  real number of vertices of the mesh
 */
template< typename PointT >
typename boost::graph_traits< CGAL::Surface_mesh< PointT > >::vertices_size_type
size_of_vertices(const CGAL::Surface_mesh< PointT > &g)
{
  return g.number_of_vertices();
}


/**
 * \brief  Real current number of edges of the mesh.
 *         Specialization for CGAL::Surface_mesh.
 *
 * \param  g   mesh
 *
 * \return  real number of edges of the mesh
 */
template< typename PointT >
typename boost::graph_traits< CGAL::Surface_mesh< PointT > >::edges_size_type
size_of_edges(const CGAL::Surface_mesh< PointT > &g)
{
  return g.number_of_edges();
}


/**
 * \brief  Real current number of halfedges of the mesh.
 *         Specialization for CGAL::Surface_mesh.
 *
 * \param  g   mesh
 *
 * \return  real number of halfedges of the mesh
 */
template< typename PointT >
typename boost::graph_traits<
    CGAL::Surface_mesh< PointT > >::halfedges_size_type
size_of_halfedges(const CGAL::Surface_mesh< PointT > &g)
{
  return g.number_of_halfedges();
}


/**
 * \brief  Real current number of faces of the mesh.
 *         Specialization for CGAL::Surface_mesh.
 *
 * \param  g   mesh
 *
 * \return  real number of faces of the mesh
 */
template< typename PointT >
typename boost::graph_traits< CGAL::Surface_mesh< PointT > >::faces_size_type
size_of_faces(const CGAL::Surface_mesh< PointT > &g)
{
  return g.number_of_faces();
}


} // namespace FEVV


// ELO note: Clear mesh
// see
//   http://doc.cgal.org/latest/Surface_mesh/classCGAL_1_1Surface__mesh.html#a247d4ad3e6b106ae22e5306203812642
//
//   void clear()
//   removes all vertices, halfedge, edges and faces. Collects garbage and
//   clears all properties.
// see
//   CGAL-4.9/include/CGAL/boost/graph/helpers.h
//


#endif //  FEVV_GRAPH_TRAITS_EXTENSION_CGAL_SURFACE_MESH_H
