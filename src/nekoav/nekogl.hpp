#pragma once

#include <QOpenGLWidget>
#include "nekoav.hpp"

namespace NekoAV {

class VideoSink;
class OpenGLRendererPrivate;

/**
 * @brief OpenGLRenderer, Render VideoFrame into screen
 * 
 */
class NEKO_API OpenGLRenderer : public QObject {
    Q_OBJECT
    Q_PROPERTY(VideoSink* videoSink READ videoSink CONSTANT)
    public:
        explicit OpenGLRenderer(QObject *parent);
        ~OpenGLRenderer();

        VideoSink *videoSink() const;
    public:  //< OpenGL
        /**
         * @brief Initialize OpenGL, must called at QOpenGLWidget::initializeGL
         * 
         */
        void initializeGL();
        /**
         * @brief Paint current video frame into framebuffer
         * 
         */
        void paintGL();
        void resizeGL(int w, int h);
    private:
        QScopedPointer<OpenGLRendererPrivate> d;
};
class NEKO_API OpenGLVideoWidget : public QOpenGLWidget {

};

}

NEKO_USING(OpenGLVideoWidget);
NEKO_USING(OpenGLRenderer);
