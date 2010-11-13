/*  $Id:  $
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

#include <algorithm>
#include <vector>


#include <boost/foreach.hpp> 

#ifdef _OPENMP
    #include <omp.h>
#endif

#include "millingcutter.h"
#include "point.h"
#include "triangle.h"
#include "line.h"
#include "path.h"

#include "adaptivewaterline.h"

#include "weave.h"
#include "fiberpushcutter.h"


namespace ocl
{

//********   ********************** */

AdaptiveWaterline::AdaptiveWaterline() {
    subOp.clear();
    subOp.push_back( new FiberPushCutter() );
    subOp.push_back( new FiberPushCutter() );
    subOp[0]->setXDirection();
    subOp[1]->setYDirection();
    nthreads=1;
#ifdef _OPENMP
    nthreads = omp_get_num_procs(); 
    //omp_set_dynamic(0);
    omp_set_nested(1);
#endif
    sampling = 1.0;
    min_sampling = 0.1;
}

AdaptiveWaterline::~AdaptiveWaterline() {
    delete subOp[1];
    delete subOp[0];
}

void AdaptiveWaterline::run() {
    adaptive_sampling_run();
}

void AdaptiveWaterline::adaptive_sampling_run() {
    minx = surf->bb.minpt.x - 2*cutter->getRadius();
    maxx = surf->bb.maxpt.x + 2*cutter->getRadius();
    miny = surf->bb.minpt.y - 2*cutter->getRadius();
    maxy = surf->bb.maxpt.y + 2*cutter->getRadius();
    Line* line = new Line( Point(minx,miny,zh) , Point(maxx,maxy,zh) );
    Span* linespan = new LineSpan(*line);
    
    xfibers.clear();
    Point xstart_p1 = Point( minx, linespan->getPoint(0.0).y,  zh );
    Point xstart_p2 = Point( maxx, linespan->getPoint(0.0).y,  zh );
    Point xstop_p1 = Point( minx, linespan->getPoint(1.0).y,  zh );
    Point xstop_p2 = Point( maxx, linespan->getPoint(1.0).y,  zh );
    Fiber xstart_f = Fiber( xstart_p1, xstart_p2 ) ;
    Fiber xstop_f = Fiber( xstop_p1, xstop_p2 ); 
    subOp[0]->run(xstart_f);
    subOp[0]->run(xstop_f);
    xfibers.push_back(xstart_f);
    std::cout << " XFiber adaptive sample \n";
    xfiber_adaptive_sample( linespan, 0.0, 1.0, xstart_f, xstop_f);
    
    yfibers.clear();
    Point ystart_p1 = Point( linespan->getPoint(0.0).x, miny,  zh );
    Point ystart_p2 = Point( linespan->getPoint(0.0).x, maxy,  zh );
    Point ystop_p1 = Point( linespan->getPoint(1.0).x, miny,  zh );
    Point ystop_p2 = Point( linespan->getPoint(1.0).x, maxy,  zh );
    Fiber ystart_f = Fiber( ystart_p1, ystart_p2 ) ;
    Fiber ystop_f = Fiber( ystop_p1, ystop_p2 ); 
    subOp[1]->run(ystart_f);
    subOp[1]->run(ystop_f);
    yfibers.push_back(ystart_f);
    std::cout << " YFiber adaptive sample \n";
    yfiber_adaptive_sample( linespan, 0.0, 1.0, ystart_f, ystop_f);
    
    weave_process();
    
}


void AdaptiveWaterline::xfiber_adaptive_sample(const Span* span, double start_t, double stop_t, Fiber start_f, Fiber stop_f) {
    const double mid_t = start_t + (stop_t-start_t)/2.0; // mid point sample
    assert( mid_t > start_t );  assert( mid_t < stop_t );
    //std::cout << "xfiber sample= ( " << start_t << " , " << stop_t << " ) \n";
    Point mid_p1 = Point( minx, span->getPoint( mid_t ).y,  zh );
    Point mid_p2 = Point( maxx, span->getPoint( mid_t ).y,  zh );
    Fiber mid_f = Fiber( mid_p1, mid_p2 );
    subOp[0]->run( mid_f );
    double fw_step = fabs( start_f.p1.y - stop_f.p1.y ) ;
    if ( fw_step > sampling ) { // above minimum step-forward, need to sample more
        xfiber_adaptive_sample( span, start_t, mid_t , start_f, mid_f  );
        xfiber_adaptive_sample( span, mid_t  , stop_t, mid_f  , stop_f );
    } else if ( !flat(start_f,mid_f,stop_f)   ) {
        if (fw_step > min_sampling) { // not a a flat segment, and we have not reached maximum sampling
            xfiber_adaptive_sample( span, start_t, mid_t , start_f, mid_f  );
            xfiber_adaptive_sample( span, mid_t  , stop_t, mid_f  , stop_f );
        }
    } else {
        xfibers.push_back(stop_f);
    } 
}

void AdaptiveWaterline::yfiber_adaptive_sample(const Span* span, double start_t, double stop_t, Fiber start_f, Fiber stop_f) {
    const double mid_t = start_t + (stop_t-start_t)/2.0; // mid point sample
    assert( mid_t > start_t );  assert( mid_t < stop_t );
    //std::cout << "yfiber sample= ( " << start_t << " , " << stop_t << " ) \n";
    Point mid_p1 = Point( span->getPoint( mid_t ).x, miny,  zh );
    Point mid_p2 = Point( span->getPoint( mid_t ).x, maxy, zh );
    Fiber mid_f = Fiber( mid_p1, mid_p2 );
    subOp[1]->run( mid_f );
    double fw_step = fabs( start_f.p1.x - stop_f.p1.x ) ;
    if ( fw_step > sampling ) { // above minimum step-forward, need to sample more
        yfiber_adaptive_sample( span, start_t, mid_t , start_f, mid_f  );
        yfiber_adaptive_sample( span, mid_t  , stop_t, mid_f  , stop_f );
    } else if ( !flat(start_f,mid_f,stop_f)   ) {
        if (fw_step > min_sampling) { // not a a flat segment, and we have not reached maximum sampling
            yfiber_adaptive_sample( span, start_t, mid_t , start_f, mid_f  );
            yfiber_adaptive_sample( span, mid_t  , stop_t, mid_f  , stop_f );
        }
    } else {
        yfibers.push_back(stop_f); 
    }
}

bool AdaptiveWaterline::flat( Fiber& start, Fiber& mid, Fiber& stop ) {
    if (start.size() != stop.size() )
        return false;
    else if (start.size() != mid.size() )
        return false;
    else if (mid.size() != stop.size() )
        return false;
    else
        return true;
}


bool xFiber_compare (const Fiber& f1, const Fiber& f2) { 
    return (f1.p1.x < f2.p1.x); 
}

bool yFiber_compare (const Fiber& f1, const Fiber& f2) { 
    return (f1.p1.y < f2.p1.y); 
}

void AdaptiveWaterline::weave_process() {
    std::cout << "Weave..." << std::flush;
    Weave w;
    
    std::cout << " adding " << xfibers.size() << " xfibers to weave \n";
    BOOST_FOREACH( Fiber f, xfibers ) {
        w.addFiber(f);
    }
    std::cout << " adding " << yfibers.size() << " yfibers to weave \n";
    BOOST_FOREACH( Fiber f, yfibers ) {
        w.addFiber(f);
    }
    //std::cout << w.str();
    std::cout << "build()..." << std::flush;
    w.build(); // build weave from fibers
    //std::cout << w.str();
    std::cout << "split()..." << std::flush;
    std::vector<Weave> subweaves = w.split_components(); // split into components
    std::cout << "traverse()..." << std::flush;
    std::vector< std::vector<Point> > subweave_loops;
    std::cout << " got " << subweaves.size() << " sub-weaves \n";
    int m = 0;
    BOOST_FOREACH( Weave sw, subweaves ) {
        std::cout << "Weave " << m << " : " << sw.str() << "\n";
        m++;
        std::cout << " face_traverse() \n";
        sw.face_traverse(); // traverse to find loops
        std::cout << " getLoops() \n";
        subweave_loops = sw.getLoops();
        BOOST_FOREACH( std::vector<Point> loop, subweave_loops ) {
            this->loops.push_back( loop );
        }
    }
    std::cout << "done.\n" << std::flush;
}      


}// end namespace
// end file adaptivewaterline.cpp
