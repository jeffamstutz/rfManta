/*
*  OctreeVolume.h : A multiresolution octree data
*   wrapper for scalar volumes, such as BrickArray3.
*
*  Written by:
*   Aaron M Knoll
*   Department of Computer Science
*   University of Utah
*   October 2005
*
*  Copyright (C) 2005 SCI Group
*/

#include <Model/Primitives/OctreeVolume.h>
#include <Core/Util/LargeFile.h>
#include <cstring>

using namespace Manta;

#define START_ISOVAL 20

#define narrow_range(val, rmin, rmax) \
	val = MAX(rmin, val);\
	val = MIN(rmax, val);\
\

OctreeVolume::OctreeVolume()
{
}

OctreeVolume::OctreeVolume(char* filebase)
{
    isoval = START_ISOVAL;
    read_file(filebase);
}

OctreeVolume::OctreeVolume(char* filebase,
            int startTimestep, 
            int endTimestep,
            double variance_threshold, 
            int kernelWidth, 
            ST isomin, 
            ST isomax)
{
    if (isomin >= isomax)
    {
        isomin = std::numeric_limits<ST>::min();
        isomax = std::numeric_limits<ST>::max();
    }
    
    current_timestep = 0;
    
    isoval = START_ISOVAL;
    
    //read in the octree data from .oth, .otd files
    {
        if (startTimestep >= endTimestep)
            endTimestep = startTimestep;
        
        num_timesteps = endTimestep - startTimestep + 1;
        steps = new Timestep[num_timesteps];
        
        char buf[1024];
        sprintf(buf, "%s.hdr", filebase);
        ifstream in(buf);
    
        cerr<< "Reading original data: " << buf << endl;
        int nx, ny, nz;
    
        if(!in)
        {
            cerr << "Error opening header: " << buf << endl;
            return;
        }
            
        in >> nx >> ny >> nz;    
        double x,y,z;
        in >> x >> y >> z;
        in >> x >> y >> z;
        double val;
        in >> val;
        //datamin = (ST)val;
        in >> val;
        //datamax = (ST)val;
        
        datamin = isomin;
        datamax = isomax;
        use_adaptive_res = false;
        
        int_dims.data[0] = nx;
        int_dims.data[1] = ny;
        int_dims.data[2] = nz;
        dims.data[0] = (float)(int_dims.data[0]); 
        dims.data[1] = (float)(int_dims.data[1]); 
        dims.data[2] = (float)(int_dims.data[2]); 
        
        bounds[0] = Vector(0.f, 0.f, 0.f);
        bounds[1] = dims;

        //determine the max_depth
        kernel_width = kernelWidth;	
        cerr << "(Original data has dimensions " << int_dims.data[0] << ", " << int_dims.data[1] << ", " << int_dims.data[2] << ")\n";
        double ddepth_1 = log((double)nx) / log(2.0);
        double ddepth_2 = log((double)ny) / log(2.0);
        double ddepth_3 = log((double)nz) / log(2.0);
        double ddepth = MAX(MAX(ddepth_1, ddepth_2), ddepth_3);
        max_depth = (int)(ceil(ddepth));
		cap_depth = max_depth - 1;
		pre_cap_depth = max_depth - 2;
        
        padded_dims.data[0] = padded_dims.data[1] = padded_dims.data[2] = (float)max_depth_bit;
        
        //make the child_bit_depth arrays
        child_bit_depth = new int[max_depth+1];
        inv_child_bit_depth = new float[max_depth+1];
        for(int d=0; d<=max_depth; d++)
        {
            child_bit_depth[d] = 1 << (max_depth - d - 1);
            inv_child_bit_depth[d] = 1.0f / static_cast<float>(child_bit_depth[d]);
        }
        max_depth_bit = 1 << max_depth;
        inv_max_depth_bit = 1.0f / static_cast<float>(max_depth_bit);
        
        if(!in)
        {
            cerr << "Error reading header: " << buf << '\n';
            return;
        }
            
        //read in the single-resolution data
        char filebase_numbered[4096];
        for(int fts = startTimestep, ts = 0; fts <= endTimestep; fts++, ts++)
        {
            if (num_timesteps > 1)
                sprintf(filebase_numbered,"%s_%04i.raw",filebase,fts);
            else
                sprintf(filebase_numbered,"%s.raw",filebase);
            
            FILE* din = fopen(filebase_numbered, "rb");
            if (!din) 
            {
                cerr << "Error opening original data file: " << filebase_numbered << '\n';
                return;
            }

	    cerr << "(size of unsigned long is " << sizeof(unsigned long) << ")" << endl;

#ifdef TEST_INDATA
	    indata.resize(nx, ny, nz);
#else
	    cerr << "Creating temporary Array3..." << endl;
	    Array3<ST> indata(nx, ny, nz);
#endif
            cerr << "indata(150,150,150)=" << (int)indata(150,150,150) << endl; 
            cerr << "Reading " << filebase_numbered << "...";
            cerr << "(" << indata.get_datasize() << " read/" << nx*long(ny*nz*sizeof(ST)) << " expected)...";
            fread_big((char*)(&indata.get_dataptr()[0][0][0]), 1, indata.get_datasize(), din);
            cerr << "done.\n";
            fclose(din);
            
            cerr << "indata(150,150,150)=" << indata(150,150,150) << endl; 
                
            //build the octree data from the single-res data.
            cerr << "Building multi-resolution octree data. (var thresh = "<< variance_threshold << ", kernel width = " << kernelWidth << ")...";
            make_from_single_res_data(indata, ts, variance_threshold, kernelWidth, isomin, isomax);
        
            cerr << "\nDone making from single res data" << endl;		 
        }
        
        write_file(filebase);
        
        multires_level = 0;
        current_timestep = 0;
    }

    cout << "\nDone creating Octree Volume!" << endl;
}	

OctreeVolume::~OctreeVolume()
{
    for(int ts=0; ts<num_timesteps; ts++)
    {
		delete[] steps[ts].caps;

		for(int d=0; d<max_depth-1; d++)
			delete[] steps[ts].nodes[d];
		delete[] steps[ts].nodes;
		delete[] steps[ts].num_nodes;        
    }
    delete[] steps;
    
    delete[] child_bit_depth;
    delete[] inv_child_bit_depth;
}


void OctreeVolume::make_from_single_res_data(Array3<ST>& originalData, 
                                             int ts, double variance_threshold, 
                                             int kernelWidth, 
                                             ST isomin, ST isomax)
{
    if (isomin >= isomax)
    {
        isomin = std::numeric_limits<ST>::min();
        isomax = std::numeric_limits<ST>::max();
    }
	
    cout << "Timestep " << ts << ": Building octree data of depth " << max_depth << ", variance threshold " << variance_threshold << ", kernel width " << kernel_width << ", isomin " << (int)isomin << ", isomax " << (int)isomax << ".\n";
	    
    Array3<BuildNode>* buildnodes = new Array3<BuildNode>[max_depth];
    
	steps[ts].num_nodes = new unsigned int[max_depth-1];
	steps[ts].nodes = new OctNode*[max_depth-1];
	
    //1. start at the bottom. Create 2^(depth-1) nodes in each dim
    //NOTE: d=0 is root node (coarsest depth)
    // for depth    0, 1, 2, 3, ... max_depth-1
    // we have dims 1, 2, 4, 8, ... 2^(max_depth-1)
    for(int d=0; d<max_depth; d++)
    {
        int dim = 1 << d;	
        buildnodes[d].resize(dim, dim, dim);
    }
    
    unsigned int totalnodesalldepths = 0;
    unsigned int totalmaxnodesalldepths = 0;
    
	//build the buildnodes, from bottom up.
	for(int d=max_depth-1; d>=0; d--)
	{	
		Array3<BuildNode>& bnodes = buildnodes[d];
		unsigned int ncaps = 0;
		unsigned int nnodes = 0;
		
		int nodes_dim = 1 << d;
		//cout << "\nd="<<d<<":";
		//cout << "nodes_dim = " << nodes_dim << endl;
		
		int dim_extents = 1 << (max_depth - d - 1);
		//cout << "dim_extents = " << dim_extents << endl;
		
		for(int i=0; i<nodes_dim; i++)
			for(int j=0; j<nodes_dim; j++)
				for(int k=0; k<nodes_dim; k++)
				{
					BuildNode& bnode = bnodes(i,j,k);
					
					if (d==max_depth-1)
					{
						for(int pi=0; pi<2; pi++)
							for(int pj=0; pj<2; pj++)
								for(int pk=0; pk<2; pk++)
								{
									Vec3i dp((2*i + pi) * dim_extents, 
											 (2*j + pj) * dim_extents, 
											 (2*k + pk) * dim_extents);
									ST odval = lookup_safe(originalData, dp.data[0], dp.data[1], dp.data[2]);
									narrow_range(odval, isomin, isomax);
									bnode.values[4*pi | 2*pj | pk] = odval;
								}
					}
					else
					{
						for(int pi=0; pi<2; pi++)
							for(int pj=0; pj<2; pj++)
								for(int pk=0; pk<2; pk++)
								{
									bnode.values[4*pi | 2*pj | pk] = buildnodes[d+1](2*i+pi, 2*j+pj, 2*k+pk).expected_value;
								}
					}
							
					if (d == max_depth-1)	//finest-level data are automatically caps, *if* they are kept
					{
						
						double expectedValue = 0.0;
						for(int v=0; v<8; v++)
							expectedValue += static_cast<double>(bnode.values[v]);
						expectedValue *= 0.125;
						
						double variances[8];
						bnode.keepMe = false;
						bnode.isLeaf = true;
						for(int v=0; v<8; v++)
						{
							double diff = static_cast<double>(bnode.values[v]) - expectedValue;
							variances[v] = diff * diff * 0.125;
							
							if (variances[v] >= variance_threshold)
							{
								bnode.keepMe = true;
								break;
							}
						}
						bnode.expected_value = (ST)expectedValue;
						
						//find min, max using a specified kernel width
						// 0 = ignore this step
						// 1 = this node only
						// 2 = this node + 1 forward (forward differences)
						// 3 = this node, 1 forward, 1 backward (central differences)
						// 4 = this, 2 forward, 1 backward.
						// etc.
						Vec3i kmin;
						kmin.data[0] = 2*i;
						kmin.data[1] = 2*j;
						kmin.data[2] = 2*k;
						Vec3i kmax = kmin;
						
						kmin.data[0] -= (kernel_width-1)/2;
						kmin.data[1] -= (kernel_width-1)/2;
						kmin.data[2] -= (kernel_width-1)/2;                                    
						kmax.data[0] += (kernel_width/2)+1;
						kmax.data[1] += (kernel_width/2)+1;
						kmax.data[2] += (kernel_width/2)+1;
						bnode.min = bnode.max = bnode.values[0];
						for(int pi=kmin.data[0]; pi<=kmax.data[0]; pi++)
							for(int pj=kmin.data[1]; pj<=kmax.data[1]; pj++)
								for(int pk=kmin.data[2]; pk<=kmax.data[2]; pk++)
								{
									ST val = lookup_safe(originalData, pi, pj, pk);
									narrow_range(val, isomin, isomax);
									bnode.min = MIN(bnode.min, val);
									bnode.max = MAX(bnode.max, val);
								}
						ncaps += bnode.keepMe;
					}			
					else	//if we're not at finest-level depth
					{
						//fill in the children
						Array3<BuildNode>& children_bnodes = buildnodes[d+1];
						bnode.isLeaf = false;
						bnode.keepMe = true;
						
						bnode.myIndex = 0;
						bnode.min = children_bnodes(2*i, 2*j, 2*k).min;
						bnode.max = children_bnodes(2*i, 2*j, 2*k).max;
						for(int pi=0; pi<2; pi++)
							for(int pj=0; pj<2; pj++)
								for(int pk=0; pk<2; pk++)
								{
									int cidx = 4*pi+2*pj+pk;
									BuildNode& child_bnode = children_bnodes(2*i+pi, 2*j+pj, 2*k+pk);
									
									//find min, max using children
									bnode.min = MIN(bnode.min, child_bnode.min);
									bnode.max = MAX(bnode.max, child_bnode.max);
									
									if (child_bnode.keepMe == false)	//make it a scalar leaf
									{
										bnode.offsets[cidx] = -1;
									}
									else
									{
										//update the pointer to this child index
										bnode.children_start = child_bnode.myStart;
										bnode.offsets[cidx] = child_bnode.myIndex;
									}
									
									bnode.values[cidx] = child_bnode.expected_value;

								}
									
						double expectedValue = 0.0;
						for(int v=0; v<8; v++)
							expectedValue += static_cast<double>(bnode.values[v]);
						expectedValue *= 0.125;
						bnode.expected_value = (ST)expectedValue;		
						
						//if all children are scalar leaves
						char chsum = 0;
						for(int ch=0; ch<8; ch++)
							chsum += bnode.offsets[ch];
						if (chsum == -8)
						{
							bnode.keepMe = ((bnode.max - bnode.min) >= variance_threshold);
						}
						
						/*
						if (bnode.scalar_leaf & 255)
						{
							bnode.keepMe = ((bnode.max - bnode.min) >= variance_threshold);
						}
						*/
						
						nnodes += bnode.keepMe;
						
					}	//end (not at finest-level depth)
				}	//end (iterate over i,j,k)
							
		//this is just for stats...
		unsigned int maxnodesthisdepth = (1 << d);
		maxnodesthisdepth = maxnodesthisdepth * maxnodesthisdepth * maxnodesthisdepth;
		totalmaxnodesalldepths += maxnodesthisdepth;
		
		if (d == max_depth-1)
		{     
			cerr << "\nAllocating "<<ncaps<< " caps at depth " << d<< "(" << 100.0f*ncaps/(float)maxnodesthisdepth <<"% full)";
			totalnodesalldepths += ncaps;
			steps[ts].num_caps = ncaps;
			steps[ts].caps = new OctCap[ncaps];
		}
		else
		{
			cerr << "\nAllocating "<<nnodes<< " nodes at depth " << d<< "(" << 100.0f*nnodes/(float)maxnodesthisdepth <<"% full)";
			totalnodesalldepths += nnodes;
			steps[ts].num_nodes[d] = nnodes;
			steps[ts].nodes[d] = new OctNode[nnodes];
		}
		
		//fill in caps and nodes, storing blocks of 8 (sort of like bricking)
		unsigned int cap = 0;
		unsigned int node = 0;
		if (d == max_depth-1)	//bottom nodes
		{
			//NEW
			unsigned int actualnodesthislvl = 1 << d;
			actualnodesthislvl = actualnodesthislvl * actualnodesthislvl * actualnodesthislvl;
			unsigned int n=0;
			unsigned int cap_start = n;
			while(n < actualnodesthislvl)
			{
				//find the x, y, z coordinates
				Vec3i coords(0,0,0);
				unsigned int remainder = n;
				for(int dc=d; dc>=0; dc--)
				{
					int divisor = (1 << 3*dc);
					int div = remainder / divisor;
					remainder = remainder % divisor;
					if (div)
					{
						coords.data[0] |= ((div&4)!=0) << dc;
						coords.data[1] |= ((div&2)!=0) << dc;
						coords.data[2] |= (div&1) << dc;	
					}
				}
				BuildNode& bnode = bnodes(coords.data[0], coords.data[1], coords.data[2]);
				if (bnode.keepMe)
				{
					OctCap& ocap = steps[ts].caps[cap];									
					memcpy(ocap.values, bnode.values, 8*sizeof(ST));
					bnode.myIndex = (unsigned char)(cap - cap_start);
					bnode.myStart = cap_start;
					cap++;
				}
				n++;
				if (n%8==0)
					cap_start = cap;
			}
		}
		else if (d > 0)		//inner nodes
		{
			unsigned int actualnodesthislvl = 1 << d;
			actualnodesthislvl = actualnodesthislvl * actualnodesthislvl * actualnodesthislvl;
			unsigned int n=0;
			unsigned int node_start = n;
			while(n < actualnodesthislvl)
			{
				//find the x, y, z coordinates
				Vec3i coords(0,0,0);
				unsigned int remainder = n;
				for(int dc=d; dc>=0; dc--)
				{
					int divisor = (1 << 3*dc);
					int div = remainder / divisor;
					remainder = remainder % divisor;
					if (div)
					{
						coords.data[0] |= ((div&4)!=0) << dc;
						coords.data[1] |= ((div&2)!=0) << dc;
						coords.data[2] |= (div&1) << dc;	
					}
				}
				BuildNode& bnode = bnodes(coords.data[0], coords.data[1], coords.data[2]);
				if (bnode.keepMe)
				{
					OctNode& onode = steps[ts].nodes[d][node];
					memcpy(onode.offsets, bnode.offsets, 8*sizeof(ST));

					Vec3i child_coords = coords * 2;
					for(int c=0; c<8; c++)
					{
						BuildNode& child_bnode = buildnodes[d+1](child_coords.data[0] + ((c&4)!=0), child_coords.data[1] + ((c&2)!=0), child_coords.data[2] + (c&1)); 
						onode.mins[c] = child_bnode.min;
						onode.maxs[c] = child_bnode.max;
					}
					onode.children_start = bnode.children_start;
					memcpy(onode.values, bnode.values, 8*sizeof(ST));
					
					bnode.myIndex = (unsigned char)(node - node_start);
					bnode.myStart = node_start;
					node++;
				}
				n++;
				if (n%8==0)
					node_start = node;
			}
		}
		else	//root node; can't group nodes into 8 because we only have 1!
		{
			BuildNode& bnode = bnodes(0,0,0);
			OctNode& onode = steps[ts].nodes[d][node];
			memcpy(onode.offsets, bnode.offsets, 8); 
			for(int c=0; c<8; c++)
			{
				BuildNode& child_bnode = buildnodes[1]((c&4)!=0, (c&2)!=0, (c&1)); 
				onode.mins[c] = child_bnode.min;
				onode.maxs[c] = child_bnode.max;
			}
			
			memcpy(onode.values, bnode.values, 8*sizeof(ST));
			onode.children_start = bnode.children_start;
			bnode.myIndex = node;
			node++;
		}
		//cout<< "caps filled = " << cap << endl;
		//cout<< "nodes filled = " << node << endl;
	}
						
	//set the min and max
	OctNode& rootnode = steps[ts].nodes[0][0];
	datamin = rootnode.mins[0];
	datamax = rootnode.maxs[0]; 
	for(int c=1; c<8; c++)
	{
		datamin = MIN(datamin, rootnode.mins[c]);
		datamax = MAX(datamax, rootnode.maxs[c]);
	}
	
	datamin = MAX(datamin, isomin);
	datamax = MIN(datamax, isomax);
	
	//cout << "datamin = " << (int)datamin << ", datamax = " << (int)datamax << endl;
	
	cout << "Total octree nodes = " << totalnodesalldepths << "/" << totalmaxnodesalldepths << " ~= " << totalnodesalldepths*100/(float)totalmaxnodesalldepths << "% full.\n\n";
                        
    delete[] buildnodes;
}

bool OctreeVolume::read_file(char* filebase)
{
    cerr << "\nReading octree volume data from \"" << filebase << "\".oth, .otd...";
    
    char filename[1024];
    sprintf(filename, "%s.oth", filebase);
    
    ifstream in(filename);
    if (!in)
        return false;
        
    in >> max_depth;
    in >> int_dims.data[0] >> int_dims.data[1] >> int_dims.data[2];
    in >> kernel_width;
    in >> datamin;
    in >> datamax;
    in >> num_timesteps;
	
	cap_depth = max_depth - 1;
	pre_cap_depth = max_depth - 2;
	
	child_bit_depth = new int[max_depth+1];
	inv_child_bit_depth = new float[max_depth+1];
	for(int d=0; d<=max_depth; d++)
	{
		child_bit_depth[d] = 1 << (max_depth - d - 1);
		inv_child_bit_depth[d] = 1.0f / static_cast<float>(child_bit_depth[d]);
	}
	max_depth_bit = 1 << max_depth;
	inv_max_depth_bit = 1.0f / static_cast<float>(max_depth_bit);
    
    dims.data[0] = (float)(int_dims.data[0]);
    dims.data[1] = (float)(int_dims.data[1]);
    dims.data[2] = (float)(int_dims.data[2]);
    padded_dims.data[0] = padded_dims.data[1] = padded_dims.data[2] = (float)max_depth_bit;
            
    bounds[0] = Vector(0.f, 0.f, 0.f);
    bounds[1] = dims;       
            
	current_timestep = 0;
	multires_level = 0;
    
    steps = new Timestep[num_timesteps];
    for(int ts = 0; ts < num_timesteps; ts++)
    {
		steps[ts].num_nodes = new unsigned int[max_depth-1];
		for(int d=0; d<max_depth-1; d++)
		{
			in >> steps[ts].num_nodes[d];
		}
		in >> steps[ts].num_caps;
    }
    
    in.close();
    
    sprintf(filename, "%s.otd", filebase);
    FILE *in2 = fopen(filename,"rb");
    if (!in2)
    {
        cerr << "OctreeVolume: error reading octree data:" << filename << endl;
        return false;
    }
    
    for(int ts = 0; ts < num_timesteps; ts++)
	{
		//read in the nodes
		steps[ts].nodes = new OctNode*[max_depth-1];
		for(int d=0; d<max_depth-1; d++)
		{
			steps[ts].nodes[d] = new OctNode[steps[ts].num_nodes[d]];
			unsigned int num_read = (unsigned int)fread((char*)steps[ts].nodes[d], sizeof(OctNode), steps[ts].num_nodes[d],in2);
			cerr << "Read " << num_read << " nodes" << " at depth " << d << endl;
			if (num_read != steps[ts].num_nodes[d])
			{
				cerr << "OctreeVolume: error reading octnodes" << endl;
        return false;
			}
		}
		//read in the caps
		steps[ts].caps = new OctCap[steps[ts].num_caps];
		unsigned int num_read = (unsigned int)fread((char*)steps[ts].caps,sizeof(OctCap),steps[ts].num_caps,in2);
		cerr << "Read " << num_read << " caps" << endl;
		if (num_read != steps[ts].num_caps)
		{
			cerr << "OctreeVolume: error reading octcaps" << endl;
      return false;
		}
	}
    fclose(in2);
    
#ifdef TEST_INDATA
    sprintf(filename, "%s.raw", filebase);
    FILE* din = fopen(filename, "rb");
    if (!din) 
    {
        cerr << "Error opening original data file: " << filename << '\n';
        return false;
    }
    indata.resize(int_dims.data[0], int_dims.data[1], int_dims.data[2]);
    cerr << "Reading indata at " << filename << "...";
    cerr << "(" << indata.get_datasize() << " read/" << int_dims.data[0] * int_dims.data[1] * int_dims.data[2] * sizeof(ST) << " expected)...";
    fread_big((void*)(&indata.get_dataptr()[0][0][0]), 1, indata.get_datasize(), din);
    cerr << "done.\n";
    fclose(din);
#endif      

    cerr << "done!" << endl;
    
    return true;
}

bool OctreeVolume::write_file(char* filebase)
{
    cout << "\nWriting octree volume data to \"" << filebase << "\".oth, .otd...";
    
    char filename[1024];
    sprintf(filename, "%s.oth", filebase);
    ofstream out(filename);
    if (!out)
        return false;
    
    out << max_depth << endl;
    out << int_dims.data[0] << " " << int_dims.data[1] << " " << int_dims.data[2] << endl;
    out << kernel_width << endl;
    out << datamin << " " << datamax << endl;
    out << num_timesteps << endl;
    for(int ts = 0; ts < num_timesteps; ts++)
	{
		for(int d=0; d<max_depth-1; d++)
			out << steps[ts].num_nodes[d] << " ";
		out << endl;
		out << steps[ts].num_caps << endl;
	}
    out.close();
	
    sprintf(filename, "%s.otd", filebase);
    FILE* out2 = fopen(filename, "wb");
    if (!out2)
        return false;
    
    for(int ts = 0; ts < num_timesteps; ts++)
	{
		//write out the nodes
		for(int d=0; d<max_depth-1; d++)
		{
			unsigned int num_written = (unsigned int)fwrite(reinterpret_cast<void*>(steps[ts].nodes[d]), sizeof(OctNode), steps[ts].num_nodes[d], out2);
			if (num_written != steps[ts].num_nodes[d])
			{
				cerr << "OctreeVolume: error writing octnodes" << endl;
				return false;
			}
		}
		//write out the caps
		unsigned int num_written = (unsigned int)fwrite(reinterpret_cast<void*>(steps[ts].caps), sizeof(OctCap), steps[ts].num_caps, out2);
		if (num_written != steps[ts].num_caps)
		{
			cerr << "OctreeVolume: error writing octcaps" << endl;
			return false;
		}
	}
    fclose(out2);
    
    cout << "done!\n";
    
    return true;
}

