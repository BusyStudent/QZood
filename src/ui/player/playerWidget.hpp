#pragma once

#include "../util/widget/customizeTitleWidget.hpp"
#include "../../BLL/data/videoBLL.hpp"

class PlayerWidgetPrivate;

class PlayerWidget final : public CustomizeTitleWidget {
    Q_OBJECT
    public:
        PlayerWidget(QWidget* parent = nullptr);
        virtual ~PlayerWidget();
        void setVideoList(VideoBLLList videos);
        void setAutoPlay(bool f);
        void appendVideo(VideoBLLPtr video);
        void clearVideoList();
        void setTitle(const QString &title);

        bool isInTitleBar(const QPoint &pos) override;

    public:
        void resizeEvent(QResizeEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        bool eventFilter(QObject* obj,QEvent* event) override;

        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void showEvent(QShowEvent* event) override;

    private:
        QScopedPointer<PlayerWidgetPrivate> d;
};