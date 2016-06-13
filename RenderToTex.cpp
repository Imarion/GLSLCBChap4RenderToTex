#include "RenderToTex.h"

#include <QtGlobal>

#include <QDebug>
#include <QFile>
#include <QImage>
#include <QTime>

#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>

#include <cmath>
#include <cstring>
#include <iostream>

MyWindow::~MyWindow()
{
    if (mProgramFromTex != 0) delete mProgramFromTex;
    if (mProgramToTex   != 0) delete mProgramToTex;
}

MyWindow::MyWindow()
    : mProgramFromTex(0), mProgramToTex(0), currentTimeMs(0), currentTimeS(0), tPrev(0), angle(2.44346), rotSpeed(M_PI / 8.0f)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    create();

    resize(800, 600);

    mContext = new QOpenGLContext(this);
    mContext->setFormat(format);
    mContext->create();

    mContext->makeCurrent( this );

    mFuncs = mContext->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if ( !mFuncs )
    {
        qWarning( "Could not obtain OpenGL versions object" );
        exit( 1 );
    }
    if (mFuncs->initializeOpenGLFunctions() == GL_FALSE)
    {
        qWarning( "Could not initialize core open GL functions" );
        exit( 1 );
    }

    initializeOpenGLFunctions();

    QTimer *repaintTimer = new QTimer(this);
    connect(repaintTimer, &QTimer::timeout, this, &MyWindow::render);
    repaintTimer->start(1000/60);

    QTimer *elapsedTimer = new QTimer(this);
    connect(elapsedTimer, &QTimer::timeout, this, &MyWindow::modCurTime);
    elapsedTimer->start(1);       
}

void MyWindow::modCurTime()
{
    currentTimeMs++;
    currentTimeS=currentTimeMs/1000.0f;
}

void MyWindow::initialize()
{
    CreateVertexBuffer();
    initShaders();
    initMatrices();

    PrepareTexture(GL_TEXTURE2, GL_TEXTURE_2D, "../Media/flower.png", true);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // One pixel white texture
    GLuint whiteTexHandle;
    GLubyte whiteTex[] = { 255, 255, 255, 255 };
    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &whiteTexHandle);
    glBindTexture(GL_TEXTURE_2D,whiteTexHandle);
    mFuncs->glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
    mFuncs->glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,whiteTex);

    setupFBO();

    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
}

void MyWindow::CreateVertexBuffer()
{
    // *** Cube
    mFuncs->glGenVertexArrays(1, &mVAOCube);
    mFuncs->glBindVertexArray(mVAOCube);

    mCube = new VBOCube();

    // Create and populate the buffer objects
    unsigned int CubeHandles[4];
    glGenBuffers(4, CubeHandles);

    glBindBuffer(GL_ARRAY_BUFFER, CubeHandles[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mCube->getnVerts()) * sizeof(float), mCube->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, CubeHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mCube->getnVerts()) * sizeof(float), mCube->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, CubeHandles[2]);
    glBufferData(GL_ARRAY_BUFFER, (2 * mCube->getnVerts()) * sizeof(float), mCube->gettc(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CubeHandles[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mCube->getnFaces() * sizeof(unsigned int), mCube->getel(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, CubeHandles[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, CubeHandles[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Vertex texure coordinates
    mFuncs->glBindVertexBuffer(2, CubeHandles[2], 0, sizeof(GLfloat) * 2);
    mFuncs->glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(2, 2);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CubeHandles[3]);

    mFuncs->glBindVertexArray(0);


    // *** Teapot
    mFuncs->glGenVertexArrays(1, &mVAOTeapot);
    mFuncs->glBindVertexArray(mVAOTeapot);

    QMatrix4x4 transform;
    //transform.translate(QVector3D(0.0f, 1.5f, 0.25f));
    mTeapot = new Teapot(14, transform);

    // Create and populate the buffer objects
    unsigned int TeapotHandles[4];
    glGenBuffers(4, TeapotHandles);

    glBindBuffer(GL_ARRAY_BUFFER, TeapotHandles[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mTeapot->getnVerts()) * sizeof(float), mTeapot->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, TeapotHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mTeapot->getnVerts()) * sizeof(float), mTeapot->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, TeapotHandles[2]);
    glBufferData(GL_ARRAY_BUFFER, (2 * mTeapot->getnVerts()) * sizeof(float), mTeapot->gettc(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TeapotHandles[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mTeapot->getnFaces() * sizeof(unsigned int), mTeapot->getelems(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, TeapotHandles[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, TeapotHandles[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Vertex texure coordinates
    mFuncs->glBindVertexBuffer(2, TeapotHandles[2], 0, sizeof(GLfloat) * 2);
    mFuncs->glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(2, 2);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TeapotHandles[3]);

    mFuncs->glBindVertexArray(0);

    // *** Plane
    mFuncs->glGenVertexArrays(1, &mVAOPlane);
    mFuncs->glBindVertexArray(mVAOPlane);

    mPlane = new VBOPlane(100.0f, 100.0f, 1, 1);

    // Create and populate the buffer objects
    unsigned int PlaneHandles[3];
    glGenBuffers(3, PlaneHandles);

    glBindBuffer(GL_ARRAY_BUFFER, PlaneHandles[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mPlane->getnVerts()) * sizeof(float), mPlane->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, PlaneHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mPlane->getnVerts()) * sizeof(float), mPlane->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PlaneHandles[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mPlane->getnFaces() * sizeof(unsigned int), mPlane->getelems(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, PlaneHandles[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, PlaneHandles[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PlaneHandles[2]);

    mFuncs->glBindVertexArray(0);
}

void MyWindow::setupFBO() {

    // Generate and bind the framebuffer
    glGenFramebuffers(1, &mFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

    // Create the texture object
    GLuint renderTex;
    glGenTextures(1, &renderTex);
    glActiveTexture(GL_TEXTURE0);  // Use texture unit 0
    glBindTexture(GL_TEXTURE_2D, renderTex);
    mFuncs->glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 512, 512);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Bind the texture to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);

    // Create the depth buffer
    GLuint depthBuf;
    glGenRenderbuffers(1, &depthBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 512, 512);

    // Bind the depth buffer to the FBO
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthBuf);

    // Set the targets for the fragment output variables
    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
    mFuncs->glDrawBuffers(1, drawBuffers);

    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( result == GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is complete" << std::endl;
    } else {
        std::cout << "Framebuffer error: " << result << std::endl;
    }

    // Unbind the framebuffer, and revert to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void MyWindow::initMatrices()
{
    //ViewMatrix.lookAt(QVector3D(1.0f, 1.25f, 1.25f), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f));
    ModelMatrixTeapot.translate(0.0f -1.5f, 0.0f);
    ModelMatrixTeapot.rotate(-90.0f, 1.0f, 0.0f, 0.0f);

    ModelMatrixPlane.translate(0.0f, -0.75f, 0.0f);

    mProjLookat = QVector3D(-2.0f,-4.0f,0.0f);

    calcProjectorMatrix();
}

void MyWindow::calcProjectorMatrix()
{
    QVector3D projPos(2.0f,5.0f,5.0f);
    QVector3D projUp(0.0f,1.0f,0.0f);

    QMatrix4x4 projView;
    projView.lookAt(projPos, mProjLookat, projUp);
    QMatrix4x4 projProj;
    projProj.perspective(30.0f, 1.0f, 0.2f, 1000.0f);
    QMatrix4x4 projTrans;
    QMatrix4x4 projScale;
    projTrans.translate(0.5f, 0.5f, 0.5f);
    projScale.scale(0.5f);

    mProjectorMatrix = projTrans * projScale * projProj * projView;
}

void MyWindow::resizeEvent(QResizeEvent *)
{
    mUpdateSize = true;

    ProjectionMatrix.setToIdentity();
    ProjectionMatrix.perspective(60.0f, (float)this->width()/(float)this->height(), 0.3f, 100.0f);
}

void MyWindow::render()
{

    if(!isVisible() || !isExposed())
        return;

    if (!mContext->makeCurrent(this))
        return;

    static bool initialized = false;
    if (!initialized) {
        initialize();
        initialized = true;
    }

    if (mUpdateSize) {
        glViewport(0, 0, size().width(), size().height());
        mUpdateSize = false;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float deltaT = currentTimeS - tPrev;
    if(tPrev == 0.0f) deltaT = 0.0f;
    tPrev = currentTimeS;
    angle += rotSpeed * deltaT;
    if (angle > TwoPI) angle -= TwoPI;

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    renderToTexture();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    renderScene();
}

void MyWindow::renderToTexture()
{
    glViewport(0, 0, 512, 512);

    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ViewMatrix.setToIdentity();
    QVector3D cameraPos = QVector3D( 7.0f * cos(angle), 2.0f, 7.0f * sin(angle));
    ViewMatrix.lookAt(cameraPos, QVector3D(0.0f,0.0f,0.0f), QVector3D(0.0f,1.0f,0.0f));

    calcProjectorMatrix();

    //QMatrix4x4 RotationMatrix;
    //RotationMatrix.rotate(EvolvingVal, QVector3D(0.1f, 0.0f, 0.1f));
    //ModelMatrix.rotate(0.3f, QVector3D(0.1f, 0.0f, 0.1f));
    QVector4D worldLight = QVector4D(0.0f, 0.0f, 0.0f, 1.0f);

    // *** Draw teapot
    mFuncs->glBindVertexArray(mVAOTeapot);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    mProgramToTex->bind();
    {
        mProgramToTex->setUniformValue("ProjectorTex", 2);

        mProgramToTex->setUniformValue("Material.Kd", 0.5f, 0.2f, 0.1f);
        mProgramToTex->setUniformValue("Material.Ks", 0.95f, 0.95f, 0.95f);
        mProgramToTex->setUniformValue("Material.Ka", 0.1f, 0.1f, 0.1f);
        mProgramToTex->setUniformValue("Material.Shininess", 100.0f);

        mProgramToTex->setUniformValue("WorldCameraPosition", cameraPos);

        QMatrix4x4 mv = ViewMatrix * ModelMatrixTeapot;
        mProgramToTex->setUniformValue("ModelMatrix", ModelMatrixTeapot);
        mProgramToTex->setUniformValue("ModelViewMatrix", mv);
        mProgramToTex->setUniformValue("NormalMatrix", mv.normalMatrix());
        mProgramToTex->setUniformValue("MVP", ProjectionMatrix * mv);

        mProgramToTex->setUniformValue("ProjectorMatrix", mProjectorMatrix);

        mProgramToTex->setUniformValue("Light.Position", QVector4D(0.0f,0.0f,0.0f,1.0f) );
        mProgramToTex->setUniformValue("Light.Intensity", QVector3D(1.0f,1.0f,1.0f));

        glDrawElements(GL_TRIANGLES, 6 * mTeapot->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
    mProgramToTex->release();

    // *** Draw plane
    mFuncs->glBindVertexArray(mVAOPlane);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    mProgramToTex->bind();
    {
        mProgramToTex->setUniformValue("ProjectorTex", 2);

        mProgramToTex->setUniformValue("Material.Kd", 0.4f, 0.4f, 0.4f);
        mProgramToTex->setUniformValue("Material.Ks", 0.0f, 0.0f, 0.0f);
        mProgramToTex->setUniformValue("Material.Ka", 0.1f, 0.1f, 0.1f);
        mProgramToTex->setUniformValue("Material.Shininess", 1.0f);

        mProgramToTex->setUniformValue("WorldCameraPosition", cameraPos);

        QMatrix4x4 mv = ViewMatrix * ModelMatrixPlane;
        mProgramToTex->setUniformValue("ModelMatrix", ModelMatrixPlane);
        mProgramToTex->setUniformValue("ModelViewMatrix", mv);
        mProgramToTex->setUniformValue("NormalMatrix", mv.normalMatrix());
        mProgramToTex->setUniformValue("MVP", ProjectionMatrix * mv);

        mProgramToTex->setUniformValue("ProjectorMatrix", mProjectorMatrix);

        mProgramToTex->setUniformValue("Light.Position", QVector4D(0.0f,0.0f,0.0f,1.0f) );
        mProgramToTex->setUniformValue("Light.Intensity", QVector3D(1.0f,1.0f,1.0f));

        glDrawElements(GL_TRIANGLES, 6 * mPlane->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
    mProgramToTex->release();
}


void MyWindow::renderScene()
{
    glViewport(0, 0, size().width(), size().height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //QMatrix4x4 RotationMatrix;
    //RotationMatrix.rotate(EvolvingVal, QVector3D(0.1f, 0.0f, 0.1f));
    //ModelMatrix.rotate(0.3f, QVector3D(0.1f, 0.0f, 0.1f));
    QVector4D worldLight = QVector4D(0.0f, 0.0f, 0.0f, 1.0f);

    QVector3D cameraPos = QVector3D(2.0f * cos(angle), 1.5f, 2.0f * sin(angle));
    ViewMatrix.setToIdentity();
    ViewMatrix.lookAt(cameraPos, QVector3D(0.0f,0.0f,0.0f), QVector3D(0.0f,1.0f,0.0f));

    mFuncs->glBindVertexArray(mVAOCube);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    mProgramFromTex->bind();
    {
        mProgramFromTex->setUniformValue("RenderTex", 0);

        mProgramFromTex->setUniformValue("Light.Position",  worldLight );
        mProgramFromTex->setUniformValue("Light.Intensity", QVector3D(1.0f, 1.0f, 1.0f));

        mProgramFromTex->setUniformValue("Material.Kd", 0.9f, 0.9f, 0.9f);
        mProgramFromTex->setUniformValue("Material.Ks", 0.0f, 0.0f, 0.0f);
        mProgramFromTex->setUniformValue("Material.Ka", 0.1f, 0.1f, 0.1f);
        mProgramFromTex->setUniformValue("Material.Shininess", 1.0f);

        QMatrix4x4 projection;
        projection.perspective(45.0f, (float)size().width()/(float)size().height(), 0.3f, 100.0f);

        QMatrix4x4 mv1 = ViewMatrix * ModelMatrixCube;
        mProgramFromTex->setUniformValue("ModelViewMatrix", mv1);
        mProgramFromTex->setUniformValue("NormalMatrix", mv1.normalMatrix());
        mProgramFromTex->setUniformValue("MVP", projection * mv1);


        glDrawElements(GL_TRIANGLES, 6 * mCube->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
    mProgramFromTex->release();

    mContext->swapBuffers(this);
}

void MyWindow::initShaders()
{
    QOpenGLShader vShader(QOpenGLShader::Vertex);
    QOpenGLShader fShader(QOpenGLShader::Fragment);    
    QFile         shaderFile;
    QByteArray    shaderSource;

    // From texture shader
    shaderFile.setFileName(":/vshader_fromtexture.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "vert from tex compile: " << vShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/fshader_fromtexture.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "frag from tex compile: " << fShader.compileSourceCode(shaderSource);

    mProgramFromTex = new (QOpenGLShaderProgram);
    mProgramFromTex->addShader(&vShader);
    mProgramFromTex->addShader(&fShader);
    qDebug() << "shader from tex link: " << mProgramFromTex->link();

    // To texture shader
    shaderFile.setFileName(":/vshader_totexture.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "vert to tex compile: " << vShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/fshader_totexture.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "frag to tex compile: " << fShader.compileSourceCode(shaderSource);

    mProgramToTex = new (QOpenGLShaderProgram);
    mProgramToTex->addShader(&vShader);
    mProgramToTex->addShader(&fShader);
    qDebug() << "shader to tex link: " << mProgramToTex->link();

}

void MyWindow::PrepareTexture(GLenum TextureUnit, GLenum TextureTarget, const QString& FileName, bool flip)
{
    QImage TexImg;

    if (!TexImg.load(FileName)) qDebug() << "Erreur chargement texture " << FileName;
    if (flip==true) TexImg=TexImg.mirrored();

    glActiveTexture(TextureUnit);
    GLuint TexObject;
    glGenTextures(1, &TexObject);
    glBindTexture(TextureTarget, TexObject);
    mFuncs->glTexStorage2D(TextureTarget, 1, GL_RGB8, TexImg.width(), TexImg.height());
    mFuncs->glTexSubImage2D(TextureTarget, 0, 0, 0, TexImg.width(), TexImg.height(), GL_BGRA, GL_UNSIGNED_BYTE, TexImg.bits());
    //glTexImage2D(TextureTarget, 0, GL_RGB, TexImg.width(), TexImg.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, TexImg.bits());
    glTexParameteri(TextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(TextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void MyWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch(keyEvent->key())
    {
        case Qt::Key_P:
            break;
        case Qt::Key_Up:
            mProjLookat.setY(mProjLookat.y() + 0.1f);
            break;
        case Qt::Key_Down:
            mProjLookat.setY(mProjLookat.y() - 0.1f);
            break;
        case Qt::Key_Left:
            mProjLookat.setX(mProjLookat.x() - 0.1f);
            break;
        case Qt::Key_Right:
            mProjLookat.setX(mProjLookat.x() + 0.1f);
            break;
        case Qt::Key_Delete:
            break;
        case Qt::Key_PageDown:
            break;
        case Qt::Key_Home:
            break;
        case Qt::Key_Z:
            break;
        case Qt::Key_Q:
            break;
        case Qt::Key_S:
            break;
        case Qt::Key_D:
            break;
        case Qt::Key_A:
            break;
        case Qt::Key_E:
            break;
        default:
            break;
    }
}

void MyWindow::printMatrix(const QMatrix4x4& mat)
{
    const float *locMat = mat.transposed().constData();

    for (int i=0; i<4; i++)
    {
        qDebug() << locMat[i*4] << " " << locMat[i*4+1] << " " << locMat[i*4+2] << " " << locMat[i*4+3];
    }
}
