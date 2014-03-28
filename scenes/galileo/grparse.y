
%{
/*****************************************************************************\
 *                                                                           *
 *  filename : grparse.y                                                     *
 *  author   : R. Keith Morley                                               *
 *  last mod : 04/12/04                                                      *
 *                                                                           *
 *  bison file used to create Pixar RIB parser.                              *
 *                                                                           *
\*****************************************************************************/

/* #include <core/api.h> */
/* #include <utilities/rgb.h> */
/* #include <utilities/DynArray.h> */
/* #include <math/Vector3.h> */
/* #include <parser/Params.h> */
/* #include <utilities/config.h> */

  #include <Model/Groups/Group.h>
  #include <Model/Primitives/Parallelogram.h>
//  #include <Model/Primitives/Triangle.h>
  #include <Model/Materials/Lambertian.h>
  #include <Model/Textures/Constant.h>

#include <Core/Geometry/Vector.h>

#ifdef USE_DEP_HEADERS
#include <stdlib.h>
#include <stdio.h>
#else
#include <cstdlib>
#include <cstdio>
#endif

#define MAX_PARAMS 100       /* maximum number of allowed params of a
                                single type in a single statement      */

#define MAX_NAME_LENGTH 256  /* max length of parameter name           */

extern FILE* yyin;           /*                                        */
extern int yylex();          /*  these are needed to make bison        */
extern int yyerror(char*);   /*  play well with C++ compilers          */
extern int yytext;           /*                                        */

extern int line_num;

/*  dynamic arrays to push paramlist values onto as they are parsed */
#include <vector>
#include <map>
#include <string>

 using namespace std;
 using namespace Manta;

std::vector<float> current_floats;
std::vector<std::string> current_strings;
 Material* current_material = new Lambertian(Color(RGBColor(0.2, 0.2, 0.9)));

 map<string, Material*> materials;
 map<string, Texture<Color>*> rgb_textures;

 // These will be accessed externally
 Group* world = new Group();


 struct Params {
   Params()
     {
     }

   void clear() {
     floats.clear();
     vectors.clear();
     rgb_textures.clear();
     rgb_colors.clear();
   }

   void addFloat(string name, vector<float>& value) {
     floats[name] = value;
   }

   void addVector(string name, vector<Vector>& value) {
     vectors[name] = value;
   }

   void addRGBTexture(string name, vector<Texture<Color>*>& value) {
     rgb_textures[name] = value;
   }

   void addRGB(string name, vector<RGBColor>& value) {
     rgb_colors[name] = value;
   }

   map<string, vector<float> > floats;
   map<string, vector<Vector> > vectors;
   map<string, vector<Texture<Color>*> > rgb_textures;
   map<string, vector<RGBColor> > rgb_colors;
 };

Params params;

%}


%union
{
    char string[256];
    float num;
}

%token <string> UNKNOWN
%token <string> STRING
%token <num>    NUM
%token NUMARRAY
%token STRINGARRAY
%token LBRACKET
%token RBRACKET

%token ACCEL
%token AREALIGHT
%token ATTRIBUTEBEGIN
%token ATTRIBUTEND
%token CAMERA
%token CLIPPING
%token CONCATTRANSFORM
%token DISPLACEMENT
%token ENVIRONMENT
%token FRAMEBEGIN
%token FRAMEEND
%token IDENTITY
%token IMAGE
%token INSTANCE
%token LIGHT
%token LIGHTSHADER
%token MAXDEPTH
%token LOOKAT
%token OBJECTBEGIN
%token OBJECTEND
%token PIXELSAMPLES
%token RENDERER
%token ROTATE
%token SAMPLER
%token SCALE
%token SPECTRAL
%token SURFACE
%token SURFACESHADER
%token TEXTURE
%token TONEMAP
%token TRANSFORM
%token TRANSFORMBEGIN
%token TRANSFORMEND
%token TRANSLATE
%token WORLDBEGIN
%token WORLDEND

%token HIGH_PRECEDENCE

%%

start: statement_list
;

init_arrays: %prec HIGH_PRECEDENCE
{
    current_floats.clear();
    current_strings.clear();
}
;

array: init_arrays string_array
{
}
| init_arrays num_array
{
}
;

string_array:  LBRACKET string_list RBRACKET
{
}
;

string_list: string_list string_list_entry
{
}
| string_list_entry
{
}
;

string_list_entry: STRING
{
    current_strings.push_back(std::string($1));
}
;

num_array:  LBRACKET num_list RBRACKET
{
}
;

num_list: num_list num_list_entry
{
}
| num_list_entry
{
}
;

num_list_entry: NUM
{
    current_floats.push_back($1);
}
;

param_list: init_param_list param_lists
{
}
;

init_param_list: %prec HIGH_PRECEDENCE
{
    params.clear();
}
;

param_lists: param_list_entry param_lists
{
}
|
{
}
;

param_list_entry: STRING array
{
    // add the param token/value to the global paramset here

    char type[128];
    char name[128];

    // parse the param declaration for type and name
    int num_tokens = sscanf($1, "%s %s", type, name);
    if (num_tokens != 2)
    {
        fprintf(stderr, "grparse: ERROR - Param declaration not in form ");
        fprintf(stderr, "\"type name\": line %i\n",
                 line_num);
        exit(0);
    }

    // add the param to params based on the type
    if (false)
      {
      }
#if 0
    else if (strcmp(type, "int") == 0)
    {
        size_t count = current_floats.size();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - int array declared \'%s\'", $1);
            fprintf(stderr, " current_floats empty: line %i\n", line_num);
            exit(0);
        }

        int* int_array = new int[count];
        for (size_t i = 0; i < count; ++i)
            int_array[i] = (int)current_floats[i];

        params.addInt(name, int_array, count);
        delete [] int_array;
    }
    else if (strcmp(type, "bool") == 0)
    {
        size_t count = current_floats.size();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - bool array declared \'%s\'", $1);
            fprintf(stderr, " current_floats empty: line %i\n", line_num);
            exit(0);
        }

        int* int_array = new int[count];
        for (size_t i = 0; i < count; i++)
            int_array[i] = (int)current_floats[i];

        params.addBool(name, int_array, count);
        delete []int_array;
    }
#endif
    else if (strcmp(type, "float") == 0)
    {
        size_t count = current_floats.size();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - float array declared \'%s\'", $1);
            fprintf(stderr, " current_floats empty: line %i\n", line_num);
            exit(0);
        }

        params.addFloat(name, current_floats);
    }
#if 0
    else if (strcmp(type, "string") == 0)
    {
        size_t count = current_strings.size();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - stringarray declared \'%s\'", $1);
            fprintf(stderr, " current_strings empty: line %i\n", line_num);
            exit(0);
        }

        std::string* string_array = new std::string[count];
        for (size_t i = 0; i < count; i++)
            string_array[i] = current_strings[i];

        params.addString(name, string_array, count);
        delete [] string_array;
    }
#endif
    else if (strcmp(type, "rgb_texture") == 0)
    {
        size_t count = current_strings.size();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - RGBtex array declared \'%s\'", $1);
            fprintf(stderr, " current_strings empty: line %i\n", line_num);
            exit(0);
        }

        vector<Texture<Color>*> tex_array(count);
        for (size_t i = 0; i < count; i++) {
          map<string, Texture<Color>*>::iterator rgb_texture = rgb_textures.find(current_strings[i]);
          if (rgb_texture != rgb_textures.end()) {
            tex_array[i] = (*rgb_texture).second;
          } else {
            fprintf(stderr, " grparse: ERROR - rgbtexture \'%s\' not found",
                    current_strings[i].c_str());
            fprintf(stderr, ": line %i\n", line_num);
            exit(2);
          }
        }
        params.addRGBTexture(name, tex_array);
    }
#if 0
    else if (strcmp(type, "float_texture") == 0)
    {
        size_t count = current_strings.size();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - floatex array \'%s\'", $1);
            fprintf(stderr, " current_strings empty: line %i\n", line_num);
            exit(0);
        }

        FloatTexture** tex_array = new FloatTexture*[count];
        for (size_t i = 0; i < count; i++)
        {
            FloatTexture* tex = grGetFloatTexture(current_strings[i].c_str());
            if (tex == NULL)
            {
                fprintf(stderr, " grparse: ERROR - floattex \'%s\' not found",
                         current_strings[i].c_str());
                fprintf(stderr, ": line %i\n", line_num);
                exit(0);
            }
            else
                tex_array[i] = tex;
        }
        params.addFloatTexture(name, tex_array, count);
        delete [] tex_array;
    }
    else if (strcmp(type, "image") == 0)
    {
        size_t count = current_strings.size();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - image array declared \'%s\'", $1);
            fprintf(stderr, " current_strings empty: line %i\n", line_num);
            exit(0);
        }

        Image** image_array = new Image*[count];
        for (size_t i = 0; i < count; i++)
        {
            Image* image = grGetImage(current_strings[i].c_str());
            if (image == NULL)
            {
                GRError("grparse: image \'%s\' not found, line %i\n",
                        current_strings[i].c_str(), line_num);
            }
            else
                image_array[i] = image;
        }
        params.addImage(name, image_array, count);
        delete [] image_array;
    }
    else if (strcmp(type, "object") == 0)
    {
        size_t count = current_strings.length();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - object declared \'%s\'", $1);
            fprintf(stderr, " current_strings empty: line %i\n", line_num);
            exit(0);
        }

        Surface** object_array = new Surface*[count];
        for (size_t i = 0; i < count; i++)
        {
            Surface* object = grGetObject(current_strings[i].c_str());
            if (object == NULL)
            {
                GRError("grparse: object\'%s\' not found, line %i\n",
                        current_strings[i].c_str(), line_num);
            }
            else
                object_array[i] = object;
        }
        params.addSurface(name, object_array, count);
        delete [] object_array;
    }
    else if (strcmp(type, "lshader") == 0)
    {
        size_t count = current_strings.length();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR-shader array declared \'%s\'", $1);
            fprintf(stderr, " current_strings empty: line %i\n", line_num);
            exit(0);
        }

        LightShader** shader_array = new LightShader*[count];
        for (size_t i = 0; i < count; i++)
        {
            LightShader* shader = grGetLShader(current_strings[i].c_str());
            if (shader == NULL)
            {
                fprintf(stderr, " grparse: ERROR - lshader\'%s\' not found",
                         current_strings[i].c_str());
                fprintf(stderr, ": line %i\n", line_num);
                exit(0);
            }
            else
                shader_array[i] = shader;
        }
        params.addLShader(name, shader_array, count);
        delete [] shader_array;
    }
    else if (strcmp(type, "sshader") == 0)
    {
        size_t count = current_strings.length();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR-shader array declared \'%s\'", $1);
            fprintf(stderr, " current_strings empty: line %i\n", line_num);
            exit(0);
        }

        SurfaceShader** shader_array = new SurfaceShader*[count];
        for (size_t i = 0; i < count; i++)
        {
            SurfaceShader* shader = grGetSShader(current_strings[i].c_str());
            if (shader == NULL)
            {
                fprintf(stderr, " grparse: ERROR - sshader\'%s\' not found",
                         current_strings[i].c_str());
                fprintf(stderr, ": line %i\n", line_num);
                exit(0);
            }
            else
                shader_array[i] = shader;
        }
        params.addSShader(name, shader_array, count);
        delete [] shader_array;
    }
#endif
    else if (strcmp(type, "vector") == 0)
    {
        size_t count = current_floats.size();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - vec array declared \'%s\'", $1);
            fprintf(stderr, " current_floats empty: line %i\n", line_num);
            exit(0);
        }
        if (count%3 != 0)
        {
            fprintf(stderr, "grparse: ERROR - vec array declared \'%s\'", $1);
            fprintf(stderr, " but array length not multiple of 3 : line %i\n",
                         line_num);
            exit(0);
        }

        count = count / 3;
        vector<Vector> vec_array(count);
        for (size_t i = 0; i < count; i++)
        {
            int index = i*3;
            vec_array[i] = Vector(current_floats[index+0],
                                  current_floats[index+1],
                                  current_floats[index+2]);
        }

        params.addVector(name, vec_array);
    }
    else if (strcmp(type, "rgb") == 0)
    {
        size_t count = current_floats.size();
        if (count == 0)
        {
            fprintf(stderr, "grparse: ERROR - vec array declared \'%s\'", $1);
            fprintf(stderr, " current_floats empty: line %i\n", line_num);
            exit(2);
        }
        if (count%3 != 0)
        {
            fprintf(stderr, "grparse: ERROR - rgb array declared \'%s\'", $1);
            fprintf(stderr, " but array length not multiple of 3 : line %i\n",
                         line_num);
            exit(2);
        }

        count = count / 3;
        vector<RGBColor> rgb_array(count);
        for (size_t i = 0; i < count; i++)
        {
            int index = i*3;
            rgb_array[i] = RGBColor(current_floats[index+0],
                                    current_floats[index+1],
                                    current_floats[index+2]);
        }

        params.addRGB(name, rgb_array);
    }
    else
    {
      fprintf(stderr,
              "grparse: - unknown param type \'%s\' at line %i\n",
              type, line_num);
    }
}
;

statement_list: statement_list statement
{
}
| statement
{
}
;

statement:  ACCEL STRING param_list
{

#if 0
    grAccel($2, params);
#endif
}
| AREALIGHT
{
#if 0
    grAreaLight();
#endif
}
| ATTRIBUTEBEGIN
{
#if 0
    grAttributeBegin();
#endif
}
| ATTRIBUTEND
{
#if 0
    grAttributeEnd();
#endif
}
| CAMERA STRING param_list
{

#if 0
    grCamera($2, params);
#endif
}
| CLIPPING NUM NUM
{
#if 0
    grClipping($2, $3);
#endif
}
| CONCATTRANSFORM NUM NUM NUM NUM
                  NUM NUM NUM NUM
                  NUM NUM NUM NUM
                  NUM NUM NUM NUM
{
/*   float trans[16] = { $2,  $3,  $4,  $5, */
/*                       $6,  $7,  $8,  $9, */
/*                       $10, $11, $12, $13, */
/*                       $14, $15, $16, $17}; */
#if 0
    grConcatTransform(trans);
#endif
}
| DISPLACEMENT STRING STRING param_list
{

#if 0
    grDisplacement($2, $3, params);
#endif
}
| ENVIRONMENT STRING param_list
{
#if 0
    grEnvironment($2, params);
#endif
}
| FRAMEBEGIN
{
#if 0
    grFrameBegin();
#endif
}
| FRAMEEND
{
#if 0
    grFrameEnd();
#endif
}
| IDENTITY
{
#if 0
    grIdentity();
#endif
}
| IMAGE STRING NUM NUM
{
#if 0
    grImage($2, (int)$3, (int)$4);
#endif
}
| INSTANCE STRING
{
#if 0
    grInstance($2);
#endif
}
| LIGHT STRING
{
#if 0
    grLight($2);
#endif
}
| LIGHTSHADER STRING STRING param_list
{
#if 0
    grLightShader($2, $3, params);
#endif
}
| LOOKAT NUM NUM NUM
         NUM NUM NUM
         NUM NUM NUM
{
#if 0
    grLookAt($2, $3, $4,
             $5, $6, $7,
             $8, $9, $10);
#endif
}
| MAXDEPTH NUM
{
#if 0
   grMaxDepth((int)$2);
#endif
}
| OBJECTBEGIN STRING
{
#if 0
    grObjectBegin($2);
#endif
}
| OBJECTEND
{
#if 0
    grObjectEnd();
#endif
}
| PIXELSAMPLES NUM
{
#if 0
    grPixelSamples((int)$2);
#endif
}
| RENDERER STRING param_list
{
#if 0
    grRenderer($2, params);
#endif
}
| ROTATE NUM NUM NUM NUM
{
#if 0
    grRotate($2, $3, $4, $5);
#endif
}
| SAMPLER STRING param_list
{
#if 0
    grSampler($2, params);
#endif
}
| SCALE NUM NUM NUM
{
#if 0
    grScale($2, $3, $4);
#endif
}
| SPECTRAL STRING
{
#if 0
    if (strcmp($2, "false") == 0)
    {
        grSpectral(false);
    }
    else if (strcmp($2, "true") == 0)
    {
        grSpectral(true);
    }
    else
    {
        GRWarning("grparse: Unknown Spectral flag \'%s\' line: %i", $2,
                line_num);
    }
#endif
}
| SURFACE STRING param_list
{
  if (strcmp($2, "Parallelogram") == 0) {
    Vector anchor, v1, v2;
    map<string, vector<Vector> >::iterator found = params.vectors.find("base");
    if (found == params.vectors.end()) {
      // woot woot
      fprintf(stderr, "vector base not found for Parallelogram\n");
      exit(2);
    } else {
      size_t num_vectors = (*found).second.size();
      if (num_vectors == 1) {
        anchor = (*found).second[0];
      } else {
        fprintf(stderr, "Got wrong number of base vectors (got %llu) \n",
                (unsigned long long)num_vectors);
      }
    }

    found = params.vectors.find("v");
    if (found == params.vectors.end()) {
      // woot woot
      fprintf(stderr, "vector v not found for Parallelogram\n");
      exit(2);
    } else {
      size_t num_vectors = (*found).second.size();
      if (num_vectors == 1) {
        v1 = (*found).second[0];
      } else {
        fprintf(stderr, "Got wrong number of v vectors (got %llu) \n",
                (unsigned long long)num_vectors);
      }
    }

    found = params.vectors.find("u");
    if (found == params.vectors.end()) {
      // woot woot
      fprintf(stderr, "vector u not found for Parallelogram\n");
      exit(2);
    } else {
      size_t num_vectors = (*found).second.size();
      if (num_vectors == 1) {
        v2 = (*found).second[0];
      } else {
        fprintf(stderr, "Got wrong number of u vectors (got %llu) \n",
                (unsigned long long)num_vectors);
      }
    }

    world->add(new Parallelogram(/* material */ current_material,
                                 /* anchor   */ anchor,
                                 /* v1       */ v1,
                                 /* v2       */ v2));
  } else if (strcmp($2, "Triangle") == 0) {
#if 0
    map<string, vector<Vector> >::iterator found = params.vectors.find("P");
    if (found != params.vectors.end()) {
      size_t num_vectors = (*found).second.size();
      if (num_vectors == 3) {
        world->add(new Triangle(current_material,
                                (*found).second[0],
                                (*found).second[1],
                                (*found).second[2]));
      } else {
        fprintf(stderr, "Got wrong number of P vectors (got %llu) \n",
                (unsigned long long)num_vectors);
        exit(2);
      }
    } else {
      // woot woot
      fprintf(stderr, "vector P not found for Triangle\n");
      exit(2);
    }
#else
    fprintf(stderr, "Triangle no longer handled\n");
#endif

  } else {
    fprintf(stderr, "Unknown surface object (%s)\n", $2);
  }
}
| SURFACESHADER STRING
{
  // Lookup named shader
  map<string, Material*>::iterator material = materials.find(string($2));
  if (material != materials.end()) {
    current_material = (*material).second;
  } else {
    fprintf(stderr, "Can't find named material %s", $2);
    exit(2);
  }
}
| SURFACESHADER STRING STRING param_list
{
  // Create new named shared
  if (strcmp($3, "LambertianShader") == 0) {
    map<string, vector<Texture<Color>*> >::iterator rgb_texture = params.rgb_textures.find("Kd");
    if (rgb_texture != params.rgb_textures.end()) {
      Material* material = new Lambertian((*rgb_texture).second[0]);
      materials[string($2)] = material;
      current_material = material;
    }
  } else {
    fprintf(stderr, "Unknown surface shader (%s)\n", $3);
  }
}
| TEXTURE STRING STRING STRING param_list
{
  if (strcmp($3, "ConstantTexture") == 0) {
    if (strcmp($4, "rgb") == 0) {
      map<string, vector<RGBColor> >::iterator rgb = params.rgb_colors.find("value");
      if (rgb != params.rgb_colors.end()) {
        rgb_textures[$2] = new Constant<Color>(Color((*rgb).second[0]));
      } else {
        fprintf(stderr, "Didn't find value\n");
        exit(2);
      }
    } else {
      fprintf(stderr, "Unknown texture type (%s)\n", $4);
    }
  } else {
    fprintf(stderr, "Unknown texture (%s)\n", $3);
  }
}
| TONEMAP STRING param_list
{
#if 0
    grToneMap($2, params);
#endif
}
| TRANSFORM NUM NUM NUM NUM
            NUM NUM NUM NUM
            NUM NUM NUM NUM
            NUM NUM NUM NUM
{
/*   float trans[16] = { $2,  $3,  $4,  $5, */
/*                       $6,  $7,  $8,  $9, */
/*                       $10, $11, $12, $13, */
/*                       $14, $15, $16, $17}; */
#if 0
    grTransform(trans);
#endif
}
| TRANSFORMBEGIN
{
#if 0
    grTransformBegin();
#endif
}
| TRANSFORMEND
{
#if 0
    grTransformEnd();
#endif
}
| TRANSLATE NUM NUM NUM
{
#if 0
    grTranslate($2, $3, $4);
#endif
}
| WORLDBEGIN
{
#if 0
    grWorldBegin();
#endif
}
| WORLDEND
{
    params.clear();
#if 0
    grWorldEnd();
#endif
}
;

%%

int yyerror(char* s)
{
   fprintf(stderr, "\n%s at line: %i\n", s, line_num );
   return 0;
}


