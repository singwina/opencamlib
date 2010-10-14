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

#include <sstream> // for str()

#include "ccpoint.h"

namespace ocl
{
    
/* ********************************************** CCPoint *************/

CCPoint::CCPoint() 
    : Point() { 
    type = NONE;
}

CCPoint::CCPoint(const Point& p) 
    : Point( p ) { 
    type = NONE;
}

CCPoint::CCPoint(const Point& p, CCType t) 
    : Point( p ) { 
    type = t;
}

CCPoint::CCPoint(double x, double y, double z) 
    : Point( x,y,z ) { 
    type = NONE;
}

CCPoint::CCPoint(double x, double y, double z, CCType t) 
    : Point( x,y,z ) { 
    type = t;
}

CCPoint& CCPoint::operator=(const Point &p) {
    x=p.x;
    y=p.y;
    z=p.z;
    type = NONE;
    return *this;
}

std::string CCPoint::str() const {
    std::ostringstream o;
    o << *this;
    return o.str();
}

std::ostream& operator<<(std::ostream &stream, const CCPoint& p) {
  stream << "CC(" << p.x << ", " << p.y << ", " << p.z << ", t=" << p.type <<")"; 
  return stream;
}

} // end namespace
// end file ccpoint.cpp
