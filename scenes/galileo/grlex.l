
%{

/*****************************************************************************\
 *                                                                           *
 *  filename : lexrib.l                                                      *
 *  author   : R. Keith Morley                                               *
 *                                                                           *
 *  flex file for generating a Galileo lexer.                                *
 *                                                                           *
\*****************************************************************************/

#include <grparse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_INCLUDE_DEPTH 10
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;

int line_num = 1;
int col_num = 1;

%}

wspace   [\t ]+
nl       \n
num      [0-9]
alpha    [a-zA-Z]
alphanum [a-zA-Z0-9]
ident    [a-zA-Z_][a-zA-Z0-9_]*
decimal  [-+]?([0-9]+|(([0-9]+)|([0-9]+\.[0-9]*)|(\.[0-9]+))([eE][-+]?[0-9]+)?)
comment  #[^\n]*\n
eatline  [^\n]*\n
qstring  \"[^\"\n]*\"
larraybracket \[  
rarraybracket \] 

%x INCLUDEMODE 

%%

{wspace}               { ; /* ignore whitespace */       }
{nl}                   { line_num++;                     }
{comment}              { line_num++;                     }
{qstring}              {                      
                          strcpy(yylval.string, &yytext[1]);
                          yylval.string[yyleng-2] = '\0';
                          return STRING;    
                       }                   
{decimal}              {  
                          yylval.num = atof(yytext);
                          return NUM;
                       }
{larraybracket}        { return LBRACKET;                }
{rarraybracket}        { return RBRACKET;                }

Include                { BEGIN(INCLUDEMODE);             }
<INCLUDEMODE>[ \t]* /* eat whitespace */
<INCLUDEMODE>\"[^\"\n]*\" {
            if ( include_stack_ptr >= MAX_INCLUDE_DEPTH )
            {
              fprintf(stderr, "Includes nested too deeply\n");
              exit(1);
            }
            include_stack[include_stack_ptr++] =
                YY_CURRENT_BUFFER;
            strcpy(yylval.string, &yytext[1]);
            yylval.string[yyleng-2] = '\0';
            yyin = fopen(yylval.string, "r");

            if ( !yyin )
            {
              fprintf(stderr, "Failed to open file %s\n", yylval.string);
              exit(-1);
            }

            yy_switch_to_buffer(
               yy_create_buffer(yyin, YY_BUF_SIZE) );

            BEGIN(INITIAL);
}

<<EOF>>   { 
          if ( --include_stack_ptr < 0 )
          {
             yyterminate();
          }
          else
          {
             yy_delete_buffer(YY_CURRENT_BUFFER);
             yy_switch_to_buffer(include_stack[include_stack_ptr]);
          }
}

 /* Interface commands */                        
Accel                  { return ACCEL;                   }
AreaLight              { return AREALIGHT;               }
AttributeBegin         { return ATTRIBUTEBEGIN;          }
AttributeEnd           { return ATTRIBUTEND;             }
Camera                 { return CAMERA;                  }
Clipping               { return CLIPPING;                }
ConcatTransform        { return CONCATTRANSFORM;         }
Displacement           { return DISPLACEMENT;            }
Environment            { return ENVIRONMENT;             }
FrameBegin             { return FRAMEBEGIN;              }
FrameEnd               { return FRAMEEND;                }
Identity               { return IDENTITY;                }
Image                  { return IMAGE;                   }
Instance               { return INSTANCE;                }
Light                  { return LIGHT;                   }
LightShader            { return LIGHTSHADER;             }
LookAt                 { return LOOKAT;                  }
MaxDepth               { return MAXDEPTH;                }
ObjectBegin            { return OBJECTBEGIN;             }
ObjectEnd              { return OBJECTEND;               }
PixelSamples           { return PIXELSAMPLES;            }
Renderer               { return RENDERER;                }
Rotate                 { return ROTATE;                  }
Sampler                { return SAMPLER;                 }
Scale                  { return SCALE;                   }
Spectral               { return SPECTRAL;                }
Surface                { return SURFACE;                 }
SurfaceShader          { return SURFACESHADER;           }
Texture                { return TEXTURE;                 }
ToneMap                { return TONEMAP;                 }
Transform              { return TRANSFORM;               }
TransformBegin         { return TRANSFORMBEGIN;          }
TransformEnd           { return TRANSFORMEND;            }
Translate              { return TRANSLATE;               }
WorldBegin             { return WORLDBEGIN;              }
WorldEnd               { return WORLDEND;                }


{ident}                {  
                          strcpy(yylval.string, yytext);
                          return UNKNOWN;                       
                       }

%%
int yywrap(void)
{
    return 1;
}               

