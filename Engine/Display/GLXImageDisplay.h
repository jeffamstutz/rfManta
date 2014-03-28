#ifndef Manta_Engine_GLXImageDisplay_h
#define Manta_Engine_GLXImageDisplay_h

#include <Interface/ImageDisplay.h>

#include <X11/Xlib.h>
#include <GL/glx.h>

namespace Manta {
	
	class SetupContext;
	class DisplayContext;
	class Image;

	class GLXImageDisplay : public ImageDisplay {
	private:
		// X Windows context structures passed to this class by the driving 
		// application.
		GLXContext  glx_context;
		GLXDrawable glx_drawable; // Or window id.
		Display     *x_display;
		XVisualInfo *x_visual_info;

    // Font info
    XFontStruct* font_info;
    GLuint fontbase;
    
    bool use_stereo;   // Setup parameter used when creating the visual
		int manta_channel; // The last channel number to call setupDisplayChannel.

    static void createVisual( bool stereo_ );
    void display_frame_rate(double framerate);
    
	public:
		// Note that the GLXImage display must be passed a GLX context by the
		// application it is embedded inside of.
		GLXImageDisplay( XVisualInfo *x_visual_info_, 
										 GLXDrawable glx_drawable_ );

    // Create a visual to use automatically.    
    GLXImageDisplay( bool stereo_, GLXDrawable glx_drawable_ );
    
		~GLXImageDisplay();
		
		// Manta ImageDisplay interface.
    void setupDisplayChannel( SetupContext &context );
    void displayImage( const DisplayContext &context, const Image* image );
		
		// Accessors.
		int getMantaChannel() const { return manta_channel; } // NOT THREAD SAFE.
	};
};

#endif
