
#include <Core/Geometry/Vector.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Noise.h>

#ifdef MANTA_SSE
#  include <Core/Math/SSEDefs.h>
#endif

using namespace std;

namespace Manta {

  /**
   * This is the table of permutations required by the noise functions to
   * hash their input to generate the X coordinates.
   */
  static int NoiseXPermutationTable[] = {
    76, 97, 84, 215, 37, 4, 251, 48, 100, 174, 161, 125, 136, 99, 139, 19,
    101, 158, 202, 165, 138, 129, 144, 180, 232, 196, 155, 252, 182, 173,
    55, 201, 189, 14, 225, 31, 171, 46, 79, 146, 217, 127, 41, 92, 191, 82,
    183, 87, 65, 212, 108, 43, 24, 27, 111, 206, 9, 132, 13, 224, 77, 102,
    120, 194, 1, 107, 60, 38, 88, 90, 121, 163, 3, 137, 66, 181, 184, 227,
    122, 152, 233, 71, 59, 104, 133, 142, 160, 75, 236, 177, 78, 179, 12,
    74, 135, 126, 213, 51, 115, 243, 188, 164, 230, 114, 223, 208, 209, 68,
    130, 176, 190, 148, 53, 110, 96, 169, 105, 166, 150, 231, 131, 36, 241,
    62, 30, 56, 255, 221, 6, 123, 207, 72, 245, 134, 15, 54, 145, 193, 5,
    254, 98, 204, 175, 57, 205, 222, 186, 109, 229, 17, 45, 119, 117, 151,
    29, 49, 32, 22, 198, 140, 94, 253, 154, 195, 10, 159, 50, 23, 8, 86,
    95, 187, 25, 242, 214, 7, 203, 143, 147, 67, 64, 81, 73, 16, 21, 116,
    167, 244, 34, 200, 250, 44, 239, 33, 170, 113, 185, 85, 18, 40, 91,
    168, 89, 235, 238, 156, 172, 63, 249, 197, 35, 0, 28, 234, 237, 2, 210,
    247, 178, 103, 52, 141, 228, 70, 218, 112, 226, 248, 106, 157, 20, 83,
    118, 240, 128, 11, 211, 153, 42, 149, 216, 93, 69, 61, 220, 47, 199,
    219, 124, 80, 26, 58, 162, 39, 246, 192
  };

  /**
   * This is the table of permutations required by the noise functions to
   * hash their input to generate the Y coordinates.
   */
  static int NoiseYPermutationTable[] = {
    105, 76, 135, 3, 66, 169, 252, 41, 40, 69, 26, 141, 91, 88, 75, 191,
    118, 184, 126, 110, 172, 64, 185, 142, 157, 85, 39, 113, 56, 192, 70,
    111, 170, 87, 177, 100, 186, 216, 127, 94, 136, 182, 200, 37, 241, 251,
    128, 9, 161, 237, 32, 219, 44, 211, 20, 217, 35, 245, 42, 68, 218, 130,
    73, 90, 209, 228, 249, 7, 61, 102, 109, 17, 92, 116, 201, 208, 171,
    238, 96, 2, 62, 4, 223, 206, 106, 19, 86, 99, 214, 46, 38, 52, 83, 240,
    222, 150, 89, 204, 47, 231, 79, 114, 11, 49, 188, 193, 159, 72, 227,
    81, 235, 108, 25, 156, 230, 120, 80, 27, 225, 115, 220, 158, 13, 244,
    121, 55, 97, 5, 246, 67, 254, 233, 23, 194, 181, 146, 21, 236, 34, 212,
    104, 199, 122, 95, 163, 18, 15, 14, 167, 178, 143, 36, 58, 139, 229,
    205, 74, 48, 190, 60, 215, 93, 234, 162, 71, 82, 78, 232, 33, 103, 239,
    28, 63, 138, 189, 203, 210, 147, 57, 180, 207, 165, 145, 137, 198, 253,
    195, 45, 166, 77, 12, 134, 123, 221, 154, 129, 22, 50, 0, 152, 101,
    197, 226, 98, 247, 124, 117, 65, 10, 1, 107, 243, 16, 168, 54, 196,
    202, 183, 30, 133, 112, 125, 179, 31, 248, 176, 213, 140, 84, 24, 164,
    132, 51, 131, 255, 144, 8, 224, 173, 153, 242, 160, 29, 43, 149, 6,
    155, 148, 175, 53, 151, 250, 174, 59, 187, 119
  };

  /**
   * This is the table of permutations required by the noise functions to
   * hash their input to generate the Z coordinates.
   */
  static int NoiseZPermutationTable[] = {
    228, 169, 108, 236, 15, 208, 250, 147, 72, 76, 155, 10, 186, 3, 187,
    87, 1, 37, 19, 239, 123, 235, 70, 181, 122, 30, 125, 175, 221, 165,
    167, 231, 58, 211, 154, 38, 191, 130, 64, 206, 197, 114, 54, 40, 220,
    131, 41, 32, 53, 110, 172, 245, 219, 188, 95, 140, 44, 31, 255, 51,
    173, 73, 63, 229, 56, 49, 109, 118, 253, 115, 232, 61, 119, 42, 170,
    86, 192, 159, 79, 196, 80, 152, 157, 105, 23, 82, 20, 241, 156, 26, 36,
    2, 195, 94, 252, 142, 139, 24, 204, 17, 133, 178, 129, 74, 200, 12,
    151, 135, 153, 182, 60, 226, 185, 100, 161, 84, 222, 207, 213, 55, 102,
    88, 99, 134, 66, 121, 242, 216, 21, 0, 13, 28, 77, 168, 128, 83, 210,
    91, 127, 244, 46, 85, 52, 25, 138, 247, 163, 9, 81, 33, 96, 11, 223,
    16, 107, 35, 7, 237, 117, 97, 214, 143, 120, 174, 199, 194, 183, 227,
    57, 201, 27, 198, 146, 65, 71, 162, 164, 4, 112, 62, 45, 248, 106, 43,
    124, 217, 90, 101, 177, 18, 116, 225, 215, 69, 243, 238, 68, 67, 126,
    78, 158, 104, 176, 212, 203, 137, 59, 249, 34, 209, 180, 132, 141, 224,
    50, 22, 29, 75, 39, 47, 251, 144, 5, 240, 89, 160, 166, 254, 171, 8,
    93, 246, 179, 148, 230, 234, 233, 193, 103, 189, 14, 190, 92, 149, 184,
    202, 205, 111, 98, 218, 145, 48, 136, 113, 150, 6
  };

  /**
   * A table of 256 random values, evenly distributed between -1.0 and 1.0.
   * The values in the noise permutation tables can be used to index into
   * this table to get the gradients for the noise.
   */
  static Real Noise1DValueTable[] = {
    0.47451, 0.482353, 0.670588, 0.545098, 0.803922, -0.443137, -0.678431,
    0.32549, 0.45098, 0.396078, 0.905882, -0.0666667, -0.466667, 0.733333,
    0.87451, 0.568627, 0.921569, -0.584314, 0.262745, -0.670588, 0.254902,
    0.0745098, 0.647059, 0.654902, 0.858824, 0.00392157, 0.796078,
    0.333333, -0.835294, 0.498039, 0.0431373, -0.960784, 0.0117647,
    -0.286275, -0.858824, -0.717647, 0.207843, -0.639216, 0.458824,
    0.121569, 0.835294, 0.278431, -0.545098, -0.819608, 0.341176, 0.85098,
    -0.262745, -0.568627, -0.890196, 0.160784, 0.592157, 0.223529,
    -0.662745, 0.490196, 0.72549, 0.537255, -0.403922, -0.709804,
    -0.701961, -0.513726, -0.984314, 0.662745, 0.364706, -0.239216,
    -0.380392, 0.247059, -0.278431, -0.372549, -0.898039, 0.231373,
    0.992157, 0.937255, -0.647059, 0.215686, -0.34902, 0.0980392, 0.694118,
    -0.552941, -0.694118, -0.364706, -0.129412, 0.709804, 0.882353,
    -0.537255, -0.576471, 0.717647, 0.435294, 0.356863, -0.623529,
    -0.0431373, 1.0, -0.85098, -0.168627, -0.654902, -0.215686, -1.0,
    -0.411765, -0.396078, 0.443137, 0.607843, -0.027451, 0.0666667,
    0.380392, -0.0745098, 0.984314, -0.0509804, 0.929412, -0.811765,
    -0.764706, 0.137255, 0.239216, -0.32549, -0.113725, 0.411765, 0.576471,
    -0.160784, -0.803922, 0.301961, -0.796078, -0.0117647, 0.811765, 0.6,
    -0.921569, -0.6, 0.952941, -0.780392, 0.756863, -0.0823529, -0.968627,
    0.309804, 0.521569, -0.521569, 0.819608, 0.827451, -0.145098, 0.960784,
    0.168627, 0.372549, -0.992157, -0.0901961, 0.976471, -0.231373,
    -0.47451, -0.458824, -0.388235, 0.427451, 0.513726, -0.0196078,
    -0.74902, -0.270588, -0.137255, 0.945098, -0.843137, 0.184314,
    -0.905882, -0.301961, 0.615686, -0.333333, -0.482353, 0.466667,
    0.788235, -0.254902, 0.772549, -0.772549, 0.145098, 0.403922, 0.286275,
    -0.223529, -0.247059, 0.0901961, -0.741176, 0.388235, -0.0588235,
    -0.615686, -0.937255, 0.0588235, -0.631373, 0.270588, 0.560784,
    0.898039, 0.113725, -0.435294, 0.631373, 0.686275, -0.882353,
    -0.341176, 0.505882, -0.294118, -0.756863, 0.529412, -0.121569,
    0.027451, 0.584314, -0.317647, -0.866667, 0.0196078, 0.317647,
    -0.309804, -0.105882, -0.00392157, -0.945098, 0.152941, 0.678431,
    0.552941, 0.741176, -0.592157, -0.0980392, -0.152941, -0.827451,
    0.419608, 0.192157, -0.176471, 0.639216, -0.2, -0.952941, 0.0352941,
    0.890196, 0.780392, -0.929412, -0.490196, 0.866667, -0.788235,
    0.764706, -0.419608, 0.34902, 0.623529, -0.87451, -0.976471, 0.0509804,
    -0.184314, -0.427451, -0.45098, -0.607843, 0.129412, 0.294118,
    -0.192157, -0.686275, -0.356863, 0.701961, 0.74902, 0.913725, 0.843137,
    -0.560784, 0.968627, -0.72549, 0.0823529, 0.2, 0.176471, -0.505882,
    -0.207843, -0.733333, -0.913725, -0.498039, 0.105882, -0.0352941,
    -0.529412
  };

  /**
   * A table of 256 random vectors, evenly distributed over the unit sphere
   * (precalculated using point repulsion).  The values in the noise
   * permutation tables can be used to index into this table to get the
   * gradients for the noise.
   */
  static Real Noise3DValueTable[][ 3 ] = {
    { -0.439336, 0.540416, 0.71759 }, { 0.183348, 0.923971, 0.33565 },
    { -0.15203, -0.392751, -0.906991 }, { 0.707649, 0.112768, 0.697507 },
    { 0.136611, -0.456018, 0.879423 }, { -0.414372, -0.868792, -0.271103 },
    { 0.226922, 0.95503, -0.190853 }, { -0.503268, -0.567759, -0.651438 },
    { 0.202705, -0.917503, -0.342199 }, { 0.442574, -0.38187, -0.811359 },
    { -0.431245, 0.718579, 0.545594 }, { -0.45611, 0.123864, 0.881261 },
    { 0.731822, 0.601694, 0.320003 }, { 0.857833, -0.486358, -0.16607 },
    { -0.984773, -0.0700546, -0.159108 }, { 0.897567, -0.128187, -0.42183 },
    { 0.0719148, 0.226977, -0.971241 }, { 0.406964, 0.45094, -0.794377 },
    { 0.348909, -0.935977, 0.0469955 }, { -0.713226, -0.70007, -0.0347978 },
    { -0.682955, -0.626269, 0.375979 }, { -0.451844, 0.834955, -0.314146 },
    { 0.944641, -0.237783, -0.226082 }, { 0.347588, -0.360093, 0.865746 },
    { 0.322352, 0.271551, -0.906835 }, { -0.478177, 0.165448, -0.862539 },
    { 0.981479, 0.168541, -0.0910609 }, { 0.75633, -0.583618, 0.29556 },
    { 0.483394, -0.848784, 0.214235 }, { -0.258221, 0.469577, 0.844286 },
    { -0.98852, -0.12115, 0.0902812 }, { -0.815832, -0.23913, -0.526531 },
    { -0.26189, 0.248029, 0.932682 }, { -0.0667471, 0.587119, 0.806744 },
    { 0.710435, 0.484718, 0.510226 }, { 0.0537878, 0.997318, -0.0496396 },
    { -0.75667, -0.607357, -0.242007 }, { -0.752657, 0.181881, -0.632792 },
    { 0.387372, -0.820555, -0.420276 }, { -0.791318, -0.440329, -0.424177 },
    { 0.913304, 0.39413, -0.10265 }, { -0.301446, -0.891486, 0.338206 },
    { 0.714937, -0.692385, -0.0973061 }, { 0.854442, 0.40854, 0.320975 },
    { -0.603445, 0.558597, 0.569055 }, { 0.0846518, -0.641091, 0.762782 },
    { 0.798365, -0.410556, 0.440519 }, { -0.919178, -0.363545, 0.151479 },
    { 0.0297177, -0.266643, -0.963337 }, { -0.208139, -0.943941, -0.256229 },
    { -0.331726, 0.783602, -0.525286 }, { 0.252351, -0.93496, 0.249336 },
    { 0.662249, 0.335348, 0.670051 }, { 0.847415, -0.381714, -0.36903 },
    { -0.936956, 0.336445, -0.0944343 }, { 0.889042, -0.414835, 0.193691 },
    { 0.309093, 0.744696, -0.591514 }, { -0.340527, -0.264842, -0.902164 },
    { 0.258107, -0.748621, 0.610694 }, { -0.855984, -0.516825, -0.0135145 },
    { -0.265242, -0.842434, -0.46899 }, { 0.685572, -0.34165, 0.642858 },
    { -0.994972, 0.100007, -0.00536273 }, { -0.637258, 0.360475, 0.681146 },
    { -0.158408, -0.712155, 0.683917 }, { 0.940843, -0.338165, -0.0214161 },
    { 0.72989, -0.608883, -0.31068 }, { 0.822887, -0.565893, 0.0512029 },
    { 0.49129, -0.569896, -0.658674 }, { -0.676367, -0.27648, 0.682706 },
    { 0.24284, 0.60447, -0.758712 }, { -0.643739, 0.14698, 0.750997 },
    { -0.083957, 0.375356, -0.92307 }, { -0.412972, -0.0922911, 0.906056 },
    { 0.327515, -0.718158, -0.61399 }, { -0.231072, 0.520139, -0.82223 },
    { 0.142999, -0.889635, 0.433706 }, { -0.23903, 0.83204, 0.500573 },
    { -0.781023, 0.368517, 0.504181 }, { 0.784083, -0.266837, -0.560368 },
    { 0.669466, 0.554173, -0.494679 }, { 0.480597, 0.597933, -0.641485 },
    { -0.170488, -0.140845, -0.975242 }, { 0.673037, 0.67461, -0.303189 },
    { 0.640313, -0.177876, -0.747234 }, { 0.453229, -0.168765, -0.875272 },
    { 0.164875, 0.820621, 0.547172 }, { 0.858158, 0.501686, 0.108977 },
    { -0.326214, -0.943844, -0.0523639 }, { -0.515782, -0.351575, -0.781258 },
    { 0.360909, -0.12727, 0.923876 }, { 0.818096, 0.485562, -0.308137 },
    { -0.608003, -0.0753869, 0.790347 }, { 0.224449, 0.970822, 0.0844204 },
    { 0.354363, 0.346855, 0.8684 }, { -0.606054, 0.309139, -0.732893 },
    { -0.0271944, 0.9025, 0.42983 }, { 0.699927, -0.111535, 0.705452 },
    { -0.0919001, -0.772969, -0.627753 }, { -0.80478, -0.0297094, -0.592829 },
    { 0.48361, 0.840043, -0.245864 }, { -0.111169, -0.543304, 0.832143 },
    { -0.953653, 0.270821, 0.131156 }, { -0.754255, 0.387802, -0.52982 },
    { -0.394301, 0.623026, -0.675548 }, { -0.869612, 0.380197, -0.315002 },
    { 0.960692, 0.0290017, -0.276097 }, { 0.63449, 0.766558, -0.0990497 },
    { -0.240385, -0.228125, 0.94349 }, { 0.722172, 0.682823, 0.110546 },
    { 0.377269, -0.82387, 0.422973 }, { -0.0542081, -0.348111, 0.935885 },
    { -0.138573, 0.125657, -0.982348 }, { -0.827473, -0.248018, 0.503761 },
    { -0.201112, -0.967555, 0.152939 }, { 0.73238, 0.200635, -0.650666 },
    { 0.38434, -0.903242, -0.190883 }, { -0.57927, 0.801379, 0.149125 },
    { 0.994035, -0.0769966, -0.0772332 }, { -0.131061, -0.604388, -0.785836 },
    { -0.0386461, -0.110923, 0.993077 }, { -0.865407, 0.499223, 0.0429705 },
    { 0.012684, 0.980876, 0.194221 }, { 0.548351, 0.830514, 0.0977628 },
    { -0.482562, -0.28967, 0.826574 }, { 0.0657665, 0.73372, -0.676262 },
    { -0.833706, -0.424801, 0.352816 }, { -0.336057, -0.495912, -0.80071 },
    { 0.281337, -0.558986, -0.77999 }, { -0.0544871, 0.773031, 0.632023 },
    { -0.309977, -0.690948, -0.653074 }, { -0.341723, -0.768081, 0.541551 },
    { 0.796575, 0.59552, -0.104038 }, { 0.910477, -0.244838, 0.333295 },
    { 0.927058, 0.19256, 0.32169 }, { 0.7879, -0.0370806, -0.614686 },
    { 0.399428, 0.915321, -0.0514189 }, { 0.54222, -0.00273783, 0.840232 },
    { -0.43104, 0.421376, -0.797902 }, { -0.90244, 0.216822, 0.372277 },
    { -0.644789, 0.720023, -0.256543 }, { 0.162674, 0.00379719, 0.986673 },
    { 0.4398, 0.0528767, -0.896538 }, { -0.770264, -0.0601248, 0.634884 },
    { 0.976798, -0.17239, 0.127076 }, { 0.623922, -0.386115, -0.679439 },
    { -0.63839, 0.0388753, -0.768731 }, { -0.532632, -0.468541, 0.704821 },
    { 0.530223, 0.232911, 0.81524 }, { 0.53781, 0.25392, -0.803918 },
    { 0.229803, 0.0713721, -0.970617 }, { 0.124481, 0.466915, 0.875497 },
    { -0.090886, -0.995224, -0.0356067 }, { -0.644112, -0.608224, -0.463878 },
    { -0.718542, 0.694649, -0.0340805 }, { 0.816596, 0.266484, 0.512012 },
    { -0.958129, -0.279709, -0.0612457 }, { -0.460416, 0.337646, 0.820982 },
    { 0.536461, -0.243775, 0.80795 }, { -0.704418, -0.460194, 0.540386 },
    { -0.46701, -0.74384, -0.478126 }, { 0.546414, -0.837123, -0.0256313 },
    { 0.159448, -0.234379, 0.95898 }, { 0.826179, -0.181784, 0.533276 },
    { -0.584846, 0.722397, 0.368915 }, { 0.846608, 0.0427948, 0.530494 },
    { -0.815608, 0.54987, -0.180074 }, { 0.0259781, -0.0307692, -0.999189 },
    { 0.0558926, -0.49629, -0.866356 }, { 0.31474, 0.542291, 0.779012 },
    { -0.672587, -0.190602, -0.715051 }, { 0.515298, 0.491834, 0.701831 },
    { -0.229595, 0.0242324, 0.972984 }, { 0.127668, 0.857768, -0.49793 },
    { -0.960484, 0.156199, -0.230375 }, { -0.866444, 0.4235, 0.264426 },
    { 0.942263, -0.0269192, 0.333791 }, { 0.629016, 0.0399611, -0.776365 },
    { 0.464898, -0.664106, 0.58552 }, { 0.872592, 0.120409, -0.473375 },
    { -0.126973, 0.97659, -0.173639 }, { 0.511703, -0.468186, 0.72039 },
    { 0.556297, 0.645701, 0.523072 }, { -0.540185, 0.691882, -0.47906 },
    { -0.49859, -0.111542, -0.859632 }, { 0.160874, 0.248233, 0.955249 },
    { 0.0809376, -0.990302, 0.11292 }, { -0.735101, 0.555227, 0.389037 },
    { -0.721784, 0.564579, -0.400347 }, { 0.381889, 0.8967, 0.22381 },
    { -0.0352034, 0.137595, 0.989863 }, { -0.331783, 0.0141144, -0.94325 },
    { -0.971054, 0.0501974, 0.233524 }, { -0.629321, -0.758668, 0.168458 },
    { -0.919631, -0.0231463, -0.3921 }, { -0.875718, 0.191596, -0.443181 },
    { 0.0966677, -0.682085, -0.724856 }, { 0.70231, -0.50127, -0.50546 },
    { -0.89521, -0.0139915, 0.445425 }, { -0.796322, 0.160194, 0.583275 },
    { 0.0140862, -0.972376, -0.232996 }, { 0.379515, 0.81459, 0.438648 },
    { -0.203231, 0.937804, 0.281462 }, { -0.349885, -0.602319, 0.71749 },
    { 0.130501, 0.673936, 0.727172 }, { -0.880631, -0.422363, -0.214705 },
    { -0.302827, -0.426887, 0.852094 }, { -0.232555, 0.905781, -0.354231 },
    { -0.0301316, -0.953296, 0.300529 }, { 0.915627, 0.26774, -0.299903 },
    { 0.670158, -0.728537, 0.141851 }, { 0.312295, 0.870793, -0.379726 },
    { 0.35796, 0.694163, 0.624502 }, { 0.161781, 0.430248, -0.888095 },
    { -0.386391, 0.916122, 0.106878 }, { 0.243076, -0.148859, -0.958517 },
    { -0.668864, -0.410977, -0.61945 }, { 0.795898, 0.351646, -0.49284 },
    { -0.599629, -0.753855, -0.268604 }, { -0.0642878, 0.362654, 0.929704 },
    { 0.623968, 0.410496, -0.664949 }, { 0.584825, -0.718079, 0.377282 },
    { -0.173356, 0.983369, 0.0541666 }, { 0.144749, -0.824954, -0.546351 },
    { 0.647336, -0.549778, 0.52792 }, { -0.932486, -0.19334, 0.305104 },
    { -0.745692, 0.640997, 0.18184 }, { 0.0362768, -0.795103, 0.605389 },
    { 0.312114, -0.572358, 0.758282 }, { -0.13296, -0.866316, 0.481476 },
    { -0.0425183, -0.89691, -0.440165 }, { -0.0974423, 0.847846, -0.521212 },
    { -0.00241325, 0.573725, -0.819044 }, { 0.0351067, 0.946666, -0.320297 },
    { -0.406575, 0.851314, 0.331604 }, { -0.251984, 0.674039, 0.694389 },
    { -0.919299, -0.23074, -0.318824 }, { 0.355339, 0.111842, 0.928023 },
    { 0.504158, 0.734106, -0.454877 }, { -0.344281, 0.930389, -0.125885 },
    { -0.533668, -0.640121, 0.552669 }, { 0.990907, 0.0632633, 0.118745 },
    { -0.169873, 0.703965, -0.68962 }, { -0.436359, -0.887434, 0.148501 },
    { -0.541072, 0.837283, -0.0787307 }, { -0.789581, -0.585243, 0.184535 },
    { -0.536856, -0.841774, -0.0565978 }, { -0.29148, 0.279713, -0.914768 },
    { -0.589255, 0.517219, -0.620696 }, { 0.570477, 0.756654, 0.319423 },
    { 0.181417, -0.97924, -0.090425 }, { -0.505724, -0.782838, 0.362502 },
    { 0.949439, 0.293359, 0.111829 }, { 0.568102, -0.780605, -0.260608 },
    { 0.238053, -0.36183, -0.901338 }, { 0.5581, -0.681458, -0.473433 }
  };

  Real ScalarNoise( Vector const& location )
  {
    int integer_of_x = Floor( location.x() );
    int integer_of_y = Floor( location.y() );
    int integer_of_z = Floor( location.z() );
    Real offset_of_x = location.x() - integer_of_x;
    Real offset_of_y = location.y() - integer_of_y;
    Real offset_of_z = location.z() - integer_of_z;
    Real fade_x = offset_of_x * offset_of_x * offset_of_x * ( offset_of_x * ( offset_of_x * 6 - 15 ) + 10 );
    Real fade_y = offset_of_y * offset_of_y * offset_of_y * ( offset_of_y * ( offset_of_y * 6 - 15 ) + 10 );
    Real fade_z = offset_of_z * offset_of_z * offset_of_z * ( offset_of_z * ( offset_of_z * 6 - 15 ) + 10 );
    int hash_0 = NoiseXPermutationTable[ integer_of_x & 0xFF ];
    int hash_00 = NoiseXPermutationTable[ ( hash_0 + integer_of_y ) & 0xFF ];
    int hash_000 = NoiseXPermutationTable[ ( hash_00 + integer_of_z ) & 0xFF ];
    int hash_001 = NoiseXPermutationTable[ ( hash_00 + integer_of_z + 1 ) & 0xFF ];
    Real value_00 = Interpolate( ( Noise3DValueTable[ hash_000 ][ 0 ] * offset_of_x +
                                   Noise3DValueTable[ hash_000 ][ 1 ] * offset_of_y +
                                   Noise3DValueTable[ hash_000 ][ 2 ] * offset_of_z ),
                                 ( Noise3DValueTable[ hash_001 ][ 0 ] * offset_of_x +
                                   Noise3DValueTable[ hash_001 ][ 1 ] * offset_of_y +
                                   Noise3DValueTable[ hash_001 ][ 2 ] * ( offset_of_z - 1 ) ),
                                 fade_z );
    int hash_01 = NoiseXPermutationTable[ ( hash_0 + integer_of_y + 1 ) & 0xFF ];
    int hash_010 = NoiseXPermutationTable[ ( hash_01 + integer_of_z ) & 0xFF ];
    int hash_011 = NoiseXPermutationTable[ ( hash_01 + integer_of_z + 1 ) & 0xFF ];
    Real value_01 = Interpolate( ( Noise3DValueTable[ hash_010 ][ 0 ] * offset_of_x +
                                   Noise3DValueTable[ hash_010 ][ 1 ] * ( offset_of_y - 1 ) +
                                   Noise3DValueTable[ hash_010 ][ 2 ] * offset_of_z ),
                                 ( Noise3DValueTable[ hash_011 ][ 0 ] * offset_of_x +
                                   Noise3DValueTable[ hash_011 ][ 1 ] * ( offset_of_y - 1 ) +
                                   Noise3DValueTable[ hash_011 ][ 2 ] * ( offset_of_z - 1 ) ),
                                 fade_z );
    Real value_0 = Interpolate( value_00, value_01, fade_y );
    int hash_1 = NoiseXPermutationTable[ ( integer_of_x + 1 ) & 0xFF ];
    int hash_10 = NoiseXPermutationTable[ ( hash_1 + integer_of_y ) & 0xFF ];
    int hash_100 = NoiseXPermutationTable[ ( hash_10 + integer_of_z ) & 0xFF ];
    int hash_101 = NoiseXPermutationTable[ ( hash_10 + integer_of_z + 1 ) & 0xFF ];
    Real value_10 = Interpolate( ( Noise3DValueTable[ hash_100 ][ 0 ] * ( offset_of_x - 1 ) +
                                   Noise3DValueTable[ hash_100 ][ 1 ] * offset_of_y +
                                   Noise3DValueTable[ hash_100 ][ 2 ] * offset_of_z ),
                                 ( Noise3DValueTable[ hash_101 ][ 0 ] * ( offset_of_x - 1 ) +
                                   Noise3DValueTable[ hash_101 ][ 1 ] * offset_of_y +
                                   Noise3DValueTable[ hash_101 ][ 2 ] * ( offset_of_z - 1 ) ),
                                 fade_z );
    int hash_11 = NoiseXPermutationTable[ ( hash_1 + integer_of_y + 1 ) & 0xFF ];
    int hash_110 = NoiseXPermutationTable[ ( hash_11 + integer_of_z ) & 0xFF ];
    int hash_111 = NoiseXPermutationTable[ ( hash_11 + integer_of_z + 1 ) & 0xFF ];
    Real value_11 = Interpolate( ( Noise3DValueTable[ hash_110 ][ 0 ] * ( offset_of_x - 1 ) +
                                   Noise3DValueTable[ hash_110 ][ 1 ] * ( offset_of_y - 1 ) +
                                   Noise3DValueTable[ hash_110 ][ 2 ] * offset_of_z ),
                                 ( Noise3DValueTable[ hash_111 ][ 0 ] * ( offset_of_x - 1 ) +
                                   Noise3DValueTable[ hash_111 ][ 1 ] * ( offset_of_y - 1 ) +
                                   Noise3DValueTable[ hash_111 ][ 2 ] * ( offset_of_z - 1 ) ),
                                 fade_z );
    Real value_1 = Interpolate( value_10, value_11, fade_y );
    return Interpolate( value_0, value_1, fade_x );
  }

#if MANTA_SSE
  static inline __m128 grad(const __m128i& hash,
                            const __m128 & x,
                            const __m128 & y,
                            const __m128 & z)
  {
    // CONVERT LO 4 BITS OF HASH CODE
    // int h = hash & 15;
    __m128i h    = _mm_and_si128(hash, _mm_set1_epi32(15));
    // INTO 12 GRADIENT DIRECTIONS.
    // double u = h<8 ? x : y;
    __m128  u    = mask4(_mm_castsi128_ps(_mm_cmplt_epi32(h, _mm_set1_epi32(8))),
                         x,
                         y);
    // double v = h<4 ? y : h==12||h==14 ? x : z;
    __m128  v    = mask4(_mm_castsi128_ps(_mm_cmplt_epi32(h, _mm_set1_epi32(4))),
                         y,
                         mask4(_mm_castsi128_ps(_mm_or_si128(_mm_cmpeq_epi32(h, _mm_set1_epi32(12)),
                                                             _mm_cmpeq_epi32(h, _mm_set1_epi32(14)))),
                               x,
                               z));
    // return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
    // return ((h&1) == 1 ? -u : u) + ((h&2) == 2 ? -v : v);

    // To do the unary negation we will xor 0x80000000 with the sign
    // bit.  xor'ing with zero will leave the result unchanged, so if
    // we 'and' the comparison mask with 0x80000000 we will be able to
    // selectively change the sign bit.
    __m128 uPart = _mm_xor_ps(_mm_castsi128_ps(_mm_and_si128(_mm_cmpeq_epi32(_mm_and_si128(h, _mm_set1_epi32(1)),
                                                                             _mm_set1_epi32(1)),
                                                             _mm_set1_epi32(0x80000000))),
                              u);
    __m128 vPart = _mm_xor_ps(_mm_castsi128_ps(_mm_and_si128(_mm_cmpeq_epi32(_mm_and_si128(h, _mm_set1_epi32(2)),
                                                                             _mm_set1_epi32(2)),
                                                             _mm_set1_epi32(0x80000000))),
                              v);
    return _mm_add_ps(uPart, vPart);
  }

  static inline __m128 Interpolate(const __m128& d1, const __m128& d2, const __m128& weight)
  {
    return _mm_add_ps(d1,
                      _mm_mul_ps(weight,
                                 _mm_sub_ps(d2, d1)));
  }

  typedef union {
      unsigned int i[4];
      __m128i      s;
  } unsigned_int_ssei;
  
  __m128 ScalarNoiseSSE( const __m128& location_x,
                         const __m128& location_y,
                         const __m128& location_z)
  {
    // x
    __m128  offset_of_x  = fracSSE(location_x);
    unsigned_int_ssei integer_of_x;
    integer_of_x.s = _mm_and_si128(_mm_cvttps_epi32(_mm_sub_ps(location_x, offset_of_x)), _mm_set1_epi32(0xFF));
    __m128  fade_x       = _mm_mul_ps(_mm_mul_ps(offset_of_x, offset_of_x),
                                      _mm_mul_ps(offset_of_x,
                                                 _mm_add_ps(_mm_mul_ps(offset_of_x,
                                                                       _mm_sub_ps(_mm_mul_ps(offset_of_x,
                                                                                             _mm_set_ps1(6.f)),
                                                                                  _mm_set_ps1(15.f))),
                                                            _mm_set_ps1(10.f))));

    // y
    __m128  offset_of_y  = fracSSE(location_y);
    unsigned_int_ssei integer_of_y;
    integer_of_y.s = _mm_cvttps_epi32(_mm_sub_ps(location_y, offset_of_y));
    __m128  fade_y       = _mm_mul_ps(_mm_mul_ps(offset_of_y, offset_of_y), _mm_mul_ps(offset_of_y, _mm_add_ps(_mm_mul_ps(offset_of_y, _mm_sub_ps(_mm_mul_ps(offset_of_y, _mm_set_ps1(6.f)), _mm_set_ps1(15.f))), _mm_set_ps1(10.f))));

    // z
    __m128  offset_of_z  = fracSSE(location_z);
    unsigned_int_ssei integer_of_z;
    integer_of_z.s = _mm_cvttps_epi32(_mm_sub_ps(location_z, offset_of_z));
    __m128  fade_z       = _mm_mul_ps(_mm_mul_ps(offset_of_z, offset_of_z), _mm_mul_ps(offset_of_z, _mm_add_ps(_mm_mul_ps(offset_of_z, _mm_sub_ps(_mm_mul_ps(offset_of_z, _mm_set_ps1(6.f)), _mm_set_ps1(15.f))), _mm_set_ps1(10.f))));

    unsigned_int_ssei hash_000, hash_001, hash_010, hash_011, hash_100, hash_101, hash_110, hash_111;
    for(unsigned int i = 0; i < 4; ++i) {
      unsigned int x = integer_of_x.i[i];
      unsigned int y = integer_of_y.i[i];
      unsigned int z = integer_of_z.i[i];
      unsigned int hash_0  = NoiseXPermutationTable[(x)                   ];
      unsigned int hash_1  = NoiseXPermutationTable[(x+1)           & 0xFF];
      unsigned int hash_00 = NoiseXPermutationTable[(hash_0 + y)    & 0xFF];
      unsigned int hash_01 = NoiseXPermutationTable[(hash_0 + y+1)  & 0xFF];
      unsigned int hash_10 = NoiseXPermutationTable[(hash_1 + y)    & 0xFF];
      unsigned int hash_11 = NoiseXPermutationTable[(hash_1 + y+1)  & 0xFF];
      hash_000.i[i]        = NoiseXPermutationTable[(hash_00 + z)   & 0xFF];
      hash_001.i[i]        = NoiseXPermutationTable[(hash_00 + z+1) & 0xFF];
      hash_010.i[i]        = NoiseXPermutationTable[(hash_01 + z)   & 0xFF];
      hash_011.i[i]        = NoiseXPermutationTable[(hash_01 + z+1) & 0xFF];
      hash_100.i[i]        = NoiseXPermutationTable[(hash_10 + z)   & 0xFF];
      hash_101.i[i]        = NoiseXPermutationTable[(hash_10 + z+1) & 0xFF];
      hash_110.i[i]        = NoiseXPermutationTable[(hash_11 + z)   & 0xFF];
      hash_111.i[i]        = NoiseXPermutationTable[(hash_11 + z+1) & 0xFF];
    }

    __m128  value_000    = grad(hash_000.s, offset_of_x, offset_of_y, offset_of_z);
    __m128  value_001    = grad(hash_001.s, offset_of_x, offset_of_y, _mm_sub_ps(offset_of_z, _mm_set_ps1(1)));
    __m128  value_010    = grad(hash_010.s, offset_of_x, _mm_sub_ps(offset_of_y, _mm_set_ps1(1)), offset_of_z);
    __m128  value_011    = grad(hash_011.s, offset_of_x, _mm_sub_ps(offset_of_y, _mm_set_ps1(1)), _mm_sub_ps(offset_of_z, _mm_set_ps1(1)));
    __m128  value_100    = grad(hash_100.s, _mm_sub_ps(offset_of_x, _mm_set_ps1(1)), offset_of_y, offset_of_z);
    __m128  value_101    = grad(hash_101.s, _mm_sub_ps(offset_of_x, _mm_set_ps1(1)), offset_of_y, _mm_sub_ps(offset_of_z, _mm_set_ps1(1)));
    __m128  value_110    = grad(hash_110.s, _mm_sub_ps(offset_of_x, _mm_set_ps1(1)), _mm_sub_ps(offset_of_y, _mm_set_ps1(1)), offset_of_z);
    __m128  value_111    = grad(hash_111.s, _mm_sub_ps(offset_of_x, _mm_set_ps1(1)), _mm_sub_ps(offset_of_y, _mm_set_ps1(1)), _mm_sub_ps(offset_of_z, _mm_set_ps1(1)));

    //
    __m128  value_00     = Interpolate(value_000, value_001, fade_z);
    __m128  value_01     = Interpolate(value_010, value_011, fade_z);
    __m128  value_0      = Interpolate(value_00,  value_01,  fade_y);
    // 
    __m128  value_10     = Interpolate(value_100, value_101, fade_z);
    __m128  value_11     = Interpolate(value_110, value_111, fade_z);
    __m128  value_1      = Interpolate(value_10,  value_11,  fade_y);
    //
    __m128  value        = Interpolate(value_0,   value_1,   fade_x);
    
    return value;
  }
#endif
  
  Vector const VectorNoise( Vector const& location )
  {
    int integer_of_x = Floor( location.x() );
    int integer_of_y = Floor( location.y() );
    int integer_of_z = Floor( location.z() );
    Real offset_of_x = location.x() - integer_of_x;
    Real offset_of_y = location.y() - integer_of_y;
    Real offset_of_z = location.z() - integer_of_z;
    Real fade_x = offset_of_x * offset_of_x * offset_of_x * ( offset_of_x * ( offset_of_x * 6 - 15 ) + 10 );
    Real fade_y = offset_of_y * offset_of_y * offset_of_y * ( offset_of_y * ( offset_of_y * 6 - 15 ) + 10 );
    Real fade_z = offset_of_z * offset_of_z * offset_of_z * ( offset_of_z * ( offset_of_z * 6 - 15 ) + 10 );
    int hash_0 = NoiseXPermutationTable[ integer_of_x & 0xFF ];
    int hash_00 = NoiseXPermutationTable[ ( hash_0 + integer_of_y ) & 0xFF ];
    int hash_000 = NoiseXPermutationTable[ ( hash_00 + integer_of_z ) & 0xFF ];
    int hash_001 = NoiseXPermutationTable[ ( hash_00 + integer_of_z + 1 ) & 0xFF ];
    Real value_00 = Interpolate( ( Noise3DValueTable[ hash_000 ][ 0 ] * offset_of_x +
                                   Noise3DValueTable[ hash_000 ][ 1 ] * offset_of_y +
                                   Noise3DValueTable[ hash_000 ][ 2 ] * offset_of_z ),
                                 ( Noise3DValueTable[ hash_001 ][ 0 ] * offset_of_x +
                                   Noise3DValueTable[ hash_001 ][ 1 ] * offset_of_y +
                                   Noise3DValueTable[ hash_001 ][ 2 ] * ( offset_of_z - 1 ) ),
                                 fade_z );
    int hash_01 = NoiseXPermutationTable[ ( hash_0 + integer_of_y + 1 ) & 0xFF ];
    int hash_010 = NoiseXPermutationTable[ ( hash_01 + integer_of_z ) & 0xFF ];
    int hash_011 = NoiseXPermutationTable[ ( hash_01 + integer_of_z + 1 ) & 0xFF ];
    Real value_01 = Interpolate( ( Noise3DValueTable[ hash_010 ][ 0 ] * offset_of_x +
                                   Noise3DValueTable[ hash_010 ][ 1 ] * ( offset_of_y - 1 ) +
                                   Noise3DValueTable[ hash_010 ][ 2 ] * offset_of_z ),
                                 ( Noise3DValueTable[ hash_011 ][ 0 ] * offset_of_x +
                                   Noise3DValueTable[ hash_011 ][ 1 ] * ( offset_of_y - 1 ) +
                                   Noise3DValueTable[ hash_011 ][ 2 ] * ( offset_of_z - 1 ) ),
                                 fade_z );
    Real value_0 = Interpolate( value_00, value_01, fade_y );
    int hash_1 = NoiseXPermutationTable[ ( integer_of_x + 1 ) & 0xFF ];
    int hash_10 = NoiseXPermutationTable[ ( hash_1 + integer_of_y ) & 0xFF ];
    int hash_100 = NoiseXPermutationTable[ ( hash_10 + integer_of_z ) & 0xFF ];
    int hash_101 = NoiseXPermutationTable[ ( hash_10 + integer_of_z + 1 ) & 0xFF ];
    Real value_10 = Interpolate( ( Noise3DValueTable[ hash_100 ][ 0 ] * ( offset_of_x - 1 ) +
                                   Noise3DValueTable[ hash_100 ][ 1 ] * offset_of_y +
                                   Noise3DValueTable[ hash_100 ][ 2 ] * offset_of_z ),
                                 ( Noise3DValueTable[ hash_101 ][ 0 ] * ( offset_of_x - 1 ) +
                                   Noise3DValueTable[ hash_101 ][ 1 ] * offset_of_y +
                                   Noise3DValueTable[ hash_101 ][ 2 ] * ( offset_of_z - 1 ) ),
                                 fade_z );
    int hash_11 = NoiseXPermutationTable[ ( hash_1 + integer_of_y + 1 ) & 0xFF ];
    int hash_110 = NoiseXPermutationTable[ ( hash_11 + integer_of_z ) & 0xFF ];
    int hash_111 = NoiseXPermutationTable[ ( hash_11 + integer_of_z + 1 ) & 0xFF ];
    Real value_11 = Interpolate( ( Noise3DValueTable[ hash_110 ][ 0 ] * ( offset_of_x - 1 ) +
                                   Noise3DValueTable[ hash_110 ][ 1 ] * ( offset_of_y - 1 ) +
                                   Noise3DValueTable[ hash_110 ][ 2 ] * offset_of_z ),
                                 ( Noise3DValueTable[ hash_111 ][ 0 ] * ( offset_of_x - 1 ) +
                                   Noise3DValueTable[ hash_111 ][ 1 ] * ( offset_of_y - 1 ) +
                                   Noise3DValueTable[ hash_111 ][ 2 ] * ( offset_of_z - 1 ) ),
                                 fade_z );
    Real value_1 = Interpolate( value_10, value_11, fade_y );
    Real x_result = Interpolate( value_0, value_1, fade_x );
    hash_0 = NoiseYPermutationTable[ integer_of_x & 0xFF ];
    hash_00 = NoiseYPermutationTable[ ( hash_0 + integer_of_y ) & 0xFF ];
    hash_000 = NoiseYPermutationTable[ ( hash_00 + integer_of_z ) & 0xFF ];
    hash_001 = NoiseYPermutationTable[ ( hash_00 + integer_of_z + 1 ) & 0xFF ];
    value_00 = Interpolate( ( Noise3DValueTable[ hash_000 ][ 0 ] * offset_of_x +
                              Noise3DValueTable[ hash_000 ][ 1 ] * offset_of_y +
                              Noise3DValueTable[ hash_000 ][ 2 ] * offset_of_z ),
                            ( Noise3DValueTable[ hash_001 ][ 0 ] * offset_of_x +
                              Noise3DValueTable[ hash_001 ][ 1 ] * offset_of_y +
                              Noise3DValueTable[ hash_001 ][ 2 ] * ( offset_of_z - 1 ) ),
                            fade_z );
    hash_01 = NoiseYPermutationTable[ ( hash_0 + integer_of_y + 1 ) & 0xFF ];
    hash_010 = NoiseYPermutationTable[ ( hash_01 + integer_of_z ) & 0xFF ];
    hash_011 = NoiseYPermutationTable[ ( hash_01 + integer_of_z + 1 ) & 0xFF ];
    value_01 = Interpolate( ( Noise3DValueTable[ hash_010 ][ 0 ] * offset_of_x +
                              Noise3DValueTable[ hash_010 ][ 1 ] * ( offset_of_y - 1 ) +
                              Noise3DValueTable[ hash_010 ][ 2 ] * offset_of_z ),
                            ( Noise3DValueTable[ hash_011 ][ 0 ] * offset_of_x +
                              Noise3DValueTable[ hash_011 ][ 1 ] * ( offset_of_y - 1 ) +
                              Noise3DValueTable[ hash_011 ][ 2 ] * ( offset_of_z - 1 ) ),
                            fade_z );
    value_0 = Interpolate( value_00, value_01, fade_y );
    hash_1 = NoiseYPermutationTable[ ( integer_of_x + 1 ) & 0xFF ];
    hash_10 = NoiseYPermutationTable[ ( hash_1 + integer_of_y ) & 0xFF ];
    hash_100 = NoiseYPermutationTable[ ( hash_10 + integer_of_z ) & 0xFF ];
    hash_101 = NoiseYPermutationTable[ ( hash_10 + integer_of_z + 1 ) & 0xFF ];
    value_10 = Interpolate( ( Noise3DValueTable[ hash_100 ][ 0 ] * ( offset_of_x - 1 ) +
                              Noise3DValueTable[ hash_100 ][ 1 ] * offset_of_y +
                              Noise3DValueTable[ hash_100 ][ 2 ] * offset_of_z ),
                            ( Noise3DValueTable[ hash_101 ][ 0 ] * ( offset_of_x - 1 ) +
                              Noise3DValueTable[ hash_101 ][ 1 ] * offset_of_y +
                              Noise3DValueTable[ hash_101 ][ 2 ] * ( offset_of_z - 1 ) ),
                            fade_z );
    hash_11 = NoiseYPermutationTable[ ( hash_1 + integer_of_y + 1 ) & 0xFF ];
    hash_110 = NoiseYPermutationTable[ ( hash_11 + integer_of_z ) & 0xFF ];
    hash_111 = NoiseYPermutationTable[ ( hash_11 + integer_of_z + 1 ) & 0xFF ];
    value_11 = Interpolate( ( Noise3DValueTable[ hash_110 ][ 0 ] * ( offset_of_x - 1 ) +
                              Noise3DValueTable[ hash_110 ][ 1 ] * ( offset_of_y - 1 ) +
                              Noise3DValueTable[ hash_110 ][ 2 ] * offset_of_z ),
                            ( Noise3DValueTable[ hash_111 ][ 0 ] * ( offset_of_x - 1 ) +
                              Noise3DValueTable[ hash_111 ][ 1 ] * ( offset_of_y - 1 ) +
                              Noise3DValueTable[ hash_111 ][ 2 ] * ( offset_of_z - 1 ) ),
                            fade_z );
    value_1 = Interpolate( value_10, value_11, fade_y );
    Real y_result = Interpolate( value_0, value_1, fade_x );
    hash_0 = NoiseZPermutationTable[ integer_of_x & 0xFF ];
    hash_00 = NoiseZPermutationTable[ ( hash_0 + integer_of_y ) & 0xFF ];
    hash_000 = NoiseZPermutationTable[ ( hash_00 + integer_of_z ) & 0xFF ];
    hash_001 = NoiseZPermutationTable[ ( hash_00 + integer_of_z + 1 ) & 0xFF ];
    value_00 = Interpolate( ( Noise3DValueTable[ hash_000 ][ 0 ] * offset_of_x +
                              Noise3DValueTable[ hash_000 ][ 1 ] * offset_of_y +
                              Noise3DValueTable[ hash_000 ][ 2 ] * offset_of_z ),
                            ( Noise3DValueTable[ hash_001 ][ 0 ] * offset_of_x +
                              Noise3DValueTable[ hash_001 ][ 1 ] * offset_of_y +
                              Noise3DValueTable[ hash_001 ][ 2 ] * ( offset_of_z - 1 ) ),
                            fade_z );
    hash_01 = NoiseZPermutationTable[ ( hash_0 + integer_of_y + 1 ) & 0xFF ];
    hash_010 = NoiseZPermutationTable[ ( hash_01 + integer_of_z ) & 0xFF ];
    hash_011 = NoiseZPermutationTable[ ( hash_01 + integer_of_z + 1 ) & 0xFF ];
    value_01 = Interpolate( ( Noise3DValueTable[ hash_010 ][ 0 ] * offset_of_x +
                              Noise3DValueTable[ hash_010 ][ 1 ] * ( offset_of_y - 1 ) +
                              Noise3DValueTable[ hash_010 ][ 2 ] * offset_of_z ),
                            ( Noise3DValueTable[ hash_011 ][ 0 ] * offset_of_x +
                              Noise3DValueTable[ hash_011 ][ 1 ] * ( offset_of_y - 1 ) +
                              Noise3DValueTable[ hash_011 ][ 2 ] * ( offset_of_z - 1 ) ),
                            fade_z );
    value_0 = Interpolate( value_00, value_01, fade_y );
    hash_1 = NoiseZPermutationTable[ ( integer_of_x + 1 ) & 0xFF ];
    hash_10 = NoiseZPermutationTable[ ( hash_1 + integer_of_y ) & 0xFF ];
    hash_100 = NoiseZPermutationTable[ ( hash_10 + integer_of_z ) & 0xFF ];
    hash_101 = NoiseZPermutationTable[ ( hash_10 + integer_of_z + 1 ) & 0xFF ];
    value_10 = Interpolate( ( Noise3DValueTable[ hash_100 ][ 0 ] * ( offset_of_x - 1 ) +
                              Noise3DValueTable[ hash_100 ][ 1 ] * offset_of_y +
                              Noise3DValueTable[ hash_100 ][ 2 ] * offset_of_z ),
                            ( Noise3DValueTable[ hash_101 ][ 0 ] * ( offset_of_x - 1 ) +
                              Noise3DValueTable[ hash_101 ][ 1 ] * offset_of_y +
                              Noise3DValueTable[ hash_101 ][ 2 ] * ( offset_of_z - 1 ) ),
                            fade_z );
    hash_11 = NoiseZPermutationTable[ ( hash_1 + integer_of_y + 1 ) & 0xFF ];
    hash_110 = NoiseZPermutationTable[ ( hash_11 + integer_of_z ) & 0xFF ];
    hash_111 = NoiseZPermutationTable[ ( hash_11 + integer_of_z + 1 ) & 0xFF ];
    value_11 = Interpolate( ( Noise3DValueTable[ hash_110 ][ 0 ] * ( offset_of_x - 1 ) +
                              Noise3DValueTable[ hash_110 ][ 1 ] * ( offset_of_y - 1 ) +
                              Noise3DValueTable[ hash_110 ][ 2 ] * offset_of_z ),
                            ( Noise3DValueTable[ hash_111 ][ 0 ] * ( offset_of_x - 1 ) +
                              Noise3DValueTable[ hash_111 ][ 1 ] * ( offset_of_y - 1 ) +
                              Noise3DValueTable[ hash_111 ][ 2 ] * ( offset_of_z - 1 ) ),
                            fade_z );
    value_1 = Interpolate( value_10, value_11, fade_y );
    Real z_result = Interpolate( value_0, value_1, fade_x );
    return Vector( x_result, y_result, z_result );
  }

  Real ScalarCellNoise( Vector const& location )
  {
    int integer_of_x = Floor( location.x() );
    int integer_of_y = Floor( location.y() );
    int integer_of_z = Floor( location.z() );
    int hash = NoiseXPermutationTable[ integer_of_x & 0xFF ];
    hash = NoiseXPermutationTable[ ( hash + integer_of_y ) & 0xFF ];
    hash = NoiseXPermutationTable[ ( hash + integer_of_z ) & 0xFF ];
    return Noise1DValueTable[ hash ];
  }

  Vector const VectorCellNoise( Vector const& location )
  {
    int integer_of_x = Floor( location.x() );
    int integer_of_y = Floor( location.y() );
    int integer_of_z = Floor( location.z() );
    int hash = NoiseXPermutationTable[ integer_of_x & 0xFF ];
    hash = NoiseXPermutationTable[ ( hash + integer_of_y ) & 0xFF ];
    hash = NoiseXPermutationTable[ ( hash + integer_of_z ) & 0xFF ];
    Real x_result = Noise1DValueTable[ hash ];
    hash = NoiseYPermutationTable[ integer_of_x & 0xFF ];
    hash = NoiseYPermutationTable[ ( hash + integer_of_y ) & 0xFF ];
    hash = NoiseYPermutationTable[ ( hash + integer_of_z ) & 0xFF ];
    Real y_result = Noise1DValueTable[ hash ];
    hash = NoiseZPermutationTable[ integer_of_x & 0xFF ];
    hash = NoiseZPermutationTable[ ( hash + integer_of_y ) & 0xFF ];
    hash = NoiseZPermutationTable[ ( hash + integer_of_z ) & 0xFF ];
    Real z_result = Noise1DValueTable[ hash ];
    return Vector( x_result, y_result, z_result );
  }

  Real ScalarFBM( Vector const& location,
                  int           octaves,
                  Real          lacunarity,
                  Real          gain )
  {
    Real sum = 0;
    Real scale = 1;
    Real amplitude = 1;
    for( int octave = 0; octave < octaves; ++octave ) {
      sum += amplitude * ScalarNoise( location * scale );
      scale *= lacunarity;
      amplitude *= gain;
    }
    return sum;
  }

  Vector const VectorFBM( Vector const& location,
                          int           octaves,
                          Real          lacunarity,
                          Real          gain )
  {
    Vector sum( 0, 0, 0 );
    Real scale = 1;
    Real amplitude = 1;
    for( int octave = 0; octave < octaves; ++octave ) {
      sum += amplitude * VectorNoise( location * scale );
      scale *= lacunarity;
      amplitude *= gain;
    }
    return sum;
  }

  Real Turbulence( Vector const& location,
                   int           octaves,
                   Real          lacunarity,
                   Real          gain )
  {
    Real sum = 0;
    Real scale = 1;
    Real amplitude = 1;
    for( int octave = 0; octave < octaves; ++octave ) {
      sum += amplitude * Abs( ScalarNoise( location * scale ) );
      scale *= lacunarity;
      amplitude *= gain;
    }
    return sum;
  }

}

