#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLShader>
#include <QOpenGLTexture>

#include "nekoav.hpp"
#include "nekogl.hpp"

namespace NekoAV {

/**
 * @brief For Mainly OpenGL draw op and data
 * 
 */
class OpenGLRendererState : public QOpenGLFunctions_3_3_Core {
    public:
        OpenGLRendererState(QOpenGLContext *context);
        ~OpenGLRendererState();
    private:
        QOpenGLContext *ctxt = nullptr;
        QOpenGLShaderProgram program; //< Current shader program
        QOpenGLBuffer        vertexBuffer;
        QOpenGLVertexArrayObject vertexArrayObject;
};
class OpenGLRendererPrivate {
    public:
        OpenGLRendererPrivate();
        ~OpenGLRendererPrivate();
        
        void initializeGL();
        void resizeGL(int w, int h);
    private:
        QScopedPointer<OpenGLRendererState> gl;
        VideoSink  sink;
};

// GLSL Parts
auto vertexShader = R"(
    #version 330 core
    
)";


// OpenGLRenderer::OpenGLRenderer(QObject *parent) : QObject(parent), d(new OpenGLRendererPrivate) {

// }
// OpenGLRenderer::~OpenGLRenderer() {

// }

}