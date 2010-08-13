import ocl
import pyocl
import camvtk
import time
import datetime
import vtk
import math

def main():  
    print ocl.revision()
    myscreen = camvtk.VTKScreen()   
    myscreen.camera.SetPosition(-8, -4, 25)
    myscreen.camera.SetFocalPoint(5,5, 0)   
    # axis arrows
    camvtk.drawArrows(myscreen,center=(-1,-1,0))
    
    stl = camvtk.STLSurf("../stl/gnu_tux_mod.stl")
    myscreen.addActor(stl)
    #stl.SetWireframe()
    stl.SetColor((0.5,0.5,0.5))
    
    polydata = stl.src.GetOutput()
    s = ocl.STLSurf()
    camvtk.vtkPolyData2OCLSTL(polydata, s)
    print "STL surface read,", s.size(), "triangles"
    
    #angle = math.pi/4
    radius  = 1.4321
    cutter = ocl.BallCutter(radius)
    cutter.length = 20

    
    # generate CL-points
    minx=0
    dx=0.1/3
    maxx=9
    miny=0
    dy=cutter.radius/1.5
    maxy=12
    z=-1
    # this generates a list of CL-points in a grid
    clpoints = pyocl.CLPointGrid(minx,dx,maxx,miny,dy,maxy,z)
    # batchdropcutter    
    bdc = ocl.BatchDropCutter()
    bdc.bucketSize = 10
    bdc.setSTL(s)
    bdc.setCutter(cutter)
    #bdc.setThreads(1)  # explicitly setting one thread is better for debugging
    for p in clpoints:
        bdc.appendPoint(p)
    
    t_before = time.time()    
    bdc.dropCutter4()
    dc_calls = bdc.dcCalls
    t_after = time.time()
    calctime = t_after-t_before
    print " BDC4 done in ", calctime," s", dc_calls," dc-calls" 
    dropcutter_time = calctime
    clpoints = bdc.getCLPoints()
    
    camvtk.drawCLPointCloud(myscreen, clpoints)
    print " clpts= ", len(clpoints)
    myscreen.render()
    #myscreen.iren.Start() 
    #exit()
    
    s = ocl.BallCutterVolume()
    #s.center = ocl.Point(-2.50,-0.6,0)
    s.radius = cutter.radius
    s.length = cutter.length

    # screenshot writer
    w2if = vtk.vtkWindowToImageFilter()
    w2if.SetInput(myscreen.renWin)
    lwr = vtk.vtkPNGWriter()
    lwr.SetInput( w2if.GetOutput() )
    
    cp= ocl.Point(5,5,-6) # center of octree
    #depths = [3, 4, 5, 6, 7, 8]
    max_depth = 8
    root_scale = 10
    t = ocl.Octree(root_scale, max_depth, cp)
    t.init(4)
    #print n
    n = 0 # the frame number

    #nmax=20
    #theta=0
    #dtheta=0.06
    #thetalift=-0.0
    #s.center =  ocl.Point( 1.7*math.cos(theta),1.3*math.sin(theta),thetalift*theta)  
    renderinterleave=400
    while (n<len(clpoints)):
        #if n==1:
        #    print clpoints[n]
            #exit()
        s.pos = ocl.Point( clpoints[n].x, clpoints[n].y, clpoints[n].z )
        print n,": diff...",
        t_before = time.time() 
        t.diff_negative(s)
        #tris = t.mc_triangles()
        t_after = time.time() 
        build_time = t_after-t_before
        print "done in ", build_time," s"
        
        if ( (n%renderinterleave)==0):
            t_before = time.time() 
            print "mc()...",
            tris = t.mc_triangles()
            t_after = time.time() 
            mc_time = t_after-t_before
            print "done in ", mc_time," s"
            print " mc() got ", len(tris), " triangles"
            mc_surf = camvtk.STLSurf( triangleList=tris, color=camvtk.red )
            #mc_surf.SetWireframe()
            mc_surf.SetColor(camvtk.cyan)
            print " STLSurf()...",
            myscreen.addActor( mc_surf )
            print "done."
            #nodes = t.get_leaf_nodes()
            #allpoints=[]
            #for no in nodes:
            #    verts = no.vertices()
            #    for v in verts:
            #        allpoints.append(v)
            #oct_points = camvtk.PointCloud( allpoints )
            #print " PointCloud()...",
            #myscreen.addActor( oct_points )
            #print "done."
            print " render()...",
            myscreen.render()
            print "done."
            print n
            if n is not len(clpoints)-1:
                myscreen.removeActor( mc_surf )
                #myscreen.removeActor( oct_points )
        n=n+1

        #lwr.SetFileName("frames/mc8_frame"+ ('%06d' % n)+".png")
        #myscreen.camera.Azimuth( 2 )
        #myscreen.render()
        #w2if.Modified() 
        #lwr.Write()
            
        #mc_surf.SetWireframe()
        #print "sleep...",
        #time.sleep(1.02)
        #print "done."
                
        

        # move forward
        #theta = n*dtheta
        #sp1 = ocl.Point(s.center)
        #s.center =  ocl.Point( 1.7*math.cos(theta),1.3*math.sin(theta),thetalift*theta)  
        #sp2 = ocl.Point(s.center)
        #print "line from ",sp1," to ",sp2
        #if n is not nmax:
        #    myscreen.addActor( camvtk.Line( p1=(sp1.x,sp1.y,sp1.z),p2=(sp2.x,sp2.y,sp2.z), color=camvtk.red ) )
        #print "center moved to", s.center
    
    print " clpts= ", len(clpoints)
    print "All done."
    myscreen.iren.Start() 

if __name__ == "__main__":

    main()
