// Copyright (c) 2012-2019 University of Lyon and CNRS (France).
// All rights reserved.
//
// This file is part of MEPP2; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 3 of
// the License, or (at your option) any later version.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#pragma once


#include "FEVV/Wrappings/properties.h"

#include <iostream>
#include <type_traits> // for std::remove_reference


/**
 * Convert color value given in [0; 255]
 * General case, casting is enough.
 */
template< typename OutT >
OutT convert_color_value(uint8_t color_0_255)
{
  return static_cast< OutT >(color_0_255);
}

/**
 * Convert color value in [0; 255] to color value in [0.0; 1.0] (float).
 */
template<>
inline
float convert_color_value< float >(uint8_t color_0_255)
{
  return color_0_255 / 255.0f;
}

/**
 * Convert color value in [0; 255] to color value in [0.0; 1.0] (double).
 */
template<>
inline
double convert_color_value< double >(uint8_t color_0_255)
{
  return color_0_255 / 255.0;
}

/**
 * Convert color value given in [0.0; 1.0].
 * General case, casting is enough.
 */
template< typename OutT >
OutT convert_color_value(double color_0_1)
{
  return static_cast< OutT >(color_0_1);
}

/**
 * Convert color value in [0.0; 1.0] to color value in [0; 255].
 */
template<>
inline
uint8_t convert_color_value< uint8_t >(double color_0_1)
{
  // note: applies also to color_0_1 of type float
  return static_cast< uint8_t >(color_0_1 * 255.0);
}


namespace FEVV {
namespace Filters {


/**
 * \brief  Copy a source point cloud into a target point cloud.
 *         Copy standard properties too.
 *
 * \param  pc_s      the point cloud to copy from
 * \param  pmaps_s   the source property maps bag
 * \param  pc_t      the point cloud to copy to
 * \param  pmaps_t   the target property maps bag
 * \param  gt_s      the geometry traits of source point cloud
 * \param  gt_t      the geometry traits of target point cloud
 *
 * \sa     the simplified variant that use the default geometry traits.
 */
//TODO-elo: return v2v maps
template< typename PointCloudS,
          typename PointCloudT,
          typename GeometryTraitsS,
          typename GeometryTraitsT >
void
copy_point_cloud(const PointCloudS     &pc_s,
                 const PMapsContainer  &pmaps_s,
                 PointCloudT           &pc_t,
                 PMapsContainer        &pmaps_t,
                 const GeometryTraitsS &gt_s,
                 const GeometryTraitsT &gt_t)
{
  // target types
  typedef  typename GeometryTraitsT::Scalar  TScalar;
  typedef  typename GeometryTraitsT::Point   TPoint;
  typedef  typename GeometryTraitsT::Vector  TVector;
  typedef  decltype(make_property_map(FEVV::vertex_color, pc_t))  TColorMap;
  typedef  typename TColorMap::value_type    TColor;
  // note: we need the Color type because it is not a Vector with all
  //       datastructures ; the easiest way to define the Color type is
  //       "the type that is stored in the Color map", hence the declaration
  //       above

  // detect which standard properties are used by source
  bool use_vertex_normal = has_map(pmaps_s, vertex_normal);
  bool use_vertex_color  = has_map(pmaps_s, vertex_color);

  // retrieve point maps
  auto s_point_pm = get(boost::vertex_point, pc_s);
  auto t_point_pm = get(boost::vertex_point, pc_t);

  // create needed property maps in target
  if(use_vertex_normal)
  {
    auto vn_pm = make_property_map(FEVV::vertex_normal, pc_t);
    put_property_map(FEVV::vertex_normal, pc_t, pmaps_t, vn_pm);
  }
  if(use_vertex_color)
  {
    auto vc_pm = make_property_map(FEVV::vertex_color, pc_t);
    put_property_map(FEVV::vertex_color, pc_t, pmaps_t, vc_pm);
  }

  // copy geometry and properties
  auto iterator_pair = vertices(pc_s);
  auto s_vi = iterator_pair.first;
  auto s_vi_end = iterator_pair.second;
  for(; s_vi != s_vi_end; ++s_vi)
  {
    // create new vertex in target
    auto v = add_vertex(pc_t);

    // copy geometry
    auto s_point = get(s_point_pm, *s_vi);
    auto x = gt_s.get_x(s_point);
    auto y = gt_s.get_y(s_point);
    auto z = gt_s.get_z(s_point);
    put(t_point_pm, v, TPoint(static_cast< TScalar >(x),
                              static_cast< TScalar >(y),
                              static_cast< TScalar >(z)));

    // copy properties
    if(use_vertex_normal)
    {
      // vertex normal
      auto source_pm = get_property_map(FEVV::vertex_normal, pc_s, pmaps_s);
      auto target_pm = get_property_map(FEVV::vertex_normal, pc_t, pmaps_t);
      auto normal    = get(source_pm, *s_vi);
      put(target_pm, v, TVector(static_cast< TScalar >(normal[0]),
                                static_cast< TScalar >(normal[1]),
                                static_cast< TScalar >(normal[2])));
    }

    if(use_vertex_color)
    {
      // vertex color
      auto source_pm = get_property_map(FEVV::vertex_color, pc_s, pmaps_s);
      auto target_pm = get_property_map(FEVV::vertex_color, pc_t, pmaps_t);
      auto color     = get(source_pm, *s_vi);

      TColor t_color;
      typedef  typename std::remove_reference< decltype(t_color[0]) >::type
                                                              TColorValueType;

      put(target_pm,
          v,
          TColor(convert_color_value< TColorValueType >(color[0]),
                 convert_color_value< TColorValueType >(color[1]),
                 convert_color_value< TColorValueType >(color[2])));
    }
  }

  // display some information

  std::cout << "copy_point_cloud(): input point_cloud has "
            << size_of_vertices(pc_s) << " vertices, "
            << std::endl;
  std::cout << "copy_point_cloud(): input point_cloud has property maps: [";
  for(auto &name: list_property_maps(pmaps_s))
    std::cout << " " << name;
  std::cout << " ]" << std::endl;

  std::cout << "copy_point_cloud(): output point_cloud has "
            << size_of_vertices(pc_t) << " vertices, "
            << std::endl;
  std::cout << "copy_point_cloud(): output point_cloud has property maps: [";
  for(auto &name: list_property_maps(pmaps_t))
    std::cout << " " << name;
  std::cout << " ]" << std::endl;
}


/**
 * \brief  Copy a source point cloud into a target point cloud.
 *         Copy standard properties too.
 *
 * \param  pc_s      the point cloud to copy from
 * \param  pmaps_s   the source property maps bag
 * \param  pc_t      the point cloud to copy to
 * \param  pmaps_t   the target property maps bag
 *
 * \sa     the variant that use the geometry traits provided by the user.
 */
template< typename PointCloudS,
          typename PointCloudT,
          typename GeometryTraitsS = FEVV::Geometry_traits< PointCloudS >,
          typename GeometryTraitsT = FEVV::Geometry_traits< PointCloudT > >
void
copy_point_cloud(const PointCloudS     &pc_s,
                 const PMapsContainer  &pmap_s,
                 PointCloudT           &pc_t,
                 PMapsContainer        &pmap_t)
{
  GeometryTraitsS gt_s(pc_s);
  GeometryTraitsT gt_t(pc_t);
  copy_point_cloud(pc_s, pmap_s, pc_t, pmap_t, gt_s, gt_t);
}


} // namespace Filters
} // namespace FEVV
