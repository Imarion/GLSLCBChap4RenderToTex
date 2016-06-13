#include <QWindow>
#include <QTimer>
#include <QString>
#include <QKeyEvent>

#include <QVector3D>
#include <QMatrix4x4>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_4_3_Core>

#include <QOpenGLShaderProgram>

#include "vbocube.h"
#include "teapot.h"
#include "vboplane.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ToRadian(x) ((x) * M_PI / 180.0f)
#define ToDegree(x) ((x) * 180.0f / M_PI)
#define TwoPI (float)(2 * M_PI)

//class MyWindow : public QWindow, protected QOpenGLFunctions_3_3_Core
class MyWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit MyWindow();
    ~MyWindow();
    virtual void keyPressEvent( QKeyEvent *keyEvent );    

private slots:
    void render();

private:    
    void initialize();
    void modCurTime();

    void initShaders();
    void CreateVertexBuffer();
    void setupFBO();
    void initMatrices();
    void calcProjectorMatrix();

    void PrepareTexture(GLenum TextureUnit, GLenum TextureTarget, const QString& FileName, bool flip);

    void renderToTexture();
    void renderScene();       

protected:
    void resizeEvent(QResizeEvent *);

private:
    QOpenGLContext *mContext;
    QOpenGLFunctions_4_3_Core *mFuncs;

    QOpenGLShaderProgram *mProgramFromTex, *mProgramToTex;

    QTimer mRepaintTimer;
    double currentTimeMs;
    double currentTimeS;
    bool   mUpdateSize;
    float  tPrev, angle, rotSpeed;

    GLuint mVAOCube, mVAOTeapot, mVAOPlane, mVBO, mIBO, mFBO;

    VBOCube  *mCube;
    Teapot   *mTeapot;
    VBOPlane *mPlane;
    QMatrix4x4 ModelMatrixCube, ModelMatrixTeapot, ModelMatrixPlane, ViewMatrix, ProjectionMatrix;
    QMatrix4x4 mProjectorMatrix;
    QVector3D  mProjLookat;

    //debug
    void printMatrix(const QMatrix4x4& mat);
};
