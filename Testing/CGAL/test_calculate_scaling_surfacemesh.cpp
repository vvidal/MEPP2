#include <fstream>
#include <string>

#include <CGAL/Surface_mesh.h>
#include <CGAL/Cartesian.h>

#include "FEVV/Wrappings/Geometry_traits_cgal_surface_mesh.h"

#include "FEVV/Filters/Generic/scaling.hpp"

using namespace FEVV;
using namespace FEVV::Filters;

void
test_calculate_scaling_surface_mesh(std::string filename)
{
  std::ifstream in(filename);
  if(!in)
  {
    std::cout << "Unable to read file " << filename << std::endl;
    exit(EXIT_FAILURE);
  }

  typedef CGAL::Cartesian< double > Kernel;
  typedef CGAL::Surface_mesh< Kernel::Point_3 > Mesh;
  typedef Geometry_traits< Mesh > Geometry;
  typedef Geometry::Scalar Scalar;

  Mesh m;
  in >> m;
  auto pos_pm = get(boost::vertex_point, m);
  calculate_scaling(m, pos_pm, Scalar(2.0), Scalar(3.0), Scalar(4.0));

  std::cout << "Done." << std::endl;
}

int
main(int narg, char **argv)
{
  if(narg < 2)
  {
    std::cout << "Usage: " << argv[0]
              << " filename; filename being an off file." << std::endl;
    exit(EXIT_FAILURE);
  }

  test_calculate_scaling_surface_mesh(argv[1]);
  return 0;
}
