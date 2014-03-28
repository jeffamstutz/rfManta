
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


#include <Image/EXRFile.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>

#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/OutputError.h>

#include <half.h>
#include <ImfHeader.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>
#include <ImfChannelList.h>
#include <ImfArray.h>
#include <ImfFrameBuffer.h>
#include <ImathBox.h>
#include <ImathVec.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <exception>


using namespace Manta;


extern "C" 
bool isEXR( const std::string& filename )
{
  std::ifstream file(filename.c_str(), std::ios_base::binary); 

  if ( !file ) return false;

  char b[4]; 
  file.read (b, sizeof (b)); 

  return b[0] == 0x76 && b[1] == 0x2f && b[2] == 0x31 && b[3] == 0x01;
}


extern "C" 
void writeEXR( const Image* image, const std::string& file_name, 
    int which ) 
{
    throw OutputError( " writeEXR not implemented yet" );
}


extern "C" 
Image* readEXR( const std::string& filename ) 
{
  EXR::InputFile input_file( filename );
  Image* image = input_file.getImage();
  return image;
}


extern "C" 
bool EXRSupported() 
{
  return true;
}


unsigned int EXR::pixelTypeSize( EXR::PixelType type )
{
  return type == EXR::UINT ? sizeof( unsigned int ) :
         type == EXR::HALF ? sizeof( half ) :
         sizeof( float );
}


//----------------------------------------------------------------------------- 
// 
//  EXRInputFile class
// 
//----------------------------------------------------------------------------- 

EXR::InputFile::InputFile( const std::string& filename )
  : _filename( filename )
{

  // Grab the header via in InputFile
  bool is_rgba = true;
  try {

    // Test to see if we have something that can reasonably be read as Rgba
    Imf::InputFile ifile( filename.c_str() );
    const Imf::ChannelList &chans = ifile.header().channels(); 
    is_rgba = chans.findChannel( "R" ) && chans["R"].type == Imf::HALF &&
              chans.findChannel( "G" ) && chans["G"].type == Imf::HALF &&
              chans.findChannel( "B" ) && chans["B"].type == Imf::HALF;

   
  } catch( std::exception& e ) {
    throw InputError( std::string("OpenEXR lib read failed: ") + e.what() ); 
  }

  // Use the Rgba convenience class if possible
  if ( is_rgba ) {
    try {

      Imf::RgbaInputFile file( filename.c_str() ); 
      Imath::Box2i dw = file.dataWindow(); 

      _width  = dw.max.x - dw.min.x + 1; 
      _height = dw.max.y - dw.min.y + 1; 

      _rgbas = new Imf::Rgba[ _width*_height ];

      file.setFrameBuffer ( &_rgbas[0] - dw.min.x - dw.min.y*_width, 1, _width);
      file.readPixels (dw.min.y, dw.max.y); 

    } catch( std::exception& e ) {
      throw InputError( std::string("OpenEXR lib read failed: ") + e.what() ); 
    }

  // Not Rgba-like, so we will have to load it channel by channel 
  } else {
    try {
    
      Imf::InputFile file( filename.c_str() );

      Imath::Box2i dw = file.header().dataWindow(); 

      _width  = dw.max.x - dw.min.x + 1; 
      _height = dw.max.y - dw.min.y + 1; 

      Imf::FrameBuffer frame_buffer; 

      // Iterate over channels, adding a buffer slice for each
      const Imf::ChannelList &channels = file.header().channels(); 
      for ( Imf::ChannelList::ConstIterator iter = channels.begin(); 
          iter != channels.end(); ++iter ) {
   
        const Imf::Channel& chan = iter.channel(); 
        EXR::Channel* exr_chan   = new EXR::Channel();

        exr_chan->type = static_cast<EXR::PixelType>( chan.type ); 
        exr_chan->name = iter.name(); 
        unsigned int data_size  = pixelTypeSize( exr_chan->type );
        exr_chan->data = new char[ _width*_height*data_size ]; 

        frame_buffer.insert( exr_chan->name.c_str(), 
            Imf::Slice(
              chan.type,                                         // type
              exr_chan->data - dw.min.x - dw.min.y * _width,     // base 
              data_size * 1,                                     // xStride 
              data_size * _width,                                // yStride 
              chan.xSampling,                                    // x sampling
              chan.ySampling,                                    // y sampling
              0.0 ) );                                           // fillValue 
      }

      file.setFrameBuffer( frame_buffer ); 
      file.readPixels( dw.min.y, dw.max.y ); 

    } catch( std::exception& e ) {
      throw InputError( std::string("OpenEXR lib read failed: ") + e.what() ); 
    }
  }
}


EXR::InputFile::~InputFile()
{
  delete [] _rgbas;
  for (ChannelList::iterator it = _channels.begin(); 
      it != _channels.end(); ++it ) {
    delete [] (**it).data;
  }
}


Image* EXR::InputFile::getImage() const
{
  if (_rgbas) {

    SimpleImage<RGBAfloatPixel>* image = 
      new SimpleImage<RGBAfloatPixel>( false, _width, _height );

    for ( unsigned int i = 0; i < _width; ++i ) {
      for ( unsigned int j = 0; j < _height; ++j ) {

        int index = (_height-1-j) * _width + i;
        float r = _rgbas[ index ].r;
        float g = _rgbas[ index ].g;
        float b = _rgbas[ index ].b;
        RGBColor color( r, g, b ); 

        //std::cerr << float( _rgbas[ index ].r ) << " " 
        //          << float( _rgbas[ index ].g ) << " " 
        //          << float( _rgbas[ index ].b ) << std::endl;

        RGBAfloatPixel pixel;
        convertToPixel( pixel, color );
        image->set( pixel, i, j, 0 ); 
      }
    }
    return image;

  } else {
    // Need to add support for creating Image from arbitrary channel
    // list -- perhaps we only need to support single channel if the
    // image is not RGB or RGBA-like.  This would require FloatPixel
    // or LongPixel types to be added to Pixel.h
    throw InputError( "Non-RGBA image reading not fully supported yet" );
  }

}



//----------------------------------------------------------------------------- 
// 
//  EXROutputFile class
// 
//----------------------------------------------------------------------------- 

EXR::OutputFile::OutputFile( Imf::Rgba* pixels, unsigned int x, unsigned int y,
    unsigned int min_x, unsigned int max_x, unsigned int min_y,
    unsigned int max_y, float aspect, float screen_width)
: _rgbas( pixels )
{
  if ( min_x < 0 || min_x > max_x || 
       min_y < 0 || min_y > max_y ) {
    throw OutputError( " EXR::OutputFile bad data window coordinates passed \
        to constructor " );
  }

  // If zero-area data window set data window to entire display window
  Imath::Box2i dw( Imath::V2i(0, 0), Imath::V2i (x-1, y-1) );
  if ( max_x != min_x && max_y != min_y ) {
    dw = Imath::Box2i( Imath::V2i(min_x, min_y), Imath::V2i (max_x, max_y) );
  }

  _header = new Imf::Header( x, y, dw, aspect, Imath::V2f( 0.0f, 0.0f), 
      screen_width );
}


EXR::OutputFile::OutputFile( const ChannelList& channels, 
    unsigned int x,     unsigned int y, 
    unsigned int min_x, unsigned int max_x, 
    unsigned int min_y, unsigned int max_y, 
    float aspect, 
    float screen_width)
: _channels( channels )
{
  if ( min_x < 0 || min_x > max_x || 
       min_y < 0 || min_y > max_y ) {
    throw OutputError( " EXR::OutputFile bad data window coordinates passed \
        to constructor " );
  }

  Imath::Box2i dw = Imath::Box2i( Imath::V2i(min_x, min_y), 
                                  Imath::V2i(max_x, max_y) );
  
  // If zero-area data window set data window to entire display window
  if ( max_x == min_x || max_y == min_y ) {
    dw = Imath::Box2i( Imath::V2i(0, 0), Imath::V2i (x-1, y-1) );
  }

  _header = new Imf::Header( x, y, dw, aspect, Imath::V2f( 0.0f, 0.0f), 
      screen_width );

  for ( ChannelIter it = _channels.begin(); it != _channels.end(); ++it ) {
    Channel* chan = *it;
    _header->channels().insert( chan->name.c_str(), 
        static_cast<Imf::PixelType>( chan->type ) );
  }
}


EXR::OutputFile::~OutputFile()
{
  delete _header;
}


void EXR::OutputFile::write( const std::string& filename )
{

  // Get image dimensions
  Imath::Box2i data_win = _header->dataWindow(); 
  Imath::Box2i disp_win = _header->dataWindow(); 
  int width  = disp_win.max.x - disp_win.min.x + 1;
  int height = disp_win.max.y - disp_win.min.y + 1;

  // Easy case, let the OpenEXR lib do the work with RGBA images
  if ( _rgbas ) {
    
    try {
      Imf::RgbaOutputFile file ( filename.c_str(), *_header );
      file.setFrameBuffer ( _rgbas, 1, width);
      file.writePixels ( data_win.max.y - data_win.min.y + 1);
    } catch( std::exception& e ) {
      throw InputError( std::string( "OpenEXR lib write failed: " )+e.what() ); 
    }

  // Else we need to manually create a framebuffer from channels
  } else if ( _channels.size() > 0 ) {

    try {

      // Iterate over channels inserting a buffer slice for each
      Imf::FrameBuffer frameBuffer;
      for ( ChannelIter it = _channels.begin(); it != _channels.end(); ++it ) {

        Channel* chan = *it;
        int data_size = chan->type == UINT ? sizeof( unsigned int ) :
                        chan->type == HALF ? sizeof( half )    :
                        sizeof( float );

        frameBuffer.insert ( chan->name.c_str(),
            Imf::Slice ( static_cast<Imf::PixelType>( chan->type ),
              chan->data,
              data_size * 1,
              data_size * width ) );
      }

      // Write out the image
      Imf::OutputFile file(filename.c_str(), *_header);
      file.setFrameBuffer (frameBuffer);
      file.writePixels (height);


    } catch( std::exception& e ) {
      throw InputError( std::string( "OpenEXR lib write failed: " )+e.what() ); 
    }

  } else {
    throw OutputError( "EXROutputFile::write called without Pixel data" );
  }
}




