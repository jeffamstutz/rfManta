
#ifndef Manta_Engine_FileDisplay_h
#define Manta_Engine_FileDisplay_h

/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005-2006
  Scientific Computing and Imaging Institute, University of Utah

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

#include <Interface/ImageDisplay.h>
#include <string>
#include <vector>

// Abe Stephens

namespace Manta {
  using namespace std;
  class FileDisplay : public ImageDisplay {
  public:
    enum { TGA_WRITER, NRRD_WRITER };
    
    FileDisplay(const vector<string>& args);
    FileDisplay(const string &prefix, const string &type, int offset = 0, int skip = 0, bool use_timestamp_ = true, bool fps = false );
    
    virtual ~FileDisplay();
    virtual void setupDisplayChannel(SetupContext&);
    virtual void displayImage(const DisplayContext& context,
			      const Image* image);
    static ImageDisplay* create(const vector<string>& args);

    void useFrameCount(bool on) { doFrameCount = on; }
    bool useFrameCount() const { return doFrameCount; }

  protected:
    bool   display_fps;
    int    writer;
    int    current_frame;
    int    file_number;
    int    skip_frames;
    string prefix;
    bool   doFrameCount;
    string type_extension;
    float  init_time;
    bool   use_timestamp; // Use either timestamp of file number counter.
    
  private:
    FileDisplay(const FileDisplay&);
    FileDisplay& operator=(const FileDisplay&);
  };
}

#endif
