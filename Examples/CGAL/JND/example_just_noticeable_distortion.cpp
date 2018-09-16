
#include "FEVV/Filters/Generic/generic_reader.hpp"
#include "FEVV/Filters/Generic/generic_writer.hpp"

#include "FEVV/DataStructures/DataStructures_cgal_surface_mesh.h"
#include "FEVV/Wrappings/Geometry_traits_cgal_surface_mesh.h"
#include "FEVV/Wrappings/properties_surface_mesh.h"

#include "FEVV/Filters/Generic/Manifold/JustNoticeableDistortion/just_noticeable_distortion.hpp"
#include "FEVV/Filters/Generic/minmax_map.h"
#include "FEVV/Filters/Generic/color_mesh.h"
#include "FEVV/Filters/Generic/calculate_face_normals.hpp"
#include "FEVV/Filters/Generic/Manifold/calculate_vertex_normals.hpp"

#pragma warning(push)
#pragma warning(                                                               \
    disable : 4715) // Disable a warning from boost, more info :
                    // https://stackoverflow.com/questions/47136503/boost-json-parser-warning-c4715
#include <boost/property_tree/ptree.hpp>
#pragma warning(pop)
#include <boost/property_tree/json_parser.hpp>
#include <time.h>
//---------------------------------------------------------
//                       Main
//---------------------------------------------------------

// Main: load a mesh, apply the filter, write the mesh
int
main(int narg, char **argv)
{
  typedef boost::graph_traits< FEVV::MeshSurface > GraphTraits;
  typedef typename GraphTraits::vertex_iterator vertex_iterator;
  typedef typename GraphTraits::vertex_descriptor vertex_descriptor;
  // input and output files
  // std::string inputFilePath =
  // "../../../../Testing/Data/CubeTriangleFaces.obj";
  std::string input_file_path = "sphere.obj";
  if(narg == 2 || narg == 3)
  {
    std::string reader = std::string(argv[1]);
    input_file_path = reader;
  }
  std::string output_file_path = "just_noticeable_distortion_filter.output.obj";

  int screen_width = 1920;
  int screen_height = 1080;
  double screen_size = 55.;
  double user_dist = 50.;
  int scene_width = 1080;
  double scene_fov = M_PI * 0.3333;
  int number_of_lights = 128;

  if(narg == 3)
  {
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(argv[2], pt);

    screen_width = pt.get< int >("screen_width", screen_width);
    screen_height = pt.get< int >("screen_height", screen_height);
    screen_size = pt.get< double >("screen_size", screen_size);
    user_dist = pt.get< double >("user_dist", user_dist);
    scene_width = pt.get< int >("scene_width", scene_width);
    scene_fov = M_PI * pt.get< double >("scene_fov", scene_fov);
    number_of_lights = pt.get< int >("number_of_lights", number_of_lights);
  }
  std::cout << "Confing used :" << std::endl;
  std::cout << "\tScreen : " << screen_width << " * " << screen_height << " - "
            << screen_size << std::endl;
  std::cout << "\tUser distance : " << user_dist << std::endl;
  std::cout << "\tScene : " << scene_width << " - " << scene_fov << std::endl;
  std::cout << "\tNumber of lights : " << number_of_lights << std::endl;


  ScreenParam screen(screen_width, screen_height, screen_size);
  UserParam user(user_dist);
  SceneParam scene(scene_width, scene_fov);
  // read mesh from file
  FEVV::MeshSurface m;
  FEVV::PMapsContainer pmaps_bag;
  FEVV::Filters::read_mesh(input_file_path, m, pmaps_bag);

  // retrieve point property map (aka geometry)
  auto pm = get(boost::vertex_point, m);
  // Note: the property maps must be extracted from the
  //       property maps bag, and explicitely passed as
  //       parameters to the filter, in order to make
  //       clear what property is used by the filter

  // retrieve or create vertex-color property map
  using VertexColorMap =
      typename FEVV::PMap_traits< FEVV::vertex_color_t,
                                  FEVV::MeshSurface >::pmap_type;
  VertexColorMap v_cm;
  if(has_map(pmaps_bag, FEVV::vertex_color))
  {
    std::cout << "use existing vertex-color map" << std::endl;
    v_cm = get_property_map(FEVV::vertex_color, m, pmaps_bag);
  }
  else
  {
    std::cout << "create vertex-color map" << std::endl;
    v_cm = make_property_map(FEVV::vertex_color, m);
    // store property map in property maps bag
    put_property_map(FEVV::vertex_color, m, pmaps_bag, v_cm);
  }


  // retrieve or create vertex-normal property map
  using FaceNormalMap =
      typename FEVV::PMap_traits< FEVV::face_normal_t,
                                  FEVV::MeshSurface >::pmap_type;
  FaceNormalMap f_nm;
  if(has_map(pmaps_bag, FEVV::face_normal))
  {
    std::cout << "use existing face-normal map" << std::endl;
    f_nm = get_property_map(FEVV::face_normal, m, pmaps_bag);
  }
  else
  {
    std::cout << "create face-normal map" << std::endl;
    f_nm = make_property_map(FEVV::face_normal, m);
    // store property map in property maps bag
    put_property_map(FEVV::face_normal, m, pmaps_bag, f_nm);
    FEVV::Filters::calculate_face_normals(m, pm, f_nm);
  }


  // retrieve or create vertex-normal property map
  using VertexNormalMap =
      typename FEVV::PMap_traits< FEVV::vertex_normal_t,
                                  FEVV::MeshSurface >::pmap_type;
  VertexNormalMap v_nm;
  if(has_map(pmaps_bag, FEVV::vertex_normal))
  {
    std::cout << "use existing vertex-normal map" << std::endl;
    v_nm = get_property_map(FEVV::vertex_normal, m, pmaps_bag);
  }
  else
  {
    std::cout << "create vertex-normal map" << std::endl;
    v_nm = make_property_map(FEVV::vertex_normal, m);

    put_property_map(FEVV::vertex_normal, m, pmaps_bag, v_nm);
    FEVV::Filters::calculate_vertex_normals(m, pm, f_nm, v_nm);
  }

  using JndTypeMap =
      typename FEVV::Vertex_pmap_traits< FEVV::MeshSurface, double >::pmap_type;
  JndTypeMap jnd_m;

  jnd_m = FEVV::make_vertex_property_map< FEVV::MeshSurface, double >(m);


  // apply filter
  clock_t t_start = clock();
  FEVV::Filters::just_noticeable_distortion_filter(m,
                                                   pm,
                                                   v_nm,
                                                   f_nm,
                                                   jnd_m,
                                                   screen,
                                                   user,
                                                   scene,
                                                   number_of_lights,
                                                   true,
                                                   false);

  std::cout << "Time used to compute JND : "
            << (double)(clock() - t_start) / CLOCKS_PER_SEC << std::endl;


  double max_jnd, min_jnd;

  int i = 0;
  double lut_courbure_clust[3 * 256];
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.515600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.531300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.546900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.562500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.578100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.593800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.609400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.625000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.640600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.656300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.671900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.687500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.703100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.718800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.734400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.750000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.765600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.781300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.796900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.812500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.828100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.843800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.859400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.875000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.890600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.906300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.921900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.937500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.953100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.968800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.984400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.015600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.031300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.046900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.062500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.078100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.093800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.109400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.125000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.140600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.156300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.171900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.187500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.203100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.218800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.234400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.250000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.265600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.281300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.296900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.312500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.328100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.343800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.359400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.375000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.390600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.406300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.421900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.437500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.453100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.468800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.484400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.500000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.515600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.531300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.546900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.562500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.578100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.593800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.609400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.625000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.640600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.656300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.671900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.687500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.703100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.718800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.734400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.750000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.765600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.781300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.796900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.812500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.828100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.843800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.859400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.875000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.890600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.906300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.921900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.937500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.953100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.968800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.984400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.015600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.031300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.984400;
  lut_courbure_clust[i++] = 0.046900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.968800;
  lut_courbure_clust[i++] = 0.062500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.953100;
  lut_courbure_clust[i++] = 0.078100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.937500;
  lut_courbure_clust[i++] = 0.093800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.921900;
  lut_courbure_clust[i++] = 0.109400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.906300;
  lut_courbure_clust[i++] = 0.125000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.890600;
  lut_courbure_clust[i++] = 0.140600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.875000;
  lut_courbure_clust[i++] = 0.156300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.859400;
  lut_courbure_clust[i++] = 0.171900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.843800;
  lut_courbure_clust[i++] = 0.187500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.828100;
  lut_courbure_clust[i++] = 0.203100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.812500;
  lut_courbure_clust[i++] = 0.218800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.796900;
  lut_courbure_clust[i++] = 0.234400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.781300;
  lut_courbure_clust[i++] = 0.250000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.765600;
  lut_courbure_clust[i++] = 0.265600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.750000;
  lut_courbure_clust[i++] = 0.281300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.734400;
  lut_courbure_clust[i++] = 0.296900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.718800;
  lut_courbure_clust[i++] = 0.312500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.703100;
  lut_courbure_clust[i++] = 0.328100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.687500;
  lut_courbure_clust[i++] = 0.343800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.671900;
  lut_courbure_clust[i++] = 0.359400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.656300;
  lut_courbure_clust[i++] = 0.375000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.640600;
  lut_courbure_clust[i++] = 0.390600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.625000;
  lut_courbure_clust[i++] = 0.406300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.609400;
  lut_courbure_clust[i++] = 0.421900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.593800;
  lut_courbure_clust[i++] = 0.437500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.578100;
  lut_courbure_clust[i++] = 0.453100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.562500;
  lut_courbure_clust[i++] = 0.468800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.546900;
  lut_courbure_clust[i++] = 0.484400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.531300;
  lut_courbure_clust[i++] = 0.500000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.515600;
  lut_courbure_clust[i++] = 0.515600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.500000;
  lut_courbure_clust[i++] = 0.531300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.484400;
  lut_courbure_clust[i++] = 0.546900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.468800;
  lut_courbure_clust[i++] = 0.562500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.453100;
  lut_courbure_clust[i++] = 0.578100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.437500;
  lut_courbure_clust[i++] = 0.593800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.421900;
  lut_courbure_clust[i++] = 0.609400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.406300;
  lut_courbure_clust[i++] = 0.625000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.390600;
  lut_courbure_clust[i++] = 0.640600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.375000;
  lut_courbure_clust[i++] = 0.656300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.359400;
  lut_courbure_clust[i++] = 0.671900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.343800;
  lut_courbure_clust[i++] = 0.687500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.328100;
  lut_courbure_clust[i++] = 0.703100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.312500;
  lut_courbure_clust[i++] = 0.718800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.296900;
  lut_courbure_clust[i++] = 0.734400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.281300;
  lut_courbure_clust[i++] = 0.750000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.265600;
  lut_courbure_clust[i++] = 0.765600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.250000;
  lut_courbure_clust[i++] = 0.781300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.234400;
  lut_courbure_clust[i++] = 0.796900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.218800;
  lut_courbure_clust[i++] = 0.812500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.203100;
  lut_courbure_clust[i++] = 0.828100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.187500;
  lut_courbure_clust[i++] = 0.843800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.171900;
  lut_courbure_clust[i++] = 0.859400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.156300;
  lut_courbure_clust[i++] = 0.875000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.140600;
  lut_courbure_clust[i++] = 0.890600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.125000;
  lut_courbure_clust[i++] = 0.906300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.109400;
  lut_courbure_clust[i++] = 0.921900;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.093800;
  lut_courbure_clust[i++] = 0.937500;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.078100;
  lut_courbure_clust[i++] = 0.953100;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.062500;
  lut_courbure_clust[i++] = 0.968800;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.046900;
  lut_courbure_clust[i++] = 0.984400;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.031300;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.015600;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.984400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.968800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.953100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.937500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.921900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.906300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.890600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.875000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.859400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.843800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.828100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.812500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.796900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.781300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.765600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.750000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.734400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.718800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.703100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.687500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.671900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.656300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.640600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.625000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.609400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.593800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.578100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.562500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.546900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.531300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.515600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.500000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.484400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.468800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.453100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.437500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.421900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.406300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.390600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.375000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.359400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.343800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.328100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.312500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.296900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.281300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.265600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.250000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.234400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.218800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.203100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.187500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.171900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.156300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.140600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.125000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.109400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.093800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.078100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.062500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.046900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.031300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.015600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 1.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.984400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.968800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.953100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.937500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.921900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.906300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.890600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.875000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.859400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.843800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.828100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.812500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.796900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.781300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.765600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.750000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.734400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.718800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.703100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.687500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.671900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.656300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.640600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.625000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.609400;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.593800;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.578100;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.562500;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.546900;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.531300;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.515600;
  lut_courbure_clust[i++] = 0.000000;
  lut_courbure_clust[i++] = 0.000000;


  FEVV::Filters::compute_min_max_vertices(m, jnd_m, min_jnd, max_jnd);

  FEVV::Filters::color_vertices_from_map(
      m, jnd_m, v_cm, min_jnd, max_jnd, lut_courbure_clust, 255);
  // write mesh to file
  int counter = 0;
  BOOST_FOREACH(vertex_descriptor vd, m.vertices())
  {
    double jnd = get(jnd_m, vd);
    std::cout << "vertex #" << ++counter;
    std::cout << " - Jnd= " << jnd << std::endl;
  }
  FEVV::Filters::write_mesh(output_file_path, m, pmaps_bag);


  return 0;
}
