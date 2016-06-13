#include "vboplane.h"

#include <cstdio>
#include <cmath>

VBOPlane::~VBOPlane()
{
    delete [] v;
    delete [] n;
    delete [] tex;
    delete [] el;
}

VBOPlane::VBOPlane(float xsize, float zsize, int xdivs, int zdivs, float smax, float tmax)
{
    nFaces = xdivs * zdivs;
    nVerts = (xdivs+1) * (zdivs+1);

    v = new float[3 * (xdivs + 1) * (zdivs + 1)];
    n = new float[3 * (xdivs + 1) * (zdivs + 1)];
    tex = new float[2 * (xdivs + 1) * (zdivs + 1)];
    el = new unsigned int[6 * xdivs * zdivs];

    float x2 = xsize / 2.0f;
    float z2 = zsize / 2.0f;
    float iFactor = (float)zsize / zdivs;
    float jFactor = (float)xsize / xdivs;
    float texi = smax / zdivs;
    float texj = tmax / xdivs;
    float x, z;
    int vidx = 0, tidx = 0;
    for( int i = 0; i <= zdivs; i++ ) {
        z = iFactor * i - z2;
        for( int j = 0; j <= xdivs; j++ ) {
            x = jFactor * j - x2;
            v[vidx] = x;
            v[vidx+1] = 0.0f;
            v[vidx+2] = z;
			n[vidx] = 0.0f;
			n[vidx+1] = 1.0f;
			n[vidx+2] = 0.0f;
            vidx += 3;
            tex[tidx] = j * texi;
            tex[tidx+1] = i * texj;
            tidx += 2;
        }
    }

    unsigned int rowStart, nextRowStart;
    int idx = 0;
    for( int i = 0; i < zdivs; i++ ) {
        rowStart = i * (xdivs+1);
        nextRowStart = (i+1) * (xdivs+1);
        for( int j = 0; j < xdivs; j++ ) {
            el[idx] = rowStart + j;
            el[idx+1] = nextRowStart + j;
            el[idx+2] = nextRowStart + j + 1;
            el[idx+3] = rowStart + j;
            el[idx+4] = nextRowStart + j + 1;
            el[idx+5] = rowStart + j + 1;
            idx += 6;
        }
    }
}

float *VBOPlane::getv()
{
    return v;
}

unsigned int VBOPlane::getnVerts()
{
    return nVerts;
}

float *VBOPlane::getn()
{
    return n;
}

float *VBOPlane::gettc()
{
    return tex;
}

unsigned int *VBOPlane::getelems()
{
    return el;
}

unsigned int VBOPlane::getnFaces()
{
    return nFaces;
}
