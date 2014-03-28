
#define GL_GLEXT_PROTOTYPES

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


#include <MantaSSE.h>
#include <Interface/Image.h>
#include <Interface/Fragment.h>
#include <Core/Thread/Time.h>
#include <Core/Math/MinMax.h>
// #include <Interface/ImageTraverser.h>
// #include <Interface/PixelSampler.h>
// #include <Interface/LoadBalancer.h>

#include <Engine/Display/PureOpenGLDisplay.h>
#include <Image/SimpleImage.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace Manta;

PureOpenGLDisplay* ogl = NULL;
Image* images[2] = { NULL, NULL };

int xres_global = 1312;
int yres_global = 800;
bool stereo_global = false;
vector<string> image_args;
int display_image = 0;
int render_image = 1;
//string display_mode("texture");
//string display_mode("image");
string display_mode("pbo");
string image_type("bgra8");

void fillImage(Image* image);

int warmup_frames = 30;
int bench_frames = 100;
int frame = 0;
int master_frame = 0;
double start_time, end_time;

float red = 0.3f, green = 0.6f, blue = 0.9f;

void
gl_print_error(const char *file, int line)
{
  GLenum errcode = glGetError();
  if(errcode != GL_NO_ERROR)
    cerr << "GLError: "
         << file << ":"
         << line << " error: "
         << gluErrorString(errcode) << "\n";
}

float
change_color(float val)
{
  float color = val + (drand48()-0.5)*0.05;
  if (color < 0) color = 0;
  if (color > 1) color = 1;
  return color;
}

void
update_colors()
{
  red   = change_color(red);
  green = change_color(green);
  blue  = change_color(blue);
}

void init(void)
{
  glClearColor (.05, .1, .2, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glShadeModel(GL_FLAT);

  for(int i = 0; i < 2; ++i) {
    if (image_type == "bgra8") {
      images[i] = SimpleImage<BGRA8Pixel>::create(image_args, stereo_global,
                                                  xres_global, yres_global);
    } else if (image_type == "rgbafloat") {
      images[i] = SimpleImage<RGBAfloatPixel>::create(image_args,
                                                      stereo_global,
                                                      xres_global,
                                                      yres_global);
    } else {
      cerr << "Unknown image type "<<image_type<<"\n";
      exit(1);
    }
  }

//   glClear(GL_COLOR_BUFFER_BIT);
//   glutSwapBuffers();
//   glClear(GL_COLOR_BUFFER_BIT);
//   glutSwapBuffers();

  ogl = new PureOpenGLDisplay(display_mode);
  ogl->init();
  fillImage(images[render_image]);
  fillImage(images[display_image]);
}

void display(void)
{
  //glClear(GL_COLOR_BUFFER_BIT);
  ogl->displayImage(images[display_image]);
  //update_colors();
  fillImage(images[render_image]);
  display_image = 1 - display_image;
  render_image  = 1 - render_image;
  glutSwapBuffers();
  frame++;
  master_frame++;
  if (frame == warmup_frames) {
    start_time = Time::currentSeconds();
    cout << "Frame = "<<master_frame<<"\n";
  } else if (frame == (bench_frames+warmup_frames)) {
    end_time = Time::currentSeconds();
    cout << "Frame = "<<master_frame<<"\n";
    cout << "Display mode = "<<display_mode<<"\n";
    cout << "Benchmark completed in " << end_time-start_time
         << " seconds (" << bench_frames << " frames, "
         << bench_frames/(end_time-start_time) << " frames per second)\n";
    if (display_mode == "pbo") {
      display_mode = "texture";
      frame = 0;
    } else if (display_mode == "texture") {
      display_mode = "image";
      frame = 0;
    } else {
      exit(0);
    }
    ogl->setMode(display_mode);
  }
  glutPostRedisplay();
}

void reshape(int w, int h)
{
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, (GLfloat) w/(GLfloat) h, 1.0, 30.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -3.6);
}

void keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
  case 'q':
    exit(0);
    break;
  default:
    glutPostRedisplay();
    break;
  }
}

#if 1
#  define RED   0.3f
#  define GREEN 0.6f
#  define BLUE  0.9f
#else
#  define RED   red
#  define GREEN green
#  define BLUE  blue
#endif

void fillFragment(Fragment& fragment) {
#ifdef MANTA_SSE
  if(((fragment.pixelBegin | fragment.pixelEnd) & 0x3) == 0){
    __m128 r = _mm_set1_ps( RED );
    __m128 g = _mm_set1_ps( GREEN );
    __m128 b = _mm_set1_ps( BLUE );
    for(int i=fragment.pixelBegin; i < fragment.pixelEnd; i+=4){
      _mm_store_ps(&fragment.color[0][i], r);
      _mm_store_ps(&fragment.color[1][i], g);
      _mm_store_ps(&fragment.color[2][i], b);
    }
  } else
#endif /* MANTA_SSE */
  {
    for(int i=fragment.pixelBegin;i<fragment.pixelEnd;i++){
      fragment.color[0][i] = RED;
      fragment.color[1][i] = GREEN;
      fragment.color[2][i] = BLUE;
    }
  }
}


void fillImage(Image* image) {
  int xres, yres;
  bool stereo;
  image->getResolution(stereo, xres, yres);

//   Fragment fragment(Fragment::LineShape,
//                     Fragment::ConsecutiveX|Fragment::ConstantEye);
//   fragment.setSize(Fragment::MaxSize);
//   fillFragment(fragment);

#ifdef MANTA_SSE
  __m128i vec_4 = _mm_set1_epi32(4);
  __m128i vec_cascade = _mm_set_epi32(3, 2, 1, 0);
#endif

  for(int y = 0; y < yres; ++y)
    for(int x = 0; x < xres; x+= Fragment::MaxSize) {
      int xstart = x;
      int xend = xstart + Fragment::MaxSize;
      int fsize = Min(Fragment::MaxSize, xend-xstart);
      Fragment fragment(Fragment::LineShape,
                        Fragment::ConsecutiveX|Fragment::ConstantEye);

#ifdef MANTA_SSE
      int e = (fsize+3)&(~3);
      __m128i vec_eye = _mm_set1_epi32(0);
      for(int i=0;i<e;i+=4)
        _mm_store_si128((__m128i*)&fragment.whichEye[i], vec_eye);
#else
      for(int i=0;i<fsize;i++)
        fragment.whichEye[i] = 0;
#endif

      int size = xend-xstart;
      fragment.setSize(size);

#ifdef MANTA_SSE
      __m128i vec_x = _mm_add_epi32(_mm_set1_epi32(xstart), vec_cascade);
      for(int i=0;i<size;i+=4){
        // This will spill over by up to 3 pixels
        _mm_store_si128((__m128i*)&fragment.pixel[0][i], vec_x);
        vec_x = _mm_add_epi32(vec_x, vec_4);
      }
#else
      for(int i=0;i<size;i++)
        fragment.pixel[0][i] = i+xstart;
#endif

#ifdef MANTA_SSE
      __m128i vec_y = _mm_set1_epi32(y);
      for(int i=0;i<e;i+=4)
        _mm_store_si128((__m128i*)&fragment.pixel[1][i], vec_y);
#else
      for(int i=0;i<fsize;i++)
        fragment.pixel[1][i] = y;
#endif

      fillFragment(fragment);
      image->set(fragment);
    }
}

int main(int argc, char* argv[]) {

  glutInit(&argc, argv);

  for(int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "-imagetype") {
      image_type = argv[++i];
    } else if (arg == "-bench") {
      bench_frames  = atoi(argv[++i]);
      warmup_frames = atoi(argv[++i]);
    }
  }
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(xres_global, yres_global);
  glutInitWindowPosition(100, 100);
  glutCreateWindow(argv[0]);
  init();
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc (keyboard);
  glutMainLoop();


  return 0;
}
