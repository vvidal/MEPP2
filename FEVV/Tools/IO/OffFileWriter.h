#ifndef __OffFileWriter_h
#define __OffFileWriter_h

/*
 * Imported from
 *    https://github.com/MEPP-team/Gharial/blob/bb01259715c325baab0d8806604e7c8898a4e420/Src/DataStructures/IO_Tools/STLoffWriter.cxx
 * and
 *    https://github.com/MEPP-team/Gharial/blob/a0fb8e078d35023b578a84d5578035606851b9ca/Src/DataStructures/IO_Tools/STLoffWriter.h
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>

#include "FEVV/Tools/IO/StringUtilities.hpp"
#include "FEVV/Tools/IO/FileUtilities.hpp"

/*
 * OFF format description is available at
 *   http://www.geomview.org/docs/html/OFF.html#OFF
 * and
 *   http://people.sc.fsu.edu/~jburkardt/data/off/off.html
 */

namespace FEVV {
namespace IO {

using namespace StrUtils;
using namespace FileUtils;

/**
 * Write mesh data to OFF file.
 */
template< typename CoordType,
          typename CoordNType,
          typename CoordTType,
          typename CoordCType,
          typename IndexType >
void
write_off_file(std::string file_path,
               std::vector< std::vector< CoordType > > &points_coords,
               std::vector< std::vector< CoordNType > > &normals_coords,
               std::vector< std::vector< CoordTType > >
                   &texture_coords, // vertex texture coord only
               std::vector< std::vector< CoordCType > > &vertex_color_coords,
               std::vector< std::vector< IndexType > > &face_indices,
               std::vector< std::vector< CoordCType > > &face_color_coords,
               unsigned long nb_total_edges = 0)
{
  const bool vertices_have_color =
                 (points_coords.size() == vertex_color_coords.size()),
             vertices_have_normals =
                 (points_coords.size() == normals_coords.size()),
             vertices_have_tex_coord =
                 (points_coords.size() == texture_coords.size()),
             faces_have_colors =
                 (face_indices.size() == face_color_coords.size());

  std::ofstream file(file_path);

  if(file.is_open())
  {
    unsigned long nb_writen_vertices = 0,
                  nb_total_vertices =
                      static_cast< unsigned long >(points_coords.size()),
                  nb_total_faces =
                      static_cast< unsigned long >(face_indices.size());

    std::string mystring;
    if(vertices_have_color)
    {
      if(vertices_have_normals)
      {
        if(vertices_have_tex_coord)
          mystring = "STCNOFF";
        else
          mystring = "CNOFF";
      }
      else
      {
        if(vertices_have_tex_coord)
          mystring = "STCOFF";
        else
          mystring = "COFF";
      }
    }
    else
    {
      if(vertices_have_normals)
      {
        if(vertices_have_tex_coord)
          mystring = "STNOFF";
        else
          mystring = "NOFF";
      }
      else
      {
        if(vertices_have_tex_coord)
          mystring = "STOFF";
        else
          mystring = "OFF";
      }
    }
    file << mystring << std::endl;

    if(nb_total_edges == 0)
      nb_total_edges = nb_total_vertices + nb_total_faces - 2;
    // this number may be incorrect but it is not a problem since
    // most OFF readers do not use it

    file << nb_total_vertices << " " << nb_total_faces << " " << nb_total_edges
         << std::endl;
    file << "# #vertices #faces #edges" << std::endl;
    file << "# generated by MEPP2 software" << std::endl;

    // VERTICES

    typename std::vector< std::vector< CoordType > >::const_iterator it(
        points_coords.begin()),
        ite(points_coords.end());
    typename std::vector< std::vector< CoordNType > >::const_iterator itn(
        normals_coords.begin());
    typename std::vector< std::vector< CoordCType > >::const_iterator itc(
        vertex_color_coords.begin());
    typename std::vector< std::vector< CoordTType > >::const_iterator itt(
        texture_coords.begin());
    typename std::vector< CoordType >::const_iterator itv, itve;
    typename std::vector< CoordNType >::const_iterator itvn, itvne;
    typename std::vector< CoordCType >::const_iterator itvc, itvce;
    typename std::vector< CoordTType >::const_iterator itvt, itvte;
    for(; it != ite; ++it)
    {
      // vertex position
      for(itv = it->begin(), itve = it->end(); itv != itve; ++itv)
        file << *itv << "  ";

      if(vertices_have_normals)
      {
        for(itvn = itn->begin(), itvne = itn->end(); itvn != itvne; ++itvn)
          file << *itvn << "  ";
        ++itn;
      }

      if(vertices_have_color)
      {
        for(itvc = itc->begin(), itvce = itc->end(); itvc != itvce; ++itvc)
          file << *itvc << "  ";
        ++itc;
      }

      if(vertices_have_tex_coord)
      {
        for(itvt = itt->begin(), itvte = itt->end(); itvt != itvte; ++itvt)
          file << *itvt << "  ";
        ++itt;
      }

      file << std::endl;
      nb_writen_vertices++;
    }

    assert(nb_writen_vertices == nb_total_vertices);

    // INDEXED FACES

    typename std::vector< std::vector< IndexType > >::const_iterator itf(
        face_indices.begin()),
        itfe(face_indices.end());
    typename std::vector< std::vector< CoordCType > >::const_iterator itfc(
        face_color_coords.begin());

    typename std::vector< IndexType >::const_iterator itfindex, itfindexe;
    typename std::vector< CoordCType >::const_iterator itffc, itffce;
    for(; itf != itfe; ++itf)
    {
      file << itf->size() << "  ";

      for(itfindex = itf->begin(), itfindexe = itf->end();
          itfindex != itfindexe;
          ++itfindex)
        file << *itfindex << " ";

      if(faces_have_colors)
      {
        file << "  ";
        for(itffc = itfc->begin(), itffce = itfc->end(); itffc != itffce;
            ++itffc)
          file << *itffc << " ";
        ++itfc;
      }

      file << std::endl;
    }

    file.close();
  }
  else
  {
    throw std::runtime_error("writer_off_file: failed to open output file.");
  }
}

} // namespace IO
} // namespace FEVV

#endif // __OffFileWriter_h
