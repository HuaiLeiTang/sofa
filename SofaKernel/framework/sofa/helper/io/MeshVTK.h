/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, development version     *
*                (c) 2006-2018 INRIA, USTL, UJF, CNRS, MGH                    *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>.        *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#ifndef SOFA_HELPER_IO_MESHVTK_H
#define SOFA_HELPER_IO_MESHVTK_H

#include <sofa/helper/io/Mesh.h>
#include <fstream>

namespace sofa
{

namespace helper
{

namespace io
{

class SOFA_HELPER_API MeshVTK : public Mesh
{
public:

    MeshVTK(const std::string& filename)
    {
        init(filename);
    }

    void init(std::string filename);

protected:

    void readVTU(const std::string &filename);

};

} // namespace io

} // namespace helper

} // namespace sofa

#endif
