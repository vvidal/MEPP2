#pragma once

#include <boost/graph/graph_traits.hpp>
#include "FEVV/Operators/AIF/topology_predicates.hpp"

#include <vector>
#include <set>

namespace FEVV {
namespace Operators {

/**
 * \ingroup AIFFilters
 * \brief Replace the old_edge incident edge of face by the new_edge if old_edge
 *        and new_edge were similar/parallel edges.
 *
 * \tparam  MutableFaceIncidentGraph a Mesh type that provides a Model of the
 *          MutableFaceIncidentGraph Concept through a boost::graph_traits<>
 *          specialization.
 * \param   face The mesh face to consider.
 * \param   old_edge An incident edge of the face.
 * \param   new_edge An edge to replace old_edge in the face' incident edges.
 * \param   g The MutableFaceIncidentGraph instance.
 */
template< typename MutableFaceIncidentGraph >
static void
replace_similar_edges(typename boost::graph_traits<
                          MutableFaceIncidentGraph >::face_descriptor face,
                      const typename boost::graph_traits<
                          MutableFaceIncidentGraph >::edge_descriptor old_edge,
                      const typename boost::graph_traits<
                          MutableFaceIncidentGraph >::edge_descriptor new_edge,
                      MutableFaceIncidentGraph &g)
{
  if(Operators::are_similar_edges(old_edge, new_edge, g))
  {
    auto edges_range_pair = out_edges(face, g);
    auto iter_e = edges_range_pair.first;
    for(; iter_e != edges_range_pair.second; ++iter_e)
      if(*iter_e == old_edge)
        *iter_e = new_edge;
  }
}

/**
 * \ingroup AIFFilters
 * \brief For all faces incident to e_to_remove, if e_to_keep and e_to_remove
 *        were similar/parallel edges then these faces are made incident to
 *        e_to_keep by replacing e_to_remove by e_to_keep among their incident
 *        edges. Then each face is added to e_to_keep' incident faces.
 *
 * \tparam  MutableFaceIncidentGraph a Mesh type that provides a Model of the
 *          MutableFaceIncidentGraph Concept through a boost::graph_traits<>
 *          specialization.
 * \param   e_to_keep An edge used to replace e_to_remove.
 * \param   e_to_remove An edge whose each occurence is replaced by e_to_keep.
 * \param   g The MutableFaceIncidentGraph instance.
 */
template< typename MutableFaceIncidentGraph >
static void
merge_similar_edges(
    typename boost::graph_traits< MutableFaceIncidentGraph >::edge_descriptor
        e_to_keep,
    typename boost::graph_traits< MutableFaceIncidentGraph >::edge_descriptor
        e_to_remove,
    MutableFaceIncidentGraph &g)
{
  auto faces_range_pair = in_edges(e_to_remove, g);
  auto iter_f = faces_range_pair.first;
  for(; iter_f != faces_range_pair.second;
      ++iter_f) // for each face *iterF incident to e_to_remove
  {
    // each time e_to_remove appears into a face *iterF, it is replaced by
    // e_to_keep
    replace_similar_edges(*iter_f, e_to_remove, e_to_keep, g);
    // the kept edge must be linked to former faces incident to e_to_remove
    add_in_edge(e_to_keep, *iter_f);
  }
}

/**
 * \ingroup AIFFilters
 * \brief Remove/resolve similar/parallel edges for all incident edges of
 * v_to_keep vertex.
 *
 * \tparam  MutableFaceIncidentGraph a Mesh type that provides a Model of the
 *          MutableFaceIncidentGraph Concept through a boost::graph_traits<>
 *          specialization.
 * \param   v_to_keep A vertex whose incident similar/paralle edges are
 *          resolved.
 * \param   g The MutableFaceIncidentGraph instance.
 */
template< typename MutableFaceIncidentGraph >
static void
resolve_similar_incident_edges(
    typename boost::graph_traits< MutableFaceIncidentGraph >::vertex_descriptor
        v_to_keep,
    MutableFaceIncidentGraph &g)
{
  typedef
      typename boost::graph_traits< MutableFaceIncidentGraph >::edge_descriptor
          edge_descriptor;
  std::vector< edge_descriptor > edges_to_remove;
  /////////////////////////////////////////////////////////////////////////////
  auto edges_range_pair = in_edges(v_to_keep, g);
  auto iter_e = edges_range_pair.first;
  for(; iter_e != edges_range_pair.second; ++iter_e)
  {
    auto iter_e_bis = iter_e;
    ++iter_e_bis;
    for(; iter_e_bis != edges_range_pair.second; ++iter_e_bis)
    {
      if(Operators::are_similar_edges(*iter_e, *iter_e_bis, g))
      {
        edges_to_remove.push_back(*iter_e_bis);
        merge_similar_edges(*iter_e, *iter_e_bis, g);
      }
    }
  }
  /////////////////////////////////////////////////////////////////////////////
  typename std::vector< edge_descriptor >::iterator it_e(
      edges_to_remove.begin()),
      it_ee(edges_to_remove.end());
  for(; it_e != it_ee; ++it_e)
    remove_edge(*it_e, g);
  edges_to_remove.clear();
}

/**
 * \ingroup AIFFilters
 * \brief Remove/resolve similar faces for all incident faces of v_to_keep
 *        vertex.
 *
 * \tparam  MutableFaceIncidentGraph a Mesh type that provides a Model of the
 *          MutableFaceIncidentGraph Concept through a boost::graph_traits<>
 *          specialization.
 * \param   v_to_keep A vertex whose incident similar faces are resolved.
 * \param   g The MutableFaceIncidentGraph instance.
 * \param   take_into_account_face_rientation A boolean to tell if the similar
 *          faces must have the same orientation.
 */
template< typename MutableFaceIncidentGraph >
static void
resolve_similar_incident_faces(
    typename boost::graph_traits< MutableFaceIncidentGraph >::vertex_descriptor
        v_to_keep,
    MutableFaceIncidentGraph &g,
    bool take_into_account_face_rientation = false)
{
  typedef
      typename boost::graph_traits< MutableFaceIncidentGraph >::face_descriptor
          face_descriptor;
  std::vector< face_descriptor > faces_to_remove;
  std::set< face_descriptor > incident_faces;
  /////////////////////////////////////////////////////////////////////////////
  auto edges_range_pair = in_edges(v_to_keep, g); // incident edges
  auto iter_e = edges_range_pair.first;
  for(; iter_e != edges_range_pair.second; ++iter_e)
  {
    auto faces_range_pair = in_edges(*iter_e, g); // incident faces
    incident_faces.insert(faces_range_pair.first, faces_range_pair.second);
  }
  /////////////////////////////////////////////////////////////////////////////
  auto iter_f = incident_faces.begin();
  for(; iter_f != incident_faces.end(); ++iter_f)
  {
    if(std::find(faces_to_remove.begin(), faces_to_remove.end(), *iter_f) !=
       faces_to_remove.end())
      continue;
    auto iter_f_bis = iter_f;
    ++iter_f_bis;
    for(; iter_f_bis != incident_faces.end(); ++iter_f_bis)
    {
      if(Operators::are_similar_faces(
             *iter_f, *iter_f_bis, g, take_into_account_face_rientation))
      {
        if(std::find(
               faces_to_remove.begin(), faces_to_remove.end(), *iter_f_bis) ==
           faces_to_remove
               .end()) // ensure to suppress each similar face only once
          faces_to_remove.push_back(*iter_f_bis);
      }
    }
  }
  /////////////////////////////////////////////////////////////////////////////
  typename std::vector< face_descriptor >::iterator it_f(
      faces_to_remove.begin()),
      it_fe(faces_to_remove.end());
  for(; it_f != it_fe; ++it_f)
    remove_face(*it_f, g); // REDUNDANT FACES ARE REMOVED
  faces_to_remove.clear();
}

} // namespace Operators
} // namespace FEVV
