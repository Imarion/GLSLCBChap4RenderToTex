#ifndef VBOCUBE_H
#define VBOCUBE_H

class VBOCube
{

private:
    int nFaces;

    // Vertices
    float *v;
    int nVerts;

    // Normals
    float *n;

    // Tex coords
    float *tc;

    // Elements
    unsigned int *el;

public:
    VBOCube();

    float *getv();
    int    getnVerts();
    float *getn();
    float *gettc();
    unsigned int *getel();

    int    getnFaces();
};

#endif // VBOCUBE_H
