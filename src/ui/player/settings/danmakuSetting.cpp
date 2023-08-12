#include "danmakuSetting.hpp"

#include "../videoWidget.hpp"
#include "../../../common/myGlobalLog.hpp"
#include "ui_danmakuSetting.h"

#include <QFileDialog>

class DanmakuSettingPrivate {
public:
    DanmakuSettingPrivate(DanmakuSetting *self) : self(self), ui(new Ui::DanmakuSetting()) {
        ui->setupUi(self);
    }

    void setup(VideoWidget *videoWidget) {
        this->videoWidget = videoWidget;
        connectToVideoWidget();
    }
    void reset() {
        ui->danmakuShowAreaBar->setValue(50);
        ui->danmakuSizeBar->setValue(80);
        ui->danmakuSpeedBar->setValue(2);
        ui->danmakuBoldCheckBox->setCheckState(Qt::CheckState::Unchecked);
        ui->danmakuBoldCheckBox->stateChanged(Qt::CheckState::Unchecked);
        ui->danmakuTransparencyBar->setValue(0);
        ui->danmakuProject45degree->setCheckState(Qt::CheckState::Unchecked);
        ui->danmakuStroke->setCheckState(Qt::CheckState::Unchecked);
        ui->danmakuCombobox->clear();
        if (nullptr != videoWidget) {
            auto video = videoWidget->currentVideo();
            if (nullptr != video) {
                video->setCurrentDanmakuSource("");
                ui->danmakuCombobox->addItems(video->danmakuSourceList());
            }
        }
    }
    void refresh() {
        if (nullptr != videoWidget) {
            auto video = videoWidget->currentVideo();
            if (nullptr != video) {
                int danmakuShowArea = video->getStatus<int>("danmakuShowArea").value_or(50);
                ui->danmakuShowAreaBar->setValue(danmakuShowArea);
                
                int danmakuSize = video->getStatus<int>("danmakuSize").value_or(80);
                ui->danmakuSizeBar->setValue(danmakuSize);
                
                int danmakuSpeed = video->getStatus<int>("danmakuSpeed").value_or(2);
                ui->danmakuSpeedBar->setValue(danmakuSpeed);
                
                auto danmakuBold = video->getStatus<bool>("danmakuBold");
                Qt::CheckState boldSetting = Qt::CheckState::Unchecked;
                if (danmakuBold.has_value()){
                    boldSetting = (danmakuBold.value() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
                }
                ui->danmakuBoldCheckBox->setCheckState(boldSetting);
                ui->danmakuBoldCheckBox->stateChanged(boldSetting);
                
                int danmakuTransparency = video->getStatus<int>("danmakuTransparency").value_or(0);
                ui->danmakuTransparencyBar->setValue(danmakuTransparency);
                
                auto danmakuProject45degree = video->getStatus<bool>("danmakuProject45degree");
                Qt::CheckState project45degree = Qt::CheckState::Unchecked;
                if (danmakuProject45degree.has_value()){
                    project45degree = (danmakuProject45degree.value() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
                }
                ui->danmakuProject45degree->setCheckState(project45degree);
                ui->danmakuProject45degree->stateChanged(project45degree);

                auto danmakuStroke = video->getStatus<bool>("danmakuStroke");
                Qt::CheckState stroke = Qt::CheckState::Unchecked;
                if (danmakuStroke.has_value()){
                    stroke = (danmakuStroke.value() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
                }
                ui->danmakuStroke->setCheckState(stroke);
                ui->danmakuStroke->stateChanged(stroke);

                // 弹幕源
                ui->danmakuCombobox->clear();
                ui->danmakuCombobox->addItems(video->danmakuSourceList());
            }
        }
    }

public:
    VideoWidget *videoWidget = nullptr;

private:
    void connectToVideoWidget() {
        // 弹幕选择
        ui->danmakuFontComboBox->setCurrentFont(videoWidget->videoCanvas()->danmakuFont());

        QWidget::connect(ui->danmakuCombobox, &QComboBox::currentTextChanged, videoWidget, [this](const QString &danmakuSource) {
            videoWidget->currentVideo()->setCurrentDanmakuSource(danmakuSource);
            LOG(DEBUG) << "set danmakuSource : " << danmakuSource;
            videoWidget->currentVideo()->loadDanmaku(videoWidget, [this](const Result<DanmakuList>& danmakulist) {
                if (danmakulist.has_value()) {
                    LOG(WARNING) << "load danmaku list " << danmakulist.value().size();
                    videoWidget->videoCanvas()->setDanmakuList(danmakulist.value());
                } else {
                    LOG(DEBUG) << "danmakuSource set failed!";
                }
            });
        });
        QWidget::connect(ui->loadDanmakuButton, &QToolButton::clicked, videoWidget, [this](bool clicked){
            auto filePath = QFileDialog::getOpenFileName(videoWidget, videoWidget->tr("请选择弹幕文件"), "./", ".*");
            if (!filePath.isEmpty()) {
                videoWidget->currentVideo()->loadDanmakuFromFile(filePath);
                videoWidget->currentVideo()->setCurrentDanmakuSource(filePath);
                MDebug(MyDebug::WARNING) << "TODO(BusyStudent): support loacl danmaku file.";
            }
        });
        QWidget::connect(ui->danmakuShowAreaBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            videoWidget->videoCanvas()->setDanmakuTracksLimit((qreal)value / 100.0);
            ui->danmakuShowAreaLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuSizeBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            QFont font = videoWidget->videoCanvas()->danmakuFont();
            font.setPixelSize(font.pixelSize() * (qreal)value / 100.0);
            videoWidget->videoCanvas()->setDanmakuFont(font);
            ui->danmakuSizeLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuSpeedBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            // TODO(llhsdmd) : setDanmakuSpeed
            qInfo() << "TODO(setDanmakuSpeed)";
            static QStringList list{"极慢", "较慢", "正常", "较快", "极快"};
            ui->danmakuSpeedLabel->setText(list[value]);
        });
        QWidget::connect(ui->danmakuFontComboBox, &QFontComboBox::currentFontChanged, videoWidget, [this](const QFont &f) {
            auto font = videoWidget->videoCanvas()->danmakuFont();
            font.setFamilies(f.families());
            videoWidget->videoCanvas()->setDanmakuFont(font);
        });
        QWidget::connect(ui->danmakuBoldCheckBox, &QCheckBox::stateChanged, videoWidget, [this](int checked) {
            auto font = videoWidget->videoCanvas()->danmakuFont();
            font.setBold(checked == Qt::Checked);
            videoWidget->videoCanvas()->setDanmakuFont(font);
        });
        QWidget::connect(ui->danmakuTransparencyBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            videoWidget->videoCanvas()->setDanmakuOpacity(1 - value / 100.0);
            ui->danmakuTransparencyLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuStroke, &QCheckBox::stateChanged, videoWidget, [this](int state) {
            if (state == Qt::Checked) {
                videoWidget->videoCanvas()->setDanmakuShadowMode(VideoCanvas::Outline);
            } else {
                MDebug(MyDebug::WARNING) << "TODO(BusyStudent): off danmaku stroke.";
            }
        });
        QWidget::connect(ui->danmakuProject45degree, &QCheckBox::stateChanged, videoWidget, [this](int state) {
            if (state == Qt::Checked) {
                videoWidget->videoCanvas()->setDanmakuShadowMode(VideoCanvas::ShadowMode::Projection);
            } else {
                MDebug(MyDebug::WARNING) << "TODO(BusyStudent): off danmaku stroke.";
            }
        });
    }

private:
    DanmakuSetting *self;
    QScopedPointer<Ui::DanmakuSetting> ui;
};

DanmakuSetting::DanmakuSetting(QWidget *parent) : QWidget(parent), SettingItem(), d(new DanmakuSettingPrivate(this)) {

}
DanmakuSetting::~DanmakuSetting() {

}
void DanmakuSetting::initialize(VideoWidget *videoWidget) {
    d->setup(videoWidget);
}
QString DanmakuSetting::title() {
    return "弹幕";
}
QWidget *DanmakuSetting::widget() {
    return this;
}
void DanmakuSetting::refresh() {
    d->refresh();
}
void DanmakuSetting::reset() {
    d->reset();
}