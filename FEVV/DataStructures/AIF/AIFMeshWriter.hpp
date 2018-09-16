#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "FEVV/DataStructures/AIF/AIFMesh.hpp"

#include "FEVV/Tools/IO/MeshWriterInterface.h"

namespace FEVV {
namespace DataStructures {
namespace AIF {


/**
 * \class	AIFMeshWriter
 * \brief	This class represents an AIFMesh object writer.
 *			An AIFMeshWriter writes a mesh file (.obj, .off, .vtk, .vtp,
 *.vtu) using an input AIFMesh object. Note that some advanced properties (e.g.
 *multi-texturing) may have not yet been implemented. \see		AIFMesh
 */
class AIFMeshWriter : public MeshWriterInterface< AIFMesh >
{

public:
  typedef AIFMeshWriter self;
  typedef MeshWriterInterface< AIFMesh > superclass;
  typedef superclass::input_type input_type;
  typedef superclass::ptr_input ptr_input;
  typedef boost::shared_ptr< self > ptr_writer;

  typedef AIFMesh::CoordinateType coord_type;        // position coordinate type
  typedef AIFMesh::NormalCoordinateType coordN_type; // normal coordinate type
  typedef AIFMesh::CoordinateType coordC_type;       // color coordinate type
  typedef AIFMesh::CoordinateType coordT_type;       // texture coordinate type
  typedef unsigned long index_type;

public:
  /*!
   * 			Write a mesh file
   * \param  inputMesh AIFMesh object to save.
   * \param	filePath	Path to the output mesh file.
   */
  void write(const ptr_input inputMesh, const std::string &filePath);
  /*!
   * 			Write a mesh file
   * \param  inputMesh AIFMesh object to save.
   * \param	filePath	Path to the output mesh file.
   */
  void write(/*const*/ input_type &inputMesh, const std::string &filePath);
};

} // namespace AIF
} // namespace DataStructures
} // namespace FEVV


#include "FEVV/DataStructures/AIF/AIFMeshWriter.inl"

