/*
 * src/drawStone.c, part of Complete Goban (game program)
 * Copyright (C) 1995-1996 William Shubert.
 * See "configure.h.in" for more copyright information.
 *
 * This code extends the wmslib/but library to add special buttons needed
 *   for cgoban.
 *
 * The globe data was extracted from the CIA World Data Bank II map database.
 *
 */

#include <math.h>
#include <wms.h>
#include <but/but.h>
#include <but/net.h>
#include <wms/rnd.h>
#include <wms/str.h>
#include "goBoard.h"
#ifdef  _DRAWSTONE_H_
  Levelization Error.
#endif
#include "drawStone.h"


/**********************************************************************
 * Data Types
 **********************************************************************/
typedef struct WhiteDesc_struct  {
  float  cosTheta, sinTheta;
  float  stripeWidth, xAdd;
  float  stripeMul, zMul;
} WhiteDesc;


/**********************************************************************
 * Forward Declarations
 **********************************************************************/
static void  decideAppearance(WhiteDesc *desc, int size, Rnd *rnd);
static float  calcCosAngleReflection2View(int x, int y, int r,
					  float *lambertian, float *z);
static void  drawNngs(Display *dpy, GC gc, Pixmap pic, int xOff, int size);
static void  drawIgs(Display *dpy, GC gc, Pixmap pic, int xOff, int size);
static void  drawWorld(Display *dpy, GC gc, Pixmap pic, int xOff, int size,
		       const XPoint data[]);
static int  yinYangX(int size, int y);


/**********************************************************************
 * Global Variables
 **********************************************************************/
void  drawStone_newPics(ButEnv *env, Rnd *rnd, int baseColor,
			Pixmap *stonePixmap, Pixmap *maskBitmap, int size,
			bool color)  {
  XImage  *stones;
  GC  gc1;
  Display  *dpy = env->dpy;
  uchar  *maskData;
  int  maxRadius;
  int  maskW;
  int  x, y, i, wBright, bBright, yyx;
  XGCValues  values;
  float  bright, lambertian, z;
  float  wStripeLoc, wStripeColor;
  WhiteDesc  white[DRAWSTONE_NUMWHITE];
  int  sizeLimit;

  stones = butEnv_imageCreate(env, size * (DRAWSTONE_NUMWHITE + 1), size);
  maskW = (size * 6 + 7) >> 3;
  maskData = wms_malloc(maskW * size);
  memset(maskData, 0, maskW * size);
  for (i = 0;  i < DRAWSTONE_NUMWHITE;  ++i)  {
    decideAppearance(&white[i], size, rnd);
  }
  maxRadius = size * size;
  sizeLimit = (int)((size - butEnv_fontH(env, 0) * 0.14) *
		    (size - butEnv_fontH(env, 0) * 0.14));
  if (sizeLimit > ((size - 1) * (size - 1)))
    sizeLimit = (size - 1) * (size - 1);
  for (y = 0;  y < size;  ++y)  {
    yyx = yinYangX(size, y);
    for (x = 0;  x < size;  ++x)  {
      /*
       * Here we build the masks.  We need three basic masks; the solid
       *   circular mask, and two dithered masks used for semitransparent
       *   stones.
       * Then after that we have the yin yang mask.  We have the world masks
       *   too, but we make those in a separate function.
       */
      if (((size - 1) - (x+x)) * ((size - 1) - (x+x)) +
	  ((size - 1) - (y+y)) * ((size - 1) - (y+y)) <= maxRadius)  {
	maskData[(x >> 3) + (y * maskW)] |= (1 << (x & 7));
	x += size;
	if ((x & 1) == (y & 1))
	  maskData[(x >> 3) + (y * maskW)] |= (1 << (x & 7));
	x += size;
	if ((x & 1) == (y & 1))
	  maskData[(x >> 3) + (y * maskW)] |= (1 << (x & 7));
	x += size;
	if (x - size*3 < yyx)
	  maskData[(x >> 3) + (y * maskW)] |= (1 << (x & 7));
	x -= size*3;

	/*
	 * Now we do the actual stone.  If it's a greyscale/b&w display, it's
	 *   easy because there is no rendering going on.
	 */
	if (!color)  {
	  XPutPixel(stones, x, y, butEnv_color(env, BUT_BLACK));
	  if (((size - 1) - (x+x)) * ((size - 1) - (x+x)) +
	      ((size - 1) - (y+y)) * ((size - 1) - (y+y)) <
	      sizeLimit)  {
	    for (i = 0;  i < DRAWSTONE_NUMWHITE;  ++i)  {
	      XPutPixel(stones, x+size*(i+1), y, butEnv_color(env, BUT_WHITE));
	    }
	  } else  {
	    for (i = 0;  i < DRAWSTONE_NUMWHITE;  ++i)  {
	      XPutPixel(stones, x+size*(i+1), y, butEnv_color(env, BUT_BLACK));
	    }
	  }
	} else  {
	  /*
	   * All right, time to add some color.  First we calculate the
	   *   cosine of the angle of reflection to the angle of the viewer.
	   *   This is the basic quantity used to calculate the amount of
	   *   light seen in phong shading.  You raise this value to a
	   *   power; the higher the power, the "shinier" the object you are
	   *   rendering.  For the black stones, which aren't very shiny,
	   *   I use bright^4.  For the white stones, which are very shiny,
	   *   I use bright^32.  Then you multiply this value by the
	   *   intensity of the object at the point of rendering, add a
	   *   little bit of lambertian reflection and a tiny bit of ambient
	   *   light, and you're done!
	   */
	  bright = calcCosAngleReflection2View((size - 1) - (x+x),
					       (size - 1) - (y + y),
					       size+size, &lambertian, &z);
	  bright *= bright;
	  bright *= bright;
	  bBright = bright*165.0 + lambertian*10.0 - 5.0;
	  bright *= bright;
	  bright *= bright;
	  bright *= bright;
	  if (bBright > 255)
	    bBright = 255;
	  if (bBright < 0)
	    bBright = 0;
	  XPutPixel(stones, x, y, butEnv_color(env, baseColor + bBright));
	  /*
	   * OK, the black stones are done.  Now for the white stones.
	   * Here we have to add the stripes.  The algorithm for stripe
	   *   intensity is just something I made up.  I kept tweaking
	   *   parameters and screwing around with it until it looked sort
	   *   of like my stones.  The stripes are too regular, some day I
	   *   may go back and change that, but for now it is acceptable
	   *   looking IMHO.
	   */
	  for (i = 0;  i < DRAWSTONE_NUMWHITE;  ++i)  {
	    wStripeLoc = (x*white[i].cosTheta - y*white[i].sinTheta) +
	      white[i].xAdd;
	    wStripeColor = fmod(wStripeLoc + (z * z * z * white[i].zMul) *
				white[i].stripeWidth,
				white[i].stripeWidth) / white[i].stripeWidth;
	    wStripeColor = wStripeColor * white[i].stripeMul - 0.5;
	    if (wStripeColor < 0.0)
	      wStripeColor = -2.0 * wStripeColor;
	    if (wStripeColor > 1.0)
	      wStripeColor = 1.0;
	    wStripeColor = wStripeColor * 0.15 + 0.85;
	    wBright = bright*bright*250.0 +
	      wStripeColor * (lambertian*120.0 + 110.0);
	    if (wBright > 255)
	      wBright = 255;
	    if (wBright < 0)
	      wBright = 0;
	    XPutPixel(stones, x+(i+1)*size, y,
		      butEnv_color(env, baseColor + wBright));
	  }
	}
      }
    }
  }
  *stonePixmap =
    XCreatePixmap(dpy, RootWindow(dpy, DefaultScreen(dpy)),
		  size * (DRAWSTONE_NUMWHITE + 1),
		  size, DefaultDepth(dpy, DefaultScreen(dpy)));
  XPutImage(dpy, *stonePixmap, env->gc2, stones, 0,0, 0,0,
	    size * (DRAWSTONE_NUMWHITE + 1), size);
  butEnv_imageDestroy(stones);
  *maskBitmap =
    XCreateBitmapFromData(dpy, RootWindow(dpy, DefaultScreen(dpy)),
			  (void *)maskData, maskW<<3, size);
  /*
   * The "void *" above is because maskData should be a uchar, but the
   *   function prototype specifies a char.
   */
  wms_free(maskData);

  values.graphics_exposures = False;
  gc1 = XCreateGC(dpy, *maskBitmap, GCGraphicsExposures, &values);

  /* Do that whole world thing, you know, the world thing. */
  XSetForeground(dpy, gc1, 1);
  drawNngs(dpy, gc1, *maskBitmap, size*4, size);
  drawIgs(dpy, gc1, *maskBitmap, size*5, size);
  XSetFunction(dpy, gc1, GXand);
  XCopyArea(dpy, *maskBitmap, *maskBitmap, gc1, 0,0, size,size, size*4,0);
  XCopyArea(dpy, *maskBitmap, *maskBitmap, gc1, 0,0, size,size, size*5,0);
  XFreeGC(dpy, gc1);
}


/*
 * Viewing vector is simply "z".
 * I model the stones as the top part of the sphere with the viewer looking
 *   straight down on them.  lx*i+ly*j+lz*k is the angle of incident light.
 *   If you draw out the vectors and work out the equation, you'll notice that
 *   it very nicely simplifies down to what you get here.  The lambertian
 *   intensity (lambertian) and the z magnitude of the surface normal (z)
 *   aren't really related to the cosine being calculated, but they share
 *   many operations with it so it saves CPU time to calculate them at the
 *   same time.
 */
static float  calcCosAngleReflection2View(int x, int y, int r,
					  float *lambertian, float *z)  {
  const float  lx = 0.35355339, ly = 0.35355339, lz = 0.8660254;
  float  nx, ny, nz, rz;
  float  nDotL;

  nz = sqrt((double)(r * r - x * x - y * y));
  *z = 1.0 - (nz / r);
  nx = (float)x;
  ny = (float)y;
  nDotL = (nx*lx + ny*ly + nz*lz) / r;
  rz = (2.0 * nz * nDotL) / r - lz;
  *lambertian = nDotL;
  return(rz);
}


/*
 * drawNngs and drawIgs are a list of vectors to draw to outline the world.
 *   I took GIFs produced by XGlobe, which got it's data from the
 *   CIA World Data Bank II map database.  Then I wrote a program that
 *   traced the outlines of the continents of those gifs, clipped out
 *   unnecessare points, and saved the results as these list of points for
 *   the polygons.  Pretty cool, huh?
 */
static void  drawNngs(Display *dpy, GC gc, Pixmap pic, int xOff, int size)  {
  static const XPoint  nngsData[] = {
    {120,0},{145,0},{147,2},{153,2},{155,4},{157,4},{159,6},{161,6},{161,9},
    {159,11},{159,17},{158,17},{156,15},{152,15},{148,11},{146,11},{140,5},
    {138,5},{136,3},{130,3},{129,2},{128,2},{126,4},{126,5},{127,6},{131,6},
    {133,8},{135,8},{137,10},{137,13},{135,15},{134,15},{132,13},{128,13},
    {126,11},{124,11},{124,9},{123,8},{122,8},{119,11},{119,13},{116,13},
    {116,7},{115,6},{114,6},{113,7},{113,13},{109,13},{105,17},{105,18},
    {107,20},{111,20},{113,22},{117,22},{117,26},{118,27},{119,27},{120,26},
    {120,22},{122,22},{123,21},{123,20},{122,19},{122,14},{125,14},{127,16},
    {131,16},{133,18},{136,18},{138,16},{139,16},{145,22},{147,22},{149,24},
    {151,24},{151,28},{153,30},{157,30},{157,33},{150,33},{150,29},{149,28},
    {148,28},{147,29},{145,29},{142,32},{142,33},{143,34},{147,34},{147,37},
    {145,37},{143,39},{140,39},{138,37},{137,37},{133,41},{133,43},{131,43},
    {123,51},{123,53},{119,57},{117,57},{113,61},{113,66},{115,68},{115,71},
    {113,73},{112,73},{110,71},{110,67},{106,63},{99,63},{97,65},{87,65},
    {85,67},{83,67},{81,69},{81,75},{79,77},{79,82},{84,87},{85,87},{86,86},
    {90,86},{92,84},{92,82},{94,80},{99,80},{99,85},{97,87},{97,90},{99,92},
    {105,92},{107,94},{107,99},{105,101},{105,102},{110,107},{111,107},
    {112,106},{117,106},{119,108},{120,108},{122,106},{122,104},{124,102},
    {128,102},{130,100},{133,100},{135,102},{139,102},{141,104},{155,104},
    {155,106},{159,110},{161,110},{165,114},{171,114},{173,116},{175,116},
    {177,118},{177,122},{179,124},{179,126},{181,128},{185,128},{187,130},
    {189,130},{191,132},{193,132},{195,134},{199,134},{203,138},{205,138},
    {207,140},{207,147},{205,149},{205,151},{199,157},{199,161},{197,163},
    {197,167},{195,169},{195,171},{191,175},{191,177},{187,177},{185,179},
    {183,179},{181,181},{179,181},{177,183},{177,185},{175,187},{175,189},
    {163,201},{159,201},{157,203},{157,205},{155,207},{153,207},{151,209},
    {149,209},{138,220},{138,221},{139,222},{139,223},{135,227},{135,230},
    {137,232},{137,233},{132,233},{130,231},{128,231},{128,229},{126,227},
    {126,220},{128,218},{128,217},{126,215},{126,212},{128,210},{128,204},
    {130,202},{130,198},{132,196},{132,186},{134,184},{134,169},{128,163},
    {126,163},{124,161},{122,161},{122,157},{118,153},{118,151},{116,149},
    {116,147},{110,141},{110,136},{112,134},{112,126},{114,126},{116,124},
    {116,122},{119,119},{119,118},{118,117},{118,111},{115,108},{114,108},
    {113,109},{113,111},{110,111},{108,109},{106,109},{102,105},{100,105},
    {100,103},{96,99},{94,99},{92,97},{88,97},{83,92},{82,92},{81,93},
    {76,93},{74,91},{72,91},{70,89},{68,89},{64,85},{64,77},{60,73},{60,65},
    {57,62},{56,62},{55,63},{55,64},{57,66},{57,74},{59,76},{59,77},{56,77},
    {56,73},{52,69},{52,68},{54,66},{54,65},{52,63},{52,57},{50,55},{50,48},
    {52,46},{52,44},{54,42},{54,40},{60,34},{60,32},{62,30},{62,28},{63,27},
    {63,26},{62,25},{62,24},{67,19},{67,18},{66,17},{62,17},{62,16},{64,16},
    {66,14},{68,14},{70,12},{72,12},{74,10},{76,10},{78,8},{82,8},{84,6},
    {87,6},{88,7},{89,7},{90,6},{91,6},{93,8},{94,8},{98,4},{104,4},{106,2},
    {107,2},{107,6},{108,7},{109,7},{110,6},{110,2},{111,2},{111,4},{112,5},
    {113,5},{114,4},{114,2},{118,2},{119,1},{-1,-1},{168,10},{173,10},
    {175,12},{175,13},{172,13},{170,11},{168,11},{-1,-1},{186,14},{189,14},
    {197,22},{199,22},{201,24},{205,24},{213,32},{215,32},{219,36},{219,38},
    {221,40},{221,44},{225,48},{225,50},{226,51},{227,51},{228,50},{229,50},
    {235,56},{235,58},{239,62},{239,64},{241,66},{241,68},{243,70},{243,72},
    {245,74},{245,76},{247,78},{247,82},{249,84},{249,88},{251,90},{251,94},
    {253,96},{253,104},{255,106},{255,137},{254,137},{254,119},{251,116},
    {250,116},{249,117},{248,117},{247,116},{246,116},{245,117},{242,117},
    {242,115},{238,111},{238,107},{232,101},{232,97},{230,95},{230,87},
    {228,85},{228,83},{226,81},{226,66},{227,65},{227,64},{226,63},{226,61},
    {224,59},{224,53},{222,51},{220,51},{216,47},{216,45},{212,41},{212,38},
    {213,37},{213,36},{212,35},{210,35},{202,27},{198,27},{190,19},{190,17},
    {188,15},{186,15},{-1,-1},{110,76},{113,76},{115,78},{117,78},{119,80},
    {121,80},{123,82},{125,82},{125,83},{123,83},{120,86},{120,87},{121,88},
    {121,89},{118,89},{118,86},{119,85},{119,84},{116,81},{114,81},{112,79},
    {106,79},{106,78},{108,78},{109,77},{-1,-1},{128,84},{135,84},{137,86},
    {139,86},{139,87},{135,87},{133,89},{132,89},{130,87},{126,87},{126,86},
    {127,85},{-1,-1},{142,86},{143,86},{143,89},{142,89},{142,87},{-1,-1},
    {88,128},{89,128},{89,129},{88,129},{-1,-1},{254,142},{255,142},
    {255,143},{254,143},{-1,-1},{250,162},{251,162},{251,165},{250,165},
    {250,163},{-1,-1},{146,228},{149,228},{149,229},{146,229},{-1,-1},
    {140,242},{143,242},{143,243},{141,243},{139,245},{137,245},{136,246},
    {136,247},{137,248},{137,249},{135,251},{133,251},{132,252},{132,253},
    {133,254},{136,254},{138,252},{139,252},{141,254},{142,254},{144,252},
    {154,252},{156,250},{162,250},{164,248},{167,248},{167,249},{165,251},
    {161,251},{159,253},{151,253},{149,255},{106,255},{104,253},{98,253},
    {98,250},{107,250},{108,251},{109,251},{110,250},{124,250},{126,248},
    {130,248},{134,244},{138,244},{139,243},{-2,-2}};

  drawWorld(dpy, gc, pic, xOff, size, nngsData);
}


static void  drawIgs(Display *dpy, GC gc, Pixmap pic, int xOff, int size)  {
  static const XPoint  igsData[] = {
    {106,2},{115,2},{117,4},{121,4},{123,6},{126,6},{128,4},{128,2},{131,2},
    {135,6},{143,6},{145,8},{163,8},{164,9},{165,9},{166,8},{166,6},{171,6},
    {173,8},{177,8},{179,10},{181,10},{183,12},{185,12},{187,14},{189,14},
    {191,16},{191,23},{190,23},{190,21},{186,17},{180,17},{174,11},{167,11},
    {165,13},{165,15},{163,15},{161,17},{159,17},{157,19},{157,23},{155,25},
    {155,27},{152,27},{150,25},{150,18},{151,17},{151,16},{150,15},{149,15},
    {147,17},{133,17},{128,22},{128,23},{129,24},{135,24},{135,28},{137,30},
    {137,31},{135,33},{134,33},{134,29},{133,28},{132,28},{131,29},{131,33},
    {127,37},{125,37},{121,41},{117,41},{111,47},{111,48},{113,50},{113,53},
    {111,55},{106,55},{106,51},{104,49},{104,48},{105,47},{105,46},{104,45},
    {103,45},{101,47},{100,47},{98,45},{97,45},{95,47},{95,48},{97,50},
    {101,50},{101,51},{97,51},{95,53},{95,56},{97,58},{97,65},{91,71},
    {91,72},{92,73},{93,73},{94,72},{95,72},{95,77},{93,79},{92,79},{92,75},
    {91,74},{90,74},{89,75},{87,75},{85,77},{83,77},{81,79},{77,79},{73,83},
    {73,85},{71,87},{68,87},{68,84},{69,83},{69,82},{68,81},{67,81},{63,85},
    {63,88},{67,92},{67,103},{65,103},{59,109},{58,109},{58,105},{53,100},
    {52,100},{49,103},{49,108},{55,114},{55,128},{59,132},{61,132},{63,134},
    {63,135},{61,135},{59,137},{59,138},{61,140},{63,140},{65,142},{75,142},
    {79,146},{93,146},{93,147},{91,147},{89,149},{89,151},{88,151},{88,149},
    {86,147},{70,147},{68,145},{62,145},{56,139},{54,139},{52,137},{52,135},
    {50,133},{50,131},{49,130},{48,130},{47,131},{46,131},{46,130},{47,129},
    {47,128},{44,125},{44,121},{40,117},{40,116},{45,116},{48,119},{49,119},
    {50,118},{50,115},{48,113},{48,111},{46,109},{46,106},{48,104},{48,93},
    {47,92},{46,92},{45,93},{44,93},{42,91},{42,90},{44,88},{44,87},{42,85},
    {42,81},{40,79},{39,79},{23,95},{23,99},{21,101},{21,108},{23,110},
    {23,111},{21,113},{21,115},{20,115},{20,109},{19,108},{18,108},{17,109},
    {16,109},{16,88},{18,86},{18,83},{16,81},{16,78},{18,76},{18,75},{16,73},
    {15,73},{13,75},{13,77},{11,79},{11,81},{9,83},{9,85},{7,87},{7,89},
    {5,91},{4,91},{4,90},{6,88},{6,84},{8,82},{8,78},{10,76},{10,74},{14,70},
    {14,66},{16,64},{16,62},{20,58},{20,56},{24,52},{24,50},{34,40},{34,38},
    {36,36},{38,36},{40,34},{40,32},{42,32},{50,24},{52,24},{56,20},{58,20},
    {62,16},{64,16},{66,14},{68,14},{70,12},{72,12},{74,10},{76,10},{78,8},
    {84,8},{86,6},{90,6},{92,4},{93,4},{94,5},{95,5},{96,4},{104,4},{105,3},
    {-1,-1},{134,36},{135,36},{137,38},{139,38},{139,41},{135,41},{133,43},
    {133,44},{135,46},{135,47},{133,49},{133,53},{131,53},{129,55},{127,55},
    {125,57},{117,57},{115,59},{115,61},{112,61},{112,58},{118,52},{124,52},
    {130,46},{130,44},{131,43},{131,42},{130,41},{130,40},{132,40},{134,38},
    {134,37},{-1,-1},{92,86},{93,86},{95,88},{95,91},{93,93},{93,94},{95,96},
    {97,96},{97,98},{99,100},{101,100},{101,103},{98,106},{98,107},{99,108},
    {103,108},{103,111},{101,113},{101,115},{98,115},{95,112},{94,112},
    {93,113},{92,113},{92,110},{94,108},{94,105},{92,103},{92,102},{93,102},
    {95,104},{96,104},{97,103},{97,102},{96,101},{96,99},{95,98},{94,98},
    {93,99},{90,99},{90,95},{88,93},{88,92},{90,90},{90,88},{91,87},{-1,-1},
    {80,112},{83,112},{87,116},{87,117},{85,117},{83,119},{83,122},{85,124},
    {85,125},{83,127},{83,129},{81,131},{81,135},{79,137},{76,137},{74,135},
    {68,135},{68,131},{66,129},{66,124},{68,124},{76,116},{78,116},{80,114},
    {80,113},{-1,-1},{90,124},{93,124},{95,126},{96,126},{98,124},{99,124},
    {99,125},{97,127},{91,127},{90,128},{90,129},{91,130},{95,130},{95,131},
    {93,131},{92,132},{92,133},{93,134},{93,136},{95,138},{95,139},{92,139},
    {91,138},{90,138},{89,139},{89,141},{88,141},{86,139},{86,130},{88,128},
    {88,126},{89,125},{-1,-1},{104,124},{107,124},{107,127},{105,129},
    {105,131},{104,131},{104,125},{-1,-1},{114,128},{117,128},{119,130},
    {119,132},{121,134},{122,134},{123,133},{123,132},{122,131},{122,130},
    {123,130},{124,131},{125,131},{126,130},{127,130},{129,132},{133,132},
    {135,134},{139,134},{141,136},{143,136},{147,140},{149,140},{149,144},
    {155,150},{155,151},{150,151},{144,145},{141,145},{137,149},{134,149},
    {131,146},{130,146},{129,147},{128,147},{128,141},{126,139},{124,139},
    {122,137},{116,137},{114,135},{114,133},{112,131},{112,130},{113,129},
    {-1,-1},{100,132},{101,132},{103,134},{103,137},{102,137},{102,135},
    {100,133},{-1,-1},{106,134},{111,134},{111,135},{106,135},{-1,-1},
    {158,136},{161,136},{161,137},{157,141},{152,141},{152,140},{154,140},
    {157,137},{-1,-1},{98,146},{103,146},{103,147},{101,149},{99,149},
    {97,151},{96,151},{96,148},{97,147},{-1,-1},{174,148},{177,148},
    {179,150},{179,151},{178,151},{176,149},{174,149},{-1,-1},{110,152},
    {113,152},{114,153},{115,153},{116,152},{117,152},{119,154},{125,154},
    {125,159},{124,160},{124,161},{127,164},{129,164},{131,166},{132,166},
    {134,164},{134,158},{136,156},{136,152},{139,152},{139,156},{143,160},
    {143,162},{145,164},{145,168},{147,170},{149,170},{151,172},{151,174},
    {157,180},{157,193},{155,195},{155,197},{149,203},{149,205},{147,207},
    {132,207},{128,203},{126,203},{126,201},{125,200},{124,200},{123,201},
    {122,201},{122,199},{118,195},{109,195},{107,197},{103,197},{101,199},
    {95,199},{93,201},{88,201},{86,199},{86,195},{84,193},{84,191},{80,187},
    {80,183},{78,181},{78,176},{80,176},{84,172},{88,172},{90,170},{92,170},
    {94,168},{94,164},{96,164},{102,158},{105,158},{107,160},{108,160},
    {110,158},{110,156},{111,155},{111,154},{110,153},{-1,-1},{208,164},
    {209,164},{209,165},{207,167},{204,167},{204,166},{206,166},{207,165},
    {-1,-1},{180,172},{183,172},{185,174},{185,177},{184,177},{180,173},
    {-1,-1},{188,202},{189,202},{193,206},{193,207},{187,213},{186,213},
    {186,208},{188,206},{188,203},{-1,-1},{144,210},{145,210},{145,213},
    {143,215},{140,215},{138,213},{138,212},{142,212},{143,211},{-1,-1},
    {180,212},{183,212},{183,215},{181,215},{175,221},{170,221},{170,218},
    {172,218},{174,216},{176,216},{179,213},{-1,-1},{90,244},{101,244},
    {102,245},{103,245},{104,244},{109,244},{111,246},{112,246},{114,244},
    {117,244},{118,245},{119,245},{120,244},{129,244},{131,246},{141,246},
    {143,248},{149,248},{149,249},{147,251},{143,251},{141,253},{139,253},
    {137,255},{106,255},{104,253},{96,253},{94,251},{90,251},{88,249},
    {84,249},{82,247},{78,247},{78,246},{88,246},{89,245},{-1,-1},{152,252},
    {155,252},{155,253},{152,253},{-2,-2}};

  drawWorld(dpy, gc, pic, xOff, size, igsData);
}


/*
 * Here we take the world polygon point list and trace it out into a bitmap.
 */
static void  drawWorld(Display *dpy, GC gc, Pixmap pic, int xOff, int size,
		       const XPoint data[])  {
  XPoint  temp[344];
  int  i, numPoints;

  numPoints = 0;
  for (i = 0;;  ++i)  {
    if (data[i].x < 0)  {
      if (numPoints > 2)  {
	XFillPolygon(dpy, pic, gc, temp, numPoints, Complex, CoordModeOrigin);
      }
      numPoints = 0;
      if (data[i].x == -2)
	return;
    } else  {
      temp[numPoints].x = ((data[i].x * (size + 1)) >> 8) + xOff;
      temp[numPoints].y = ((data[i].y * (size + 1)) >> 8);
      if ((numPoints == 0) || (temp[numPoints - 1].x != temp[numPoints].x) ||
	  (temp[numPoints - 1].y != temp[numPoints].y))  {
	++numPoints;
	assert(numPoints <= 344);
      }
    }
  }
}


/*
 * Returns the answer:
 *   Where is the x coordinate of the "seam" in the yin yang symbol for this
 *   particular y value?
 */
static int  yinYangX(int size, int y)  {
  float  center;
  float  fy = y + 0.5;

  if (y < size / 2)  {
    center = (float)size * 0.25;
    return((int)((float)size * 0.5 +
		 sqrt(center * center -
		      (fy - center) * (fy - center)) +
		 0.5));
  } else  {
    center = (float)size * 0.75;
    return((int)((float)size * 0.5 -
		 sqrt((float)(size * size) * 0.0625 -
		      (fy - center) * (fy - center)) +
		 0.5));
  }
}


static void  decideAppearance(WhiteDesc *desc, int size, Rnd *rnd)  {
  double  minStripeW, maxStripeW, theta;

  minStripeW = (float)size / 20.0;
  if (minStripeW < 2.5)
    minStripeW = 2.5;
  maxStripeW = (float)size / 5.0;
  if (maxStripeW < 4.0)
    maxStripeW = 4.0;
  theta = rnd_float(rnd) * 2.0 * M_PI;
  desc->cosTheta = cos(theta);
  desc->sinTheta = sin(theta);
  desc->stripeWidth = minStripeW +
    (rnd_float(rnd) * (maxStripeW - minStripeW));
  desc->xAdd = rnd_float(rnd) * desc->stripeWidth +
    (float)size * 3;  /* Make sure that all x's are positive! */
  desc->stripeMul = rnd_float(rnd) * 4.0 + 1.5;
  desc->zMul = rnd_float(rnd) * 650.0 + 70.0;
}
