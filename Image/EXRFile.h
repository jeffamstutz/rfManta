
/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institue, University of Utah

  License for the specific language governing rights and limitations under
  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#ifndef Manta_Image_EXRFile_h
#define Manta_Image_EXRFile_h

#include <string>
#include <vector>

#ifdef USE_OPENEXR
#  include <OpenEXR/ImfRgba.h>
#  include <OpenEXR/ImfHeader.h>
#else

namespace Imf
{

  struct Header;
  struct Rgba;

} // namespace Imf

#endif // USE_OPENEXR

namespace Manta 
{

  //---------------------------------------------------------------------------
  //
  // Standard Manta Image IO functions
  //  
  //---------------------------------------------------------------------------
  class Image;

  extern "C" 
  bool isEXR( const std::string& filename );

  extern "C" 
  void writeEXR( const Image* image, const std::string &filename, int which=0 );

  extern "C" 
  Image* readEXR( const std::string& filename );

  // Returns true if this reader is supported
  extern "C" 
  bool EXRSupported();


  //---------------------------------------------------------------------------
  //
  // EXR helpers 
  //  
  //---------------------------------------------------------------------------

  namespace EXR
  {

    //
    // Should be identical to Imf::PixelType
    //
    enum PixelType        
    {
      UINT  = 0,          // unsigned int (32 bit)
      HALF  = 1,          // half (16 bit floating point)
      FLOAT = 2,          // float (32 bit floating point)
      NUM_PIXELTYPES      // number of different pixel types
    };

    //
    // Represents an OpenEXR buffer slice
    // 
    struct Channel
    {
      Channel() : type( HALF ), data( NULL ) {}
      Channel( const std::string& n, PixelType t, char* d) 
        : name( n ), type( t ), data( d ) {}

      std::string     name;  // Name of the channel, eg "R", "G", "B"
      PixelType       type;  // Data type 
      char*           data;  // The raster data 
    };

    typedef std::vector<Channel*> ChannelList;
    typedef ChannelList::iterator ChannelIter;


    unsigned int pixelTypeSize( PixelType type );

    
    //-------------------------------------------------------------------------
    //
    // Wrapper for Imf::InputFile and ImfRgbaInputFile 
    //  
    //-------------------------------------------------------------------------
    class InputFile 
    {
    public:

      InputFile( const std::string& filename );
      ~InputFile();

      Image* getImage()const;
      std::string filename()const  { return _filename; }

    private:
      InputFile() {}

      std::string  _filename;  // Filename read from or last written to
      unsigned int _width;
      unsigned int _height;

      Imf::Rgba*   _rgbas;     // Array of Rgba Halfs.  Null if using channels 
      ChannelList  _channels;  // List of arbitrary channels
    };



    //-------------------------------------------------------------------------
    //
    // Wrapper for Imf::OutputFile and ImfRgbaOutputFile 
    //  
    //-------------------------------------------------------------------------
    class OutputFile 
    {
    public:
      //
      // Create output file from list of Rgba pixels 
      //
      // x            : Image width
      // y            : Image height 
      // 
      // min_x        : 
      // max_x        : Cropped data window max and min 
      // min_y        : coordinates
      // max_y        : 
      // 
      // aspect       : Pixel aspect ratio
      // screen_width : Screen window width
      //
      OutputFile( Imf::Rgba* pixels, 
               unsigned int x, 
               unsigned int y, 
               unsigned int min_x=0, 
               unsigned int max_x=0, 
               unsigned int min_y=0, 
               unsigned int max_y=0, 
               float aspect=1.0f, 
               float screen_width=1.0f
               );

      //
      // Create output file from list of channels.  Other params
      // same as Rgba* constructor 
      //
      OutputFile( const ChannelList& channels, 
               unsigned int x, 
               unsigned int y, 
               unsigned int min_x=0, 
               unsigned int max_x=0, 
               unsigned int min_y=0, 
               unsigned int max_y=0, 
               float aspect=1.0f, 
               float screen_width=1.0f 
               );


      ~OutputFile();

      //
      // Write the OutputFile to disk
      //
      void write( const std::string& filename );

    private:
      OutputFile() {}

      Imf::Header* _header;

      ChannelList  _channels;  // List of arbitrary channels
      Imf::Rgba*   _rgbas;     // Array of Rgba Halfs. Null if using channels  
    };


  } // namespace EXR

} // namespace Manta


#endif
