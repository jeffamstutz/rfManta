
#include <Model/Primitives/IsosurfaceGridVolume.h>
#include <Model/Primitives/OctreeVolume.h>
#include <Core/Util/LargeFile.h>
#include <Interface/RayPacket.h>
#include <Model/Intersections/IsosurfaceImplicit.h>

using namespace std;
using namespace Manta;

IsosurfaceGridVolume::IsosurfaceGridVolume(char* filebase, int _mc_depth, double isovalue, Material* _matl)
: PrimitiveCommon(_matl), isoval((ST)isovalue)
{
    mc_depth = _mc_depth;

    this->filebase=strdup(filebase);
    if (mc_depth<=0)	
        mc_depth=1;
    char filename[2048];

    sprintf(filename, "%s.hdr", filebase);
    ifstream in(filename);

    if(!in){
    cerr << "Error opening header: " << filename << endl;
    exit(1);
    }
    in >> nx >> ny >> nz;
    double x,y,z;
    in >> x >> y >> z;
    bounds[0]=Vector(x,y,z);
    in >> x >> y >> z;
    bounds[1]=Vector(x,y,z);
    double val; //do it this way, if ST is char, this will just read the first char's instead of the whole number
    in >> val;
    datamin = (ST)val;
    in >> val;
    datamax = (ST)val;

    if(!in)
    {
        cerr << "Error reading header: " << filename << '\n';
        exit(1);
    }
    datadiag=bounds[1] - bounds[0];
    sdiag=datadiag/Vector(nx-1,ny-1,nz-1);

    blockdata.resize(nx, ny, nz);

    sprintf(filename, "%s.raw", filebase);
    FILE* din = fopen(filename, "rb");
    if (!din) 
    {
        cerr << "Error opening original data file: " << filename << '\n';
        exit(1);
    }

    Array3<ST> indata;
    indata.resize(nx, ny, nz);
    cerr << "Reading " << filename << "...";
    fread_big((char*)(&indata.get_dataptr()[0][0][0]), 1, indata.get_datasize(), din);
    cerr << "done.\n";
    fclose(din);

    cerr << "Copying original data to BrickArray3...";
    for(int i=0; i<nx; i++)
        for(int j=0; j<ny; j++)
            for(int k=0; k<nz; k++)
                blockdata(i,j,k) = indata(i,j,k);
    cerr << "done!";
        
    xsize=new int[mc_depth];
    ysize=new int[mc_depth];
    zsize=new int[mc_depth];
    int tx=nx-1;
    int ty=ny-1;
    int tz=nz-1;
    for(int i=mc_depth-1;i>=0;i--)
    {
        int nx=(int)(pow(tx, 1./(i+1))+.9);
        tx=(tx+nx-1)/nx;
        xsize[mc_depth-i-1]=nx;
        int ny=(int)(pow(ty, 1./(i+1))+.9);
        ty=(ty+ny-1)/ny;
        ysize[mc_depth-i-1]=ny;
        int nz=(int)(pow(tz, 1./(i+1))+.9);
        tz=(tz+nz-1)/nz;
        zsize[mc_depth-i-1]=nz;
    }
    ixsize=new double[mc_depth];
    iysize=new double[mc_depth];
    izsize=new double[mc_depth];
    cerr << "Calculating mc_depths...\n";
    for(int i=0;i<mc_depth;i++){
        cerr << "xsize=" << xsize[i] << ", ysize=" << ysize[i] << ", zsize=" << zsize[i] << '\n';
        ixsize[i]=1./xsize[i];
        iysize[i]=1./ysize[i];
        izsize[i]=1./zsize[i];
    }
    cerr << "X: ";
    tx=1;
    for(int i=mc_depth-1;i>=0;i--){
        cerr << xsize[i] << ' ';
        tx*=xsize[i];
    }
    cerr << "(" << tx << ")\n";
    if(tx<nx-1)
    {
        cerr << "TX TOO SMALL!\n";
        exit(1);
    }
    cerr << "Y: ";
    ty=1;
    for(int i=mc_depth-1;i>=0;i--)
    {
        cerr << ysize[i] << ' ';
        ty*=ysize[i];
    }
    cerr << "(" << ty << ")\n";
    if(ty<ny-1)
    {
        cerr << "TY TOO SMALL!\n";
        exit(1);
    }
    cerr << "Z: ";
    tz=1;
    for(int i=mc_depth-1;i>=0;i--){
        cerr << zsize[i] << ' ';
        tz*=zsize[i];
    }
    cerr << "(" << tz << ")\n";
    if(tz<nz-1){
        cerr << "TZ TOO SMALL!\n";
        exit(1);
    }
    hierdiag=datadiag*Vector(tx,ty,tz)/Vector(nx-1,ny-1,nz-1);
    ihierdiag=Vector(1.,1.,1.)/hierdiag;

    if(mc_depth==1){
        macrocells=0;
    } 
    else 
    {
        macrocells=new BrickArray3<VMCell>[mc_depth+1];
        int xs=1;
        int ys=1;
        int zs=1;
        for(int d=mc_depth-1;d>=1;d--){
            xs*=xsize[d];
            ys*=ysize[d];
            zs*=zsize[d];
            macrocells[d].resize(xs, ys, zs);
            cerr << "Depth " << d << ": " << xs << "x" << ys << "x" << zs << '\n';
        }
        cerr << "Creating macrocells...";
        
        VMCell top;
        calc_mcell(mc_depth-1, 0, 0, 0, top);
        cerr << "Min: " << (int)top.min << ", Max: " << (int)top.max << endl;
        cerr << "\ndone!\n";
    }
    
#ifdef TEST_OCTDATA
    //build the octree data from the single-res data.
    double threshold = 0.0;
    int max_depth = -1;
    cerr << "Reading multi-resolution octree data.";
    //octdata.make_from_single_res_data(indata, threshold, 2, 0);
    octdata.read_file(filebase);
#endif

}

IsosurfaceGridVolume::~IsosurfaceGridVolume()
{
}

void IsosurfaceGridVolume::preprocess( PreprocessContext const &context )
{
    PrimitiveCommon::preprocess(context);
}

void IsosurfaceGridVolume::computeBounds( PreprocessContext const &context, BBox &box ) const
{
    box = bounds;
}

void IsosurfaceGridVolume::computeNormal(const RenderContext& context, RayPacket& rays) const
{
}
            
BBox IsosurfaceGridVolume::getBounds() const
{
    return bounds;
}

void IsosurfaceGridVolume::intersect(RenderContext const &context, RayPacket &packet) const
{
    for ( int i = packet.rayBegin; i < packet.rayEnd; i++ )
        single_intersect(packet, i);
}


void IsosurfaceGridVolume::single_intersect(RayPacket& rays, int which_one) const
{
    int start_depth = this->mc_depth-1;
    //find tenter, texit, penter using ray-AABB intersection
    Vector dir = rays.getDirection(which_one);
    Vector inv_dir = dir.inverse();
    Vector orig = rays.getOrigin(which_one);
    Vector min = bounds[0];
    Vector max = bounds[1];

    int dix_dx;
    int ddx;
    int diy_dy;
    int ddy;
    int diz_dz;
    int ddz;

    float tenter, texit;
    if(dir.x() > 0)
    {
        tenter = inv_dir.x() * (min.x()-orig.x());
        texit = inv_dir.x() * (max.x()-orig.x());
        dix_dx=1;
        ddx=1;
    } 
    else 
    {
        tenter = inv_dir.x() * (max.x()-orig.x());
        texit = inv_dir.x() * (min.x()-orig.x());
        dix_dx=-1;
        ddx=0;
    }

    float y0, y1;
    if(dir.y() > 0)
    {
        y0 = inv_dir.y() * (min.y()-orig.y());
        y1 = inv_dir.y() * (max.y()-orig.y());
        diy_dy=1;
        ddy=1;
    } 
    else 
    {
        y0 = inv_dir.y() * (max.y()-orig.y());
        y1 = inv_dir.y() * (min.y()-orig.y());
        diy_dy=-1;
        ddy=0;
    }
    tenter = MAX(tenter, y0);
    texit = MIN(texit, y1);
    if (tenter > texit)
        return;

    float z0, z1;
    if(dir.z() > 0)
    {
        z0 = inv_dir.z() * (min.z()-orig.z());
        z1 = inv_dir.z() * (max.z()-orig.z());
        diz_dz=1;
        ddz=1;
    } 
    else 
    {
        z0 = inv_dir.z() * (max.z()-orig.z());
        z1 = inv_dir.z() * (min.z()-orig.z());
        diz_dz=-1;
        ddz=0;
    }
    tenter = MAX(tenter, z0);
    texit = MIN(texit, z1);
    if (tenter > texit)
        return;

    if (texit < 0.)
        return;

    tenter = MAX(0., tenter);
    Vector p = orig + dir*tenter;
    Vector s((p-min)*ihierdiag);
    int cx=xsize[start_depth];
    int cy=ysize[start_depth];
    int cz=zsize[start_depth];
    int ix=(int)(s.x()*cx);
    int iy=(int)(s.y()*cy);
    int iz=(int)(s.z()*cz);
    if(ix>=cx)
    ix--;
    if(iy>=cy)
    iy--;
    if(iz>=cz)
    iz--;
    if(ix<0)
    ix++;
    if(iy<0)
    iy++;
    if(iz<0)
    iz++;


    float next_x, next_y, next_z;
    float dtdx, dtdy, dtdz;
    float icx=ixsize[start_depth];
    float x=min.x()+hierdiag.x()*float(ix+ddx)*icx;
    next_x=(x-orig.x())*inv_dir.x();
    dtdx=dix_dx*hierdiag.x()*icx*inv_dir.x();
    float icy=iysize[start_depth];
    float y=min.y()+hierdiag.y()*float(iy+ddy)*icy;
    next_y=(y-orig.y())*inv_dir.y();
    dtdy=diy_dy*hierdiag.y()*icy*inv_dir.y();
    float icz=izsize[start_depth];
    float z=min.z()+hierdiag.z()*float(iz+ddz)*icz;
    next_z=(z-orig.z())*inv_dir.z();
    dtdz=diz_dz*hierdiag.z()*icz*inv_dir.z();

    Vector cellsize(cx,cy,cz);
    Vector cellcorner((orig-min)*ihierdiag*cellsize);
    Vector celldir(dir*ihierdiag*cellsize);
    
    isect(rays, which_one, orig, dir, inv_dir, 
        start_depth, tenter, dtdx, dtdy, dtdz, next_x, next_y, next_z,
        ix, iy, iz, dix_dx, diy_dy, diz_dz,
        0, 0, 0, cellcorner, celldir); 
}	

void IsosurfaceGridVolume::isect(RayPacket& rays, int which_one, 
       const Vector& orig, const Vector& dir, const Vector& inv_dir, 
       int depth, float t,
       float dtdx, float dtdy, float dtdz,
       float next_x, float next_y, float next_z,
       int ix, int iy, int iz,
       int dix_dx, int diy_dy, int diz_dz,
       int startx, int starty, int startz,
       const Vector& cellcorner, const Vector& celldir) const
{
	int cx=xsize[depth];
	int cy=ysize[depth];
	int cz=zsize[depth];
	if(depth==0)
    {
		for(;;)
        {
            int gx=startx+ix;
            int gy=starty+iy;
            int gz=startz+iz;
            if(gx<nx-1 && gy<ny-1 && gz<nz-1)
            {
                //alog( << "Doing cell: " << gx << ", " << gy << ", " << gz
                //<< " (" << startx << "+" << ix << ", " << starty << "+" << iy << ", " << startz << "+" << iz << ")\n");
                ST rhos[8];
#ifdef USE_OCTREE_DATA
#ifdef USE_OTD_NEIGHBOR_FIND	
                //neighbor find
                unsigned int index_trace[octdata.max_depth];
                ST min_rho, max_rho, this_rho;
                Vec3i child_cell(gx, gy, gz);
                rhos[0]=octdata.lookup_node_trace(child_cell, 0, 0, 0, index_trace);
                int target_child = ((gx & 1) << 2) | ((gy & 1) << 1) | (gz & 1);
                OctCap& cap = octdata.get_cap(index_trace[octdata.max_depth-1]);
                //0,0,1
                Vec3i offset(0,0,1);
                if (target_child & 1)
                    rhos[1] = octdata.lookup_neighbor<0,0,1>(child_cell, offset, 0, depth, index_trace);
                else
                    rhos[1] = cap.values[target_child | 1];
                //0,1,1
                offset.data[1] = 1;
                if (target_child & 3)
                    rhos[3] = octdata.lookup_neighbor<0,1,1>(child_cell, offset, 0, depth, index_trace);
                else
                    rhos[3] = cap.values[target_child | 3];
                //1,1,1
                offset.data[0] = 1;
                if (target_child & 7)
                    rhos[7] = octdata.lookup_neighbor<1,1,1>(child_cell, offset, 0, depth, index_trace);
                else
                    rhos[7] = cap.values[target_child | 7];		
                //1,1,0
                offset.data[2] = 0;
                if (target_child & 6)
                    rhos[6] = octdata.lookup_neighbor<1,1,0>(child_cell, offset, 0, depth, index_trace);
                else
                    rhos[6] = cap.values[target_child | 6];
                //1,0,0
                offset.data[1] = 0;
                if (target_child & 4)
                    rhos[4] = octdata.lookup_neighbor<1,0,0>(child_cell, offset, 0, depth, index_trace);
                else
                    rhos[4] = cap.values[target_child | 4];			
                //1,0,1
                offset.data[2] = 1;
                if (target_child & 5)
                    rhos[5] = octdata.lookup_neighbor<1,0,1>(child_cell, offset, 0, depth, index_trace);
                else
                    rhos[5] = cap.values[target_child | 5];
                //0,1,0
                offset.data[0] = 0;
                offset.data[1] = 1;
                offset.data[2] = 0;
                if (target_child & 2)
                    rhos[2] = octdata.lookup_neighbor<0,1,0>(child_cell, offset, 0, depth, index_trace);
                else
                    rhos[2] = cap.values[target_child | 2];
#else
                //pure point location
                rhos[0]=octdata(gx, gy, gz);
                rhos[1]=octdata(gx, gy, gz+1);
                rhos[2]=octdata(gx, gy+1, gz);
                rhos[3]=octdata(gx, gy+1, gz+1);
                rhos[4]=octdata(gx+1, gy, gz);
                rhos[5]=octdata(gx+1, gy, gz+1);
                rhos[6]=octdata(gx+1, gy+1, gz);
                rhos[7]=octdata(gx+1, gy+1, gz+1);
#endif
#else	//lookup using the grid
                rhos[0]=blockdata(gx, gy, gz);
                rhos[1]=blockdata(gx, gy, gz+1);
                rhos[2]=blockdata(gx, gy+1, gz);
                rhos[3]=blockdata(gx, gy+1, gz+1);
                rhos[4]=blockdata(gx+1, gy, gz);
                rhos[5]=blockdata(gx+1, gy, gz+1);
                rhos[6]=blockdata(gx+1, gy+1, gz);
                rhos[7]=blockdata(gx+1, gy+1, gz+1);
#endif
                ST min=rhos[0];
                ST max=rhos[0];
                for(int i=1;i<8;i++)
                {
                    if(rhos[i]<min)
                        min=rhos[i];
                    if(rhos[i]>max)
                        max=rhos[i];
                }
                
                if(min < isoval && max>isoval)
                {
                    //alog(<<"might have a hit" << endl);
                    float hit_t;
                    Vector p0(bounds[0]+sdiag*Vector(gx,gy,gz));
                    Vector p1(p0+sdiag);
                    float tmax=next_x;
                    if(next_y<tmax)
                        tmax=next_y;
                    if(next_z<tmax)
                        tmax=next_z;
                    float rho[2][2][2];
                    rho[0][0][0]=rhos[0];
                    rho[0][0][1]=rhos[1];
                    rho[0][1][0]=rhos[2];
                    rho[0][1][1]=rhos[3];
                    rho[1][0][0]=rhos[4];
                    rho[1][0][1]=rhos[5];
                    rho[1][1][0]=rhos[6];
                    rho[1][1][1]=rhos[7];
                    
                    if (IsosurfaceImplicit::single_intersect_neubauer(orig, dir, p0, p1, rho, 
                        isoval, t, tmax, hit_t))
                    {
                        if (rays.hit(which_one, hit_t, PrimitiveCommon::getMaterial(), this, 0)) 
                        {
                            Vector normal;
                            Vector phit = orig + dir*hit_t;
                            IsosurfaceImplicit::single_normal(normal, p0, p1, phit, rho);
                            normal.normalize();
                            rays.setNormal(which_one, normal);
                        }
                    }
                }
            }
            
            if(next_x < next_y && next_x < next_z)
            {
                // Step in x...
                t=next_x;
                next_x+=dtdx;
                ix+=dix_dx;
                if(ix<0 || ix>=cx)
                break;
            } 
            else if(next_y < next_z){
                t=next_y;
                next_y+=dtdy;
                iy+=diy_dy;
                if(iy<0 || iy>=cy)
                break;
            } 
            else {
                t=next_z;
                next_z+=dtdz;
                iz+=diz_dz;
                if(iz<0 || iz>=cz)
                break;
            }
		}
	} 
    else 
    {
		BrickArray3<VMCell>& mcells=macrocells[depth];
		for(;;)
        {
            int gx=startx+ix;
            int gy=starty+iy;
            int gz=startz+iz;
            VMCell& mcell=mcells(gx,gy,gz);
            //alog( << "doing macrocell: " << gx << ", " << gy << ", " << gz << ": " << mcell.min << ", " << mcell.max << endl);
            if(mcell.max>isoval && mcell.min<isoval)
            {
                // Do this cell...
                int new_cx=xsize[depth-1];
                int new_cy=ysize[depth-1];
                int new_cz=zsize[depth-1];
                int new_ix=(int)((cellcorner.data[0]+t*celldir.data[0]-ix)*new_cx);
                int new_iy=(int)((cellcorner.data[1]+t*celldir.data[1]-iy)*new_cy);
                int new_iz=(int)((cellcorner.data[2]+t*celldir.data[2]-iz)*new_cz);
                //alog( << "new: " << (cellcorner.x()+t*celldir.x()-ix)*new_cx
                //<< " " << (cellcorner.y()+t*celldir.y()-iy)*new_cy
                //<< " " << (cellcorner.z()+t*celldir.z()-iz)*new_cz
                //<< '\n');
                if(new_ix<0)
                new_ix=0;
                else if(new_ix>=new_cx)
                new_ix=new_cx-1;
                if(new_iy<0)
                new_iy=0;
                else if(new_iy>=new_cy)
                new_iy=new_cy-1;
                if(new_iz<0)
                new_iz=0;
                else if(new_iz>=new_cz)
                new_iz=new_cz-1;
                
                float new_dtdx=dtdx*ixsize[depth-1];
                float new_dtdy=dtdy*iysize[depth-1];
                float new_dtdz=dtdz*izsize[depth-1];
                const Vector dir = rays.getDirection(which_one);
                float new_next_x;
                if(dir.data[0] > 0){
                    new_next_x=next_x-dtdx+new_dtdx*(new_ix+1);
                } else {
                    new_next_x=next_x-new_ix*new_dtdx;
                }
                float new_next_y;
                if(dir.data[1] > 0){
                    new_next_y=next_y-dtdy+new_dtdy*(new_iy+1);
                    } else {
                new_next_y=next_y-new_iy*new_dtdy;
                }
                float new_next_z;
                if(dir.data[2] > 0){
                    new_next_z=next_z-dtdz+new_dtdz*(new_iz+1);
                } else {
                    new_next_z=next_z-new_iz*new_dtdz;
                }
                int new_startx=gx*new_cx;
                int new_starty=gy*new_cy;
                int new_startz=gz*new_cz;
                //alog( << "startz=" << startz << '\n');
                //alog( << "iz=" << iz << '\n');
                //alog( <
                Vector cellsize(new_cx, new_cy, new_cz);
                isect(rays, which_one,
                    orig, dir, inv_dir, 
                    depth-1, t,
                    new_dtdx, new_dtdy, new_dtdz,
                    new_next_x, new_next_y, new_next_z,
                    new_ix, new_iy, new_iz,
                    dix_dx, diy_dy, diz_dz,
                    new_startx, new_starty, new_startz,
                    (cellcorner-Vector(ix, iy, iz))*cellsize, celldir*cellsize);
            }
            
            if(next_x < next_y && next_x < next_z)
            {
                // Step in x...
                t=next_x;
                next_x+=dtdx;
                ix+=dix_dx;
                if(ix<0 || ix>=cx)
                    break;
            } 
            else if(next_y < next_z)
            {
                t=next_y;
                next_y+=dtdy;
                iy+=diy_dy;
                if(iy<0 || iy>=cy)
                break;
            } 
            else 
            {
                t=next_z;
                next_z+=dtdz;
                iz+=diz_dz;
                if(iz<0 || iz>=cz)
                break;
            }
            if(rays.getMinT(which_one) < t)
                break;
            }
        }
	}

void IsosurfaceGridVolume::calc_mcell(int depth, int startx, int starty, int startz, VMCell& mcell)
{
    mcell.min=std::numeric_limits<ST>::max();
    mcell.max=std::numeric_limits<ST>::min();
    int endx=startx+xsize[depth];
    int endy=starty+ysize[depth];
    int endz=startz+zsize[depth];
    if(endx>nx-1)
    endx=nx-1;
    if(endy>ny-1)
    endy=ny-1;
    if(endz>nz-1)
    endz=nz-1;
    if(startx>=endx || starty>=endy || startz>=endz){
    /* This cell won't get used... */
    mcell.min=datamax;
    mcell.max=datamin;
    return;
    }
    if(depth==0){
    for(int ix=startx;ix<endx;ix++){
        for(int iy=starty;iy<endy;iy++){
        for(int iz=startz;iz<endz;iz++){
            ST rhos[8];
            rhos[0]=blockdata(ix, iy, iz);
            rhos[1]=blockdata(ix, iy, iz+1);
            rhos[2]=blockdata(ix, iy+1, iz);
            rhos[3]=blockdata(ix, iy+1, iz+1);
            rhos[4]=blockdata(ix+1, iy, iz);
            rhos[5]=blockdata(ix+1, iy, iz+1);
            rhos[6]=blockdata(ix+1, iy+1, iz);
            rhos[7]=blockdata(ix+1, iy+1, iz+1);
            ST min=rhos[0];
            ST max=rhos[0];
            for(int i=1;i<8;i++){
            if(rhos[i]<min)
                min=rhos[i];
            if(rhos[i]>max)
                max=rhos[i];
            }
            if(min<mcell.min)
            mcell.min=min;
            if(max>mcell.max)
            mcell.max=max;
        }
        }
    }
    } else {
    int nx=xsize[depth-1];
    int ny=ysize[depth-1];
    int nz=zsize[depth-1];
    BrickArray3<VMCell>& mcells=macrocells[depth];
    for(int x=startx;x<endx;x++){
        for(int y=starty;y<endy;y++){
        for(int z=startz;z<endz;z++){
            VMCell tmp;
            calc_mcell(depth-1, x*nx, y*ny, z*nz, tmp);
            if(tmp.min < mcell.min)
            mcell.min=tmp.min;
            if(tmp.max > mcell.max)
            mcell.max=tmp.max;
            mcells(x,y,z)=tmp;
        }
        }
    }
    }
}
