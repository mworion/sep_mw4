/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*
* This file is part of SEP
*
* Copyright 1993-2011 Emmanuel Bertin -- IAP/CNRS/UPMC
* Copyright 2014 SEP developers
*
* SEP is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* SEP is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with SEP.  If not, see <http://www.gnu.org/licenses/>.
*
*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/* datatype codes */
#define SEP_TBYTE        11  /* 8-bit unsigned byte */
#define SEP_TINT         31  /* native int type */
#define SEP_TFLOAT       42
#define SEP_TDOUBLE      82

/* object flags */
#define SEP_OBJ_CROWDED    0x0001
#define SEP_OBJ_MERGED     0x0002
#define SEP_OBJ_SATUR      0x0004
#define SEP_OBJ_TRUNC      0x0008
#define SEP_APER_TRUNC     0x0010
#define SEP_OBJ_ISO_PB     0x0020
#define SEP_OBJ_DOVERFLOW  0x0040
#define SEP_OBJ_OVERFLOW   0x0080
#define SEP_OBJ_SINGU      0x0100
#define SEP_APER_HASMASKED 0x0200

/* input flags for aperture photometry */
#define SEP_ERROR_IS_VAR   0x0001
#define SEP_ERROR_IS_ARRAY 0x0002

/*--------------------- global background estimation ------------------------*/

typedef struct
{
  int w, h;          /* original image width, height */
  int bw, bh;        /* single tile width, height */
  int nx, ny;        /* number of tiles in x, y */
  int n;             /* nx*ny */
  float globalback;  /* global mean */
  float globalrms;   /* global sigma */
  float *back;       /* node data for interpolation */
  float *dback;
  float *sigma;    
  float *dsigma;
} sepbackmap;

int sep_makeback(void *im,            /* image data                          */
		 void *mask,          /* mask (can be NULL)                  */
		 int dtype,           /* image datatype code                 */
		 int mdtype,          /* mask datatype code                  */
		 int w, int h,        /* image and mask size (width, height) */
		 int bw, int bh,      /* size of a single background tile    */
		 float mthresh,       /* only (mask<=maskthresh) counted     */
		 int fw, int fh,      /* filter size in tiles                */
		 float fthresh,       /* filter threshold                    */
		 sepbackmap **bkmap); /* OUTPUT                              */
/* Create representation of spatially varying image background and variance.
 *
 * Note that the returned pointer must eventually be freed by calling 
 * `sep_freeback()`.
 *
 * If a mask is supplied, only pixels with mask value <= mthresh are counted.
 * In addition to the mask, pixels <= -1e30 and NaN are ignored.
 * 
 * Source Extractor defaults:
 * 
 * - bw, bh = (64, 64)
 * - fw, fh = (3, 3)
 * - fthresh = 0.0
 */

float sep_globalback(sepbackmap *bkmap);
float sep_globalrms(sepbackmap *bkmap);
/* Get the estimate of the global background "mean" or standard deviation */

float sep_backpix_linear(sepbackmap *bkmap, int x, int y);
/* Return background at (x, y).
 * Unlike other routines, this uses simple linear interpolation. */

int sep_backline(sepbackmap *bkmap, int y, void *line, int dtype);
int sep_subbackline(sepbackmap *bkmap, int y, void *line, int dtype);
int sep_backrmsline(sepbackmap *bkmap, int y, void *line, int dtype);
/* Evaluate the background, RMS, or variance at line y.
 * Uses bicubic spline interpolation between background map verticies.
 * The second function subtracts the background from the input array.
 * Line must be an array with same width as original image. */

int sep_backarray(sepbackmap *bkmap, void *arr, int dtype);
int sep_subbackarray(sepbackmap *bkmap, void *arr, int dtype);
int sep_backrmsarray(sepbackmap *bkmap, void *arr, int dtype);
/* Evaluate the background, RMS, or variance for entire image.
 * Uses bicubic spline interpolation between background map verticies.
 * The second function subtracts the background from the input array.
 * Arr must be an array of the same size as original image. */

void sep_freeback(sepbackmap *bkmap);
/* Free memory associated with bkmap */

/*-------------------------- source extraction ------------------------------*/

typedef struct
{
  float	   thresh;               /* threshold (ADU)                          */
  int	   npix;                 /* # pixels extracted (size of pix array)   */
  int      tnpix;                /* # pixels above thresh (unconvolved)      */
  int	   xmin,xmax,ymin,ymax;  /* x,y limits                               */
  double   x, y;                 /* barycenter (first moments)               */
  double   x2,y2,xy;		 /* second moments                           */
  float	   a, b, theta;          /* ellipse parameters                       */
  float	   cxx,cyy,cxy;	         /* ellipse parameters (alternative)         */
  float	   cflux;                /* total flux of pixels (convolved im)      */
  float	   flux;      		 /* total flux of pixels (unconvolved)       */
  float    cpeak;                /* peak intensity (ADU) (convolved)         */
  float    peak;                 /* peak intensity (ADU) (unconvolved)       */
  int      xcpeak, ycpeak;       /* x, y coords of peak (convolved) pixel    */
  int      xpeak, ypeak;         /* x, y coords of peak (unconvolved) pixel  */
  short	   flag;                 /* extraction flags                         */
  int      *pix;                 /* pixel array (length is npix)             */
} sepobj;

int sep_extract(void *image,          /* image array                         */
		void *noise,          /* noise array (can be NULL)    [NULL] */
		int dtype,            /* data type of image                  */
		int ndtype,           /* data type of noise                  */
		short noise_flag,     /* See detail below.                   */
		int w, int h,         /* width, height of image & noise      */
		float thresh,         /* detection threshold     [1.5*sigma] */
		int minarea,          /* minimum area in pixels          [5] */
		float *conv,          /* convolution array (can be NULL)     */
                                      /*               [{1 2 1 2 4 2 1 2 1}] */
		int convw, int convh, /* w, h of convolution array     [3,3] */
		int deblend_nthresh,  /* deblending thresholds          [32] */
		double deblend_cont,  /* min. deblending contrast    [0.005] */
		int clean_flag,       /* perform cleaning?               [1] */
		double clean_param,   /* clean parameter               [1.0] */
		sepobj **objects,     /* OUTPUT: object array                */
		int *nobj);           /* OUTPUT: number of objects           */
/* Extract sources from an image.
 *
 * Source Extractor defaults are shown in [ ] above.
 *
 * Notes
 * -----
 * `dtype` and `ndtype` indicate the data type (float, int, double) of the 
 * image and noise arrays, respectively.
 *
 * If `noise` is NULL, thresh is interpreted as an absolute threshold.
 *
 * If `noise` is not null, thresh is interpreted as a relative threshold
 * (the absolute threshold will be thresh*noise). `noise_flag` can be used
 * to alter this behavior.
 * 
 */

void sep_freeobjarray(sepobj *objects, int nobj);
/* free memory associated with an sepobj array, including pixel lists */


/*-------------------------- aperture photometry ----------------------------*/

/* alternative names? 
   sep_circsum(), sep_circannsum(), sep_ellipsum() */

int sep_apercirc(void *data,        /* data array */
		 void *error,       /* error value or array or NULL */
		 void *mask,        /* mask array (can be NULL) */
		 int dtype,         /* data dtype code */
		 int edtype,        /* error dtype code */
		 int mdtype,        /* mask dtype code */
		 int w, int h,      /* width, height of input arrays */
		 double maskthresh, /* pixel masked if mask > maskthresh */
		 double gain,       /* (poisson counts / data unit) */
		 short inflags,     /* input flags (see below) */
		 double x,          /* center of aperture in x */
		 double y,          /* center of aperture in y */
		 double r,          /* radius of aperture */
		 int subpix,        /* subpixel sampling */
		 double *sum,       /* OUTPUT: sum */
		 double *sumerr,    /* OUTPUT: error on sum */
		 double *area,      /* OUTPUT: area included in sum */
		 short *flag);      /* OUTPUT: flags */
/* Sum array values within a circular aperture.
 * 
 * Notes
 * -----
 * error : Can be a scalar (default), an array, or NULL
 *         If an array, set the flag SEP_ERROR_IS_ARRAY in `inflags`.
 *         Can represent 1-sigma std. deviation (default) or variance.
 *         If variance, set the flag SEP_ERROR_IS_VARIANCE in `inflags`.
 *
 * gain : If 0.0, poisson noise on sum is ignored when calculating error.
 *        Otherwise, (sum / gain) is added to the variance on sum.
 *
 * area : Total pixel area included in sum. Includes masked pixels that were
 *        corrected. The area can differ from the exact area of a circle due
 *        to inexact subpixel sampling and intersection with array boundaries.
 */

int sep_apercircann(void *data, void *error, void *mask,
		    int dtype, int edtype, int mdtype, int w, int h,
		    double maskthresh, double gain, short inflags,
		    double x, double y, double rin, double rout, int subpix,
		    double *sum, double *sumerr, double *area, short *flag);

/*----------------------- info & error messaging ----------------------------*/

extern char *sep_version_string;
/* library version string (e.g., "0.2.0") */

void sep_get_errmsg(int status, char *errtext);
/* Return a short descriptive error message that corresponds to the input
 * error status value.  The message may be up to 60 characters long, plus
 * the terminating null character. */

void sep_get_errdetail(char *errtext);
/* Return a longer error message with more specifics about the problem.
   The message may be up to 512 characters */
