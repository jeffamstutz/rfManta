#include <Model/Readers/UDAReader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <Core/Util/Endian.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include <iomanip>
#include <fstream>
#include <fcntl.h>


#include <zlib.h>
#include <stdint.h>

using namespace std;
using namespace Manta;

static void parseLevel(xmlDocPtr doc, xmlNodePtr cur, UDAReader::Timestep& timestep);
static void parsePatch(xmlDocPtr doc, xmlNodePtr cur, UDAReader::Timestep& timestep);

static string parseDirectory(string file)
{
  return file.substr(0,file.find_last_of("/")+1);
}

static string parseXMLString(string st)
{
    for (unsigned int i=0;i<st.length();i++)
    {
        if (st[i] == '[' || st[i] == ']' || st[i] == ',')
            st[i] = ' ';
    }
    return st;
}

void UDAReader::readUDAHeader(string directory)
{
  string filename;
        assert(directory.length());
        if (directory[directory.length()-1] != '/')
            directory = directory + string("/");
        _directory = directory;
        filename = directory + string("index.xml");
        cout << "UDAReader, reading header from: " << filename << endl;
        xmlDocPtr doc; 
        xmlNodePtr cur; 
        doc = xmlParseFile(filename.c_str()); 
        if (doc == NULL ) { 
            cerr << "Document not parsed successfully. \n";
            return; 
        } 
        cur = xmlDocGetRootElement(doc); 
        if (cur == NULL) { 
            fprintf(stderr,"empty document\n"); 
            xmlFreeDoc(doc); 
            return; 
        }
        if (xmlStrcmp(cur->name, (const xmlChar *) "Uintah_DataArchive")) { 
            cerr << "document of the wrong type, root node != Uintah_specification";
            xmlFreeDoc(doc); 
            return; 
        } 
        cur = cur->xmlChildrenNode; 
        while (cur != NULL) 
        { 
	  if ((!xmlStrcmp(cur->name, (const xmlChar *)"variables")))
            { 
	       xmlNodePtr t = cur->xmlChildrenNode;
                while (t != NULL)
                {
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"variable")))
                    {
		      string name = string((const char*)xmlGetProp(t,(const xmlChar*)"name"));
                        string type = string((const char*)xmlGetProp(t,(const xmlChar*)"type"));
			if ((type.find("double") != string::npos || type.find("float") != string::npos) && type.find("CCVariable") != string::npos)
			  _varHeaders[name] = VarHeader(name, "CCVariable", "float");
                    }
                    t = t->next;
		}
	    }
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"timesteps")))
            { 
                xmlNodePtr t = cur->xmlChildrenNode;
                while (t != NULL)
                {
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"timestep")))
                    {
		      //TODO: track timesteps
                    }
                    t = t->next;
                }
            } 
            cur = cur->next; 
        } 
        xmlFreeDoc(doc);
	cout << "UDAReader Header complete\n";
}

void UDAReader::readUDA(string directory, string volumeVarName)
    {
      _volumeVarName = volumeVarName;
        string filename;
        assert(directory.length());
        if (directory[directory.length()-1] != '/')
            directory = directory + string("/");
        _directory = directory;
        filename = directory + string("index.xml");
        cout << "UDAReader, reading data: " << filename << endl;
        xmlDocPtr doc; 
        xmlNodePtr cur; 
        doc = xmlParseFile(filename.c_str()); 
        if (doc == NULL ) { 
            cerr << "Document not parsed successfully. \n";
            return; 
        } 
        cur = xmlDocGetRootElement(doc); 
        if (cur == NULL) { 
            fprintf(stderr,"empty document\n"); 
            xmlFreeDoc(doc); 
            return; 
        }
        if (xmlStrcmp(cur->name, (const xmlChar *) "Uintah_DataArchive")) { 
            cerr << "document of the wrong type, root node != Uintah_specification";
            xmlFreeDoc(doc); 
            return; 
        } 
        cur = cur->xmlChildrenNode; 
        while (cur != NULL) 
        {   
	  
	  if ((!xmlStrcmp(cur->name, (const xmlChar *)"Meta")))
            { 
                xmlNodePtr t = cur->xmlChildrenNode;
                while (t != NULL)
                {
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"endianness")))
                    {
		      _endianness = string((const char*)xmlNodeListGetString(doc, t->xmlChildrenNode, 1));
                    }
                    t = t->next;
                }
            } 
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"timesteps")))
            { 
                xmlNodePtr t = cur->xmlChildrenNode;
                while (t != NULL)
                {
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"timestep")))
                    {
                        parseTimestepFile(string(_directory+string((const char*)xmlGetProp(t,(const xmlChar*)"href"))));
                    }
                    t = t->next;
                }
            } 
            cur = cur->next; 
        } 
	cout << "timesteps: \n";
	for(int i = 0; i < int(timesteps.size()); i++){
	  cout << "timestep : " << i << endl;
	  cout << "numSphereVars: " << timesteps[i].numSphereVars << " numSpheres: " << timesteps[i].numSpheres << endl;
	  float* s = timesteps[i].sphereData;
	  for(int j = 0; j < timesteps[i].numSpheres; j++) {
	    cout << "sphere " << j << " : " << *s << " " << *(s+1) << " " << *(s+2) << endl;
	    s+=3;
	  }
	    cout << "upper: " << timesteps[i].upper << endl;
	    cout << "lower: " << timesteps[i].lower << endl;
	    cout << "indices: " << timesteps[i].indices << endl;
	}
        xmlFreeDoc(doc);
	cout << "UDAReader Data complete\n";
    }
    
    void UDAReader::parseTimestepFile(string filename)
    {
      timesteps.push_back(Timestep());
        UDAReader::Timestep& timestep = timesteps[timesteps.size()-1];
	timestep.dir = parseDirectory(filename);
        cout << "Parsing Timestep: " << filename << "\n";
        xmlDocPtr doc; 
        xmlNodePtr cur; 
        doc = xmlParseFile(filename.c_str()); 
        if (doc == NULL ) { 
            cerr << "Document not parsed successfully. \n";
            return; 
        } 
        cur = xmlDocGetRootElement(doc); 
        if (cur == NULL) { 
            fprintf(stderr,"empty document\n"); 
            xmlFreeDoc(doc); 
            return; 
        }
        if (xmlStrcmp(cur->name, (const xmlChar *) "Uintah_timestep")) { 
            cerr << "document of the wrong type, root node != Uintah_specification";
            xmlFreeDoc(doc); 
            return; 
        } 
        cur = cur->xmlChildrenNode; 
        while (cur != NULL) 
        { 
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"Grid")))
            { 
                xmlNodePtr t = cur->xmlChildrenNode;
                while (t != NULL)
                {
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"Level")))
                    {
                        parseLevel(doc, t, timestep);
                    }
                    t = t->next;
                }
            } 
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"Data")))
            { 
                xmlNodePtr t = cur->xmlChildrenNode;
                while (t != NULL)
                {
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"Datafile")))
                    {
		      string file = string(timestep.dir + string((const char*)xmlGetProp(t,(const xmlChar*)"href")));
		      if (file.find("l0") != string::npos)
                        parseDataFile(file, timestep);
                    }
                    t = t->next;
                }
            } 
            cur = cur->next; 
        } 
        xmlFreeDoc(doc); 
	//cout << "parsing timestep done.\n" << "lower: " << timestep.lower.x() << " " << timestep.lower.y() << " " << timestep.lower.z() << endl;
	//cout << "upper: " << timestep.upper.x() << " " << timestep.upper.y() << " " << timestep.upper.z() << endl;
	//cout << "indices: " << timestep.indices.x() << " " << timestep.indices.y() << " " << timestep.indices.z() << endl;
	//read in data files
 int numSpheres = 0;
 int numSphereVars = 0;
	for(map<string, vector<VarInfo> >::iterator itr = timestep.dataMapping.begin(); itr != timestep.dataMapping.end(); itr++)
	  {
	      vector<VarInfo>& vars = timestep.dataMapping[itr->first];
	      //find particles' position first, should be in first 3 indices
 	      int tempSphereVars = 0;
	      for (vector<VarInfo>::iterator itr = vars.begin(); itr != vars.end(); itr++)
		{
		  VarInfo& var = (*itr);

		  if (var.type == particleVariable)
		    {
		      if (var.dataType == pointT)
			tempSphereVars += 3;
		      var.dataIndex = 0;
		      numSpheres = max(var.numParticles, numSpheres);
		    }
		}
	      numSphereVars = max(numSphereVars, tempSphereVars);
	  }
	 timestep.volume->resize(int(timestep.indices[0]+1), int(timestep.indices[1]+1), int(timestep.indices[2]+1));
	  timestep.numSphereVars = numSphereVars;
	  timestep.numSpheres = numSpheres;
	  cout  << "numSphereVars: " << timestep.numSphereVars << endl;
	  cout << "numSpheres: " << timestep.numSpheres << endl;
	  timestep.sphereData = new float[timestep.numSphereVars*timestep.numSpheres];
	for(map<string, vector<VarInfo> >::iterator itr = timestep.dataMapping.begin(); itr != timestep.dataMapping.end(); itr++)
	  {
	    //cout << "reading filename: " << itr->first << endl;
	  readData(itr->first, timestep);
	  }
	cout << "done parsing timestep\n number of particles read: " << timestep.numSpheres << 
	  "\n number of particle variables read in: " << timestep.numSphereVars
	     << "\n number of volume indices read in: " << timestep.indices[0]+1 << " " <<
	  timestep.indices[1]+1 << " " << timestep.indices[2]+1 << endl;
    }

Vector UDAReader::readPoint(char* p)
{
  double x = *((double*)p);
  p = p + sizeof(double);
  double y = *((double*)p);
  p = p + sizeof(double);
  double z = *((double*)p);
  p = p + sizeof(double);
  return Vector(x,y,z);
}

float UDAReader::readFloat(char* p)
{
  return *((float*)p);
}

double UDAReader::readDouble(char* p)
{
  return *((double*)p);
}

#define SWAP_2(u2)/* IronDoc macro to swap two byte quantity */ \
  { unsigned char* _p = (unsigned char*)(&(u2)); \
    unsigned char _c =   *_p; *_p = _p[1]; _p[1] = _c; }
#define SWAP_4(u4)/* IronDoc macro to swap four byte quantity */ \
  { unsigned char* _p = (unsigned char*)(&(u4)); \
    unsigned char  _c =   *_p; *_p = _p[3]; _p[3] = _c; \
                   _c = *++_p; *_p = _p[1]; _p[1] = _c; }
#define SWAP_8(u8)/* IronDoc macro to swap eight byte quantity */ \
  { unsigned char* _p = (unsigned char*)(&(u8)); \
    unsigned char  _c =   *_p; *_p = _p[7]; _p[7] = _c; \
                   _c = *++_p; *_p = _p[5]; _p[5] = _c; \
                   _c = *++_p; *_p = _p[3]; _p[3] = _c; \
                   _c = *++_p; *_p = _p[1]; _p[1] = _c; }


/*static void swapbytes(bool&) { }
static void swapbytes(int8_t&) { }
static void swapbytes(uint8_t&) { }
static void swapbytes(int16_t& i) { SWAP_2(i); }
static void swapbytes(uint16_t& i) { SWAP_2(i); }
static void swapbytes(int32_t& i) { SWAP_4(i); }*/
static void swapbytes(uint32_t& i) { SWAP_4(i); }
//static void swapbytes(int64_t& i) { SWAP_8(i); }
static void swapbytes(uint64_t& i) { SWAP_8(i); }
/*static void swapbytes(float& i){SWAP_4(i);}
static void swapbytes(double& i){SWAP_8(i);}
//void swapbytes(Point &i){ // probably dangerous, but effective
//    double* p = (double *)(&i);
//    SWAP_8(*p); SWAP_8(*++p); SWAP_8(*++p); }
static void swapbytes(Vector &i){ // probably dangerous, but effective
     double* p = (double *)(&i);
     SWAP_8(*p); SWAP_8(*++p); SWAP_8(*++p); }*/



static unsigned long convertSizeType(uint64_t* ssize, bool swapBytes, int nByteMode)
{
  if (nByteMode == 4) {
    uint32_t size32 = *(uint32_t*)ssize;
    if (swapBytes) swapbytes(size32);
    return (unsigned long)size32;
  }
  else if (nByteMode == 8) {
    uint64_t size64 = *(uint64_t*)ssize;
    if (swapBytes) swapbytes(size64);

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma set woff 1209 // constant controlling expressions (sizeof)
#endif  
    if (sizeof(unsigned long) < 8 && size64 > 0xffffffff)
        throw InternalError("Overflow on 64 to 32 bit conversion");
#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma reset woff 1209
#endif

    return (unsigned long)size64;
  }
  else {
    throw InternalError("Must be 32 or 64 bits");
  }
}


void UDAReader::readData(string filename, Timestep& t)
{     
  vector<VarInfo>& vars = t.dataMapping[filename];
  string file = t.dir + string("l0/") + filename; //TODO: unhardcode the l0 
  //find particles' position first, should be in first 3 indices
  cout << "reading file: " <<  file << endl;
  for (vector<VarInfo>::iterator itr = vars.begin(); itr != vars.end(); itr++)
    {
      VarInfo& var = (*itr);
      if (var.name == _volumeVarName)
	{
	  var.dataIndex = 0;
	}
    }
   ifstream in(file.c_str(), ios::binary | ios::in);
   if (!in)
     return;
   for (vector<VarInfo>::iterator itr = vars.begin(); itr != vars.end(); itr++)
    {  
      VarInfo& var = (*itr);
      if (var.name != _volumeVarName && var.type != particleVariable)
	continue;
      in.seekg(var.start);
      string bufferStr;
      char* buffer = (char*)bufferStr.c_str();
      if (var.compressed)
	{
	  //cerr << "need to compress data\n";
	  int nByteMode = sizeof(float); //TODO: unhardcode!
	string data;
	int datasize = var.end-var.start+1;
	//string bufferStr;
	string* uncompressedData = &data;
	//FILE* ip;
	//ip = fopen(file.c_str(), "r");
	//fseek(ip, var.start, 0);
#ifdef _WIN32
  int fd = open(file.c_str(), O_RDONLY|O_BINARY);
#else
  int fd = open(file.c_str(), O_RDONLY);
#endif
  if(fd == -1) {
    cerr << "Error opening file: " << file.c_str() << '\n';
    //throw ErrnoException("DataArchive::query (open call)");
  }
  off_t ls = lseek(fd, var.start, SEEK_SET);

  if(ls == -1) {
    cerr << "Error lseek - file: " << file.c_str() << '\n';
    //throw ErrnoException("DataArchive::query (lseek call)");
  }


	data.resize(datasize);
	#ifdef _WIN32
	// casting from const char* -- use caution

	ssize_t s = ::_read(fd, const_cast<char*>(data.c_str()), datasize);
    #else
	ssize_t s = ::read(fd, const_cast<char*>(data.c_str()), datasize);
    #endif
	
	if(s != datasize) {
	  cerr << "Error reading file: " << file << '\n';
	  //SCI_THROW(ErrnoException("Variable::read (read call)", errno, __FILE__, __LINE__));
	}

	//ic.cur += datasize;
	
	bool use_gzip = true;
	bool swapBytes = false;
	if (_endianness == "")
	  {
	    cerr << "warning: endianness of file not specified, guessing big endian\n";
	    _endianness = "big_endian";
	  }
	string machine = "little_endian";
	if (is_big_endian())
	  machine = "big_endian";
	if (machine != _endianness)
	  swapBytes = true;
	if (use_gzip) {
	  // use gzip compression

	  // first read the uncompressed data size
	  istringstream compressedStream(data);
	  uint64_t uncompressed_size_64;
	  compressedStream.read((char*)&uncompressed_size_64, nByteMode);

	  unsigned long uncompressed_size = convertSizeType(&uncompressed_size_64, swapBytes, nByteMode);
	  const char* compressed_data = data.c_str() + nByteMode;

	  long compressed_datasize = datasize - (long)(nByteMode);

	  // casting from const char* below to char* -- use caution
	  bufferStr.resize(uncompressed_size);
	  buffer = (char*)bufferStr.c_str();

	  int result = uncompress( (Bytef*)buffer, &uncompressed_size,
				   (const Bytef*)compressed_data, compressed_datasize );

	  if (result != Z_OK) {
	    printf( "Uncompress error result is %d\n", result );
	    if (result == Z_MEM_ERROR)
	      cout << "not enough memory\n";
	    else if (result == Z_BUF_ERROR)
	      cout << "not enough room in the output buffer\n";
	    else if (result == Z_DATA_ERROR)
	      cout << "input data corrupted\n";
	    //throw InternalError("uncompress failed in Uintah::Variable::read", __FILE__, __LINE__);
	  }

	  uncompressedData = &bufferStr;
	}
	
      }
      else
	{
	  bufferStr.resize(var.end-var.start + 1);
	  in.read(buffer, var.end-var.start);
	}

      char* bufferP = buffer;
      float* sphereDataPtr = t.sphereData;
      if (var.type == particleVariable && var.dataType ==pointT)
      {
	 for (int i = 0; i < var.numParticles; i++)
	  {		  
	    Vector pos = readPoint(bufferP);
		  bufferP += sizeof(double)*3;
		  sphereDataPtr[0] = pos[0];
		  sphereDataPtr[1] = pos[1];
		  sphereDataPtr[2] = pos[2];	  
	      sphereDataPtr += t.numSphereVars;
	      }
      }
      else if (var.name == _volumeVarName)
	{
	  cout << "parsing volume data\n";
	  Patch& p = t.patches[var.patchId];
	  Vector l = p.lowIndex; 
	  Vector h = p.highIndex;
	  cout << "l: " << l[0] << " " << l[1] << " " << l[2] <<
	    "\nh: " << h[0] << " " << h[1] << " " << h[2] << endl;
	  cout << "data size: " << bufferStr.length()/4 << endl;
	  int l0 = int(l[0]), l1 = int(l[1]), l2 = int(l[2]), h0 = int(h[0]), h1 = int(h[1]), h2 = int(h[2]);
	  for( int x = l0; x <= h0; x++)
	    {
	      for(int y = l1; y <= h1; y++)
		{
		  for(int z = l2; z <= h2; z++)
		    {
		      float value = 0;
		      if (var.dataType == floatT)
			{
			  value = readFloat(bufferP);
			  bufferP += sizeof(float);
			}
		      if (var.dataType == doubleT)
			{
			  value = readDouble(bufferP);
			  bufferP += sizeof(double);
			}
		      (*(t.volume))(x,y,z) = value;
		    }
		}
	    }
	}
    }
}

    void UDAReader::parseDataFile(string filename, Timestep& timestep)
    {
       cout << "parsing datafile: " << filename << endl;
        xmlDocPtr doc; 
        xmlNodePtr cur; 
        doc = xmlParseFile(filename.c_str()); 
        if (doc == NULL ) { 
            cerr << "Document not parsed successfully. \n";
            return; 
        } 
        cur = xmlDocGetRootElement(doc); 
        if (cur == NULL) { 
            fprintf(stderr,"empty document\n"); 
            xmlFreeDoc(doc); 
            return; 
        }
        if (xmlStrcmp(cur->name, (const xmlChar *) "Uintah_Output")) { 
            cerr << "document of the wrong type, root node != Uintah_specification";
            xmlFreeDoc(doc); 
            return; 
        } 
        int numvars = 0;
        for (cur = cur->xmlChildrenNode; cur != NULL; cur=cur->next) 
        { 
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"Variable")))
            { 
                UDAReader::VarInfo v;
                xmlNodePtr t = cur->xmlChildrenNode;
		string type = string((const char*)xmlGetProp(cur,(const xmlChar*)"type"));
		//cout << "type: " << type << endl;
		if (type.find("ParticleVariable") != string::npos)
		   v.type = particleVariable; 
		else if (type.find("CCVariable") != string::npos)
		  v.type = cCVariable;
		else 
		  continue;
		if (type.find("Point") != string::npos)
		  v.dataType = pointT;
		else if (type.find("float") != string::npos)
		  v.dataType = floatT;
		else if (type.find("double") != string::npos)
		  v.dataType = doubleT;
                while (t != NULL)
                {
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"variable")))
                    {
                       v.name = string((const char*)xmlNodeListGetString(doc, t->xmlChildrenNode, 1));
                    }
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"index")))
                    {
                       stringstream s((const char*)xmlNodeListGetString(doc, t->xmlChildrenNode, 1));
                       s >> v.index;  
                  }
		     if ((!xmlStrcmp(t->name, (const xmlChar*)"patch")))
                    {
		      stringstream s((const char*)xmlNodeListGetString(doc, t->xmlChildrenNode, 1));
		      s >> v.patchId;
                    }
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"start")))
                    {
		      stringstream s((const char*)xmlNodeListGetString(doc, t->xmlChildrenNode, 1));
		      s >> v.start;
                    }
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"end")))
                    {
		      stringstream s((const char*)xmlNodeListGetString(doc, t->xmlChildrenNode, 1));
		      s >> v.end;
                    }
                    if ((!xmlStrcmp(t->name, (const xmlChar*)"filename")))
                    {
		       v.filename = string((const char*)xmlNodeListGetString(doc, t->xmlChildrenNode, 1));
		       //  cout << "adding data filename: " << v.filename << endl;
                    }
		    if ((!xmlStrcmp(t->name, (const xmlChar*)"numParticles")))
                    {
		      stringstream s((const char*)xmlNodeListGetString(doc, t->xmlChildrenNode, 1));
		      s >> v.numParticles;
		      cout << "numParticles: " << s.str() << " " << v.numParticles << endl;
                    }
		    if ((!xmlStrcmp(t->name, (const xmlChar*)"compression")))
                    {
		      v.compressed = true;
                    }
                    t = t->next;
                }
		timestep.dataMapping[v.filename].push_back(v);
		numvars++;
            }
        }
	cout << "numvars read: " << numvars << endl;
        xmlFreeDoc(doc);
    }

    
    void parseLevel(xmlDocPtr doc, xmlNodePtr cur, UDAReader::Timestep& timestep)
    {
        cout << "Parsing Level\n";
        cur = cur->xmlChildrenNode;
        while (cur != NULL) 
        { 
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"numpatches")))
            { 
                int numpatches;
                stringstream s(parseXMLString((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
                s >> numpatches;
            }
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"cellspacing")))
            { 
                int x, y, z;
                stringstream s(parseXMLString((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
                s >> x >> y >> z;
		//TODO: need cellspacing?
            }
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"Patch")))
            {
                parsePatch(doc, cur, timestep);
            }
            cur = cur->next; 
        } 
    }
    void parsePatch(xmlDocPtr doc, xmlNodePtr cur, UDAReader::Timestep& timestep)
    {
      // cout << "Parsing Patch\n";
        UDAReader::Patch patch;
        cur = cur->xmlChildrenNode;
        while (cur != NULL) 
        { 
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"id")))
            { 
                stringstream s(parseXMLString((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
                s >> patch.id;
            }
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"lower")))
            { 
                stringstream s(parseXMLString((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
                s >> patch.lower[0] >> patch.lower[1] >> patch.lower[2];
                timestep.lower[0] = max(patch.lower[0],timestep.lower[0]);
                timestep.lower[1] = max(patch.lower[1],timestep.lower[1]);
                timestep.lower[2] = max(patch.lower[2],timestep.lower[2]);
            } 
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"upper")))
            { 
                stringstream s(parseXMLString((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
                s >> patch.upper[0] >> patch.upper[1] >> patch.upper[2];
                timestep.upper[0] = max(patch.upper[0],timestep.upper[0]);
                timestep.upper[1] = max(patch.upper[1],timestep.upper[1]);
                timestep.upper[2] = max(patch.upper[2],timestep.upper[2]);
            }
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"lowIndex")))
            { 
                stringstream s(parseXMLString((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
                s >> patch.lowIndex[0] >> patch.lowIndex[1] >> patch.lowIndex[2];
		patch.lowIndex[0]++;
		patch.lowIndex[1]++;
		patch.lowIndex[2]++;
            }
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"interiorHighIndex")))
            { 
                stringstream s(parseXMLString((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1)));
                s >> patch.highIndex[0] >> patch.highIndex[1] >> patch.highIndex[2];
		timestep.indices[0] = max(patch.highIndex[0],timestep.indices[0]);
                timestep.indices[1] = max(patch.highIndex[1],timestep.indices[1]);
                timestep.indices[2] = max(patch.highIndex[2],timestep.indices[2]);
            }
            cur = cur->next; 
        } 
	timestep.patches[patch.id] = patch;
    }


