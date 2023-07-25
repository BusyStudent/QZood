#include "fullSettingWidget.hpp"
#include "ui_fullSettingView.h"

#include "videoWidget.hpp"

#include <QFileDialog>
#include <QColorDialog>

#include "../../common/myGlobalLog.hpp"

class FullSettingWidgetPrivate {
public:
    FullSettingWidgetPrivate(FullSettingWidget *parent) : ui(new Ui::FullSettingView()), self(parent) {}
    ~FullSettingWidgetPrivate() { }

    void setupUi() {
        ui->setupUi(self);
        // ====================播放列表====================
        ui->settingStackWidget->setCurrentIndex(0);
        ui->settingList->setCurrentRow(0);
        ui->settingList->setStyleSheet(R"(
            QListWidget#settingList{
                border: 1px solid #202020;
                background-color:  #202020;
                outline: 0px;
                padding-right: -1px;
                padding-left: -1px;
                padding-top: 8px;
            }
            QListWidget#settingList::item:hover{
                border: 1px solid #66b2ff;
                background-color: #66b2ff;
            }
            QListWidget#settingList::item {
                border: 1px solid #202020;
                color: white;	
                padding-top: 10px;
                padding-bottom: 10px;
            }
            QListWidget#settingList::item:selected{
                background-color: #66b2ff;
                show-decoration-selected: 0;
                margin: -1px  -2px;
                padding-left: 22px;
            })");

        // ====================播放设置====================
        // 播放速度去value / 10倍。
        ui->playbackRateBar->setRange(5, 30);
    }

    void connect(VideoWidget* videoWidget) {
        connectPlaySetting(videoWidget);
        connectScreenSetting(videoWidget);
        connectColorSetting(videoWidget);
        connectDanmakuSetting(videoWidget);
        connectSubtitleSetting(videoWidget);

        resetPlaySetting();
        resetScreenSetting();
        resetColorSetting();
        resetDanmakuSetting();
        resetSubtitleSetting();
    }

    void update(VideoWidget* videoWidget) {
        switch(videoWidget->skipStep()) {
            case 5:
                ui->step5Button->setChecked(true);
                break;
            case 10:
                ui->step10Button->setChecked(true);
                break;
            case 30:
                ui->step30Button->setChecked(true);
                break;
        }
    }

private:
    void connectPlaySetting(VideoWidget* videoWidget) {
        // 播放条显示的值
        QWidget::connect(ui->playbackRateBar, &QSlider::valueChanged, self, [this, videoWidget](int value){
            ui->playbackRateBar->setToolTip(QString::number(value / 10.0, 'g', 2));
            videoWidget->setPlaybackRate(value / 10.0);
        });
        // 连接播放器到
        QWidget::connect(ui->playbackRate05, &QPushButton::clicked, self, [this](bool checked) {
            ui->playbackRateBar->setValue(5);
        });
        QWidget::connect(ui->playbackRate1, &QPushButton::clicked, self, [this](bool chlicked){
            ui->playbackRateBar->setValue(10);
        });
        QWidget::connect(ui->playbackRate2, &QPushButton::clicked, self, [this](bool chlicked){
            ui->playbackRateBar->setValue(20);
        });
        QWidget::connect(ui->playbackRate3, &QPushButton::clicked, self, [this](bool chlicked){
            ui->playbackRateBar->setValue(30);
        });
        QWidget::connect(ui->step5Button, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->setSkipStep(5);
        });
        QWidget::connect(ui->step10Button, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->setSkipStep(10);
        });
        QWidget::connect(ui->step30Button, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->setSkipStep(30);
        });
    }

    void connectScreenSetting(VideoWidget* videoWidget) {
        // 播放比例
        QWidget::connect(ui->defaultAspectRatioButton, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->videoCanvas()->setAspectMode(VideoCanvas::KeepAspect);
        });
        QWidget::connect(ui->aspectRation43Button, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->videoCanvas()->setAspectMode(VideoCanvas::_4x3);
        });
        QWidget::connect(ui->aspectRation169Button, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->videoCanvas()->setAspectMode(VideoCanvas::_16x9);
        });
        QWidget::connect(ui->aspectRationFullButton, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->videoCanvas()->setAspectMode(VideoCanvas::Filling);
        });
        // 画面旋转
        QWidget::connect(ui->clockwiseRotationButton, &QPushButton::clicked, videoWidget, [videoWidget](bool clicked) {
            // TODO(llhsdmd) : RotationScreen
            qInfo() << "TODO(RotationScreen)";
        });
        QWidget::connect(ui->anticlockwiseRotationButton, &QPushButton::clicked, videoWidget, [videoWidget](bool clicked) {
            // TODO(llhsdmd) : RotationScreen
            qInfo() << "TODO(RotationScreen)";
        });
        QWidget::connect(ui->horizontalFilpButton, &QPushButton::clicked, videoWidget, [videoWidget](bool clicked) {
            // TODO(llhsdmd) : RotationScreen
            qInfo() << "TODO(RotationScreen)";
        });
        QWidget::connect(ui->verticallyFilpButton, &QPushButton::clicked, videoWidget, [videoWidget](bool clicked) {
            // TODO(llhsdmd) : RotationScreen
            qInfo() << "TODO(RotationScreen)";
        });
        // 画质增强
        QWidget::connect(ui->imageQualityEnhancementButton, &QCheckBox::stateChanged, videoWidget, [videoWidget](int status) {
            // TODO(llhsdmd) : setImageQualityEnhancement
            qInfo() << "TODO(setImageQualityEnhancement)";
        });
    }

    void connectColorSetting(VideoWidget* videoWidget) {
        // 色彩设置
        QWidget::connect(ui->brightnessBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            // TODO(llhsdmd) : setBrightness
            qInfo() << "TODO(setBrightness)";
            ui->brightnessLabel->setNum(value);
        });
        QWidget::connect(ui->contrastBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            // TODO(llhsdmd) : setContrast
            qInfo() << "TODO(setContrast)";
            ui->contrastLabel->setNum(value);
        });
        QWidget::connect(ui->hueBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            // TODO(llhsdmd) : setHue
            qInfo() << "TODO(setHue)";
            ui->hueLabel->setNum(value);
        });
        QWidget::connect(ui->saturationBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            // TODO(llhsdmd) : setSaturation
            qInfo() << "TODO(setSaturation)";
            ui->saturationLabel->setNum(value);
        });
    }

    void connectDanmakuSetting(VideoWidget* videoWidget) {
        // 弹幕选择
        ui->danmakuFontComboBox->setCurrentFont(videoWidget->videoCanvas()->danmakuFont());

        QWidget::connect(ui->danmakuCombobox, &QComboBox::currentTextChanged, videoWidget, [videoWidget](const QString &danmakuSource) {
            videoWidget->currentVideo()->setCurrentDanmakuSource(danmakuSource);
            MDebug(MyDebug::INFO) << "set danmakuSource : " << danmakuSource;
            videoWidget->currentVideo()->loadDanmaku(videoWidget, [videoWidget](const Result<DanmakuList>& danmakulist) {
                if (danmakulist.has_value()) {
                    videoWidget->videoCanvas()->setDanmakuList(danmakulist.value());
                } else {
                    MDebug(MyDebug::INFO) << "danmakuSource set failed!";
                }
            });
        });
        QWidget::connect(ui->loadDanmakuButton, &QToolButton::clicked, videoWidget, [videoWidget](bool clicked){
            auto filePath = QFileDialog::getOpenFileName(videoWidget, videoWidget->tr("请选择弹幕文件"), "./", ".*");
            if (!filePath.isEmpty()) {
                videoWidget->currentVideo()->loadDanmakuFromFile(filePath);
                videoWidget->currentVideo()->setCurrentDanmakuSource(filePath);
                MDebug(MyDebug::WARNING) << "TODO(BusyStudent): support loacl danmaku file.";
            }
        });
        QWidget::connect(ui->danmakuShowAreaBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->videoCanvas()->setDanmakuTracksLimit((qreal)value / 100.0);
            ui->danmakuShowAreaLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuSizeBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            QFont font = videoWidget->videoCanvas()->danmakuFont();
            font.setPixelSize(font.pixelSize() * (qreal)value / 100.0);
            videoWidget->videoCanvas()->setDanmakuFont(font);
            ui->danmakuSizeLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuSpeedBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            // TODO(llhsdmd) : setDanmakuSpeed
            qInfo() << "TODO(setDanmakuSpeed)";
            static QStringList list{"极慢", "较慢", "正常", "较快", "极快"};
            ui->danmakuSpeedLabel->setText(list[value]);
        });
        QWidget::connect(ui->danmakuFontComboBox, &QFontComboBox::currentFontChanged, videoWidget, [videoWidget, this](const QFont &f) {
            auto font = videoWidget->videoCanvas()->danmakuFont();
            font.setFamilies(f.families());
            videoWidget->videoCanvas()->setDanmakuFont(font);
        });
        QWidget::connect(ui->danmakuBoldCheckBox, &QCheckBox::stateChanged, videoWidget, [videoWidget, this](int checked) {
            auto font = videoWidget->videoCanvas()->danmakuFont();
            font.setBold(checked == Qt::Checked);
            videoWidget->videoCanvas()->setDanmakuFont(font);
        });
        QWidget::connect(ui->danmakuTransparencyBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->videoCanvas()->setDanmakuOpacity(1 - value / 100.0);
            ui->danmakuTransparencyLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuStroke, &QCheckBox::stateChanged, videoWidget, [videoWidget, this](int state) {
            if (state == Qt::Checked) {
                videoWidget->videoCanvas()->setDanmakuShadowMode(VideoCanvas::Outline);
            } else {
                MDebug(MyDebug::WARNING) << "TODO(BusyStudent): off danmaku stroke.";
            }
        });
        QWidget::connect(ui->danmakuProject45degree, &QCheckBox::stateChanged, videoWidget, [videoWidget, this](int state) {
            if (state == Qt::Checked) {
                videoWidget->videoCanvas()->setDanmakuShadowMode(VideoCanvas::ShadowMode::Projection);
            } else {
                MDebug(MyDebug::WARNING) << "TODO(BusyStudent): off danmaku stroke.";
            }
        });
    }

    void connectSubtitleSetting(VideoWidget *videoWidget) {
        // 选择字幕
        ui->subtitleFontComboBox->setCurrentFont(videoWidget->videoCanvas()->subtitleFont());
        ui->subtitleSizeBox->setValue(videoWidget->videoCanvas()->subtitleFont().pixelSize());

        QWidget::connect(ui->subtitleComboBox, &QComboBox::currentTextChanged, videoWidget, [videoWidget](const QString& text) {
            int index = videoWidget->currentVideo()->subtitleSourceList().indexOf(text) - 1;
            if (index >= 0 && index < videoWidget->player()->subtitleTracks().size()) {
                videoWidget->currentVideo()->setCurrentSubtitleSource(text);
                videoWidget->player()->setActiveSubtitleTrack(index);
            }
        });
        QWidget::connect(ui->loadSubtitleButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            QString filePath = QFileDialog::getOpenFileName(videoWidget, videoWidget->tr("请选择字幕文件"), "./", ".ass;;.*");
            if (!filePath.isEmpty()) {
                videoWidget->currentVideo()->loadSubtitleFromFile(filePath);
                videoWidget->currentVideo()->setCurrentSubtitleSource(filePath);
            }
        });
        QWidget::connect(ui->subtitleSynchronizeTimeBox, &QDoubleSpinBox::valueChanged, videoWidget, [videoWidget](double value) {
            // TODO(llhsdmd) : setSubtitleSynchronizeTime
            qInfo() << "TODO(setSubtitleSynchronizeTime)";
        });
        QWidget::connect(ui->subtitleDefualtsynchronizeTimeButton, &QToolButton::clicked, videoWidget, [this](bool checked) {
            ui->subtitleSynchronizeTimeBox->setValue(0);
        });
        QWidget::connect(ui->subtitlePositionBar, &QSlider::valueChanged, videoWidget, [videoWidget](int value) {
            // TODO(llhsdmd) : setSubtitlePosition
            qInfo() << "TODO(setSubtitlePosition)";
        });
        QWidget::connect(ui->subtitleFontComboBox, &QFontComboBox::currentFontChanged, videoWidget, [videoWidget](const QFont& f) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setFamilies(f.families());
            videoWidget->videoCanvas()->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleSizeBox, &QSpinBox::valueChanged, videoWidget, [videoWidget](double v) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setPixelSize(v);
            videoWidget->videoCanvas()->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleBoldButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setBold(!checked);
            videoWidget->videoCanvas()->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleItalicsButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setItalic(!checked);
            videoWidget->videoCanvas()->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleUnderlineButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setUnderline(!checked);
            videoWidget->videoCanvas()->setFont(font);
        });
        QWidget::connect(ui->subtitleColor, &QPushButton::clicked, videoWidget, [videoWidget, this](bool checked) {
            getColor(ui->subtitleColor, [videoWidget](const QColor& color) {
                videoWidget->videoCanvas()->setSubtitleColor(color);
            });
        });
        QWidget::connect(ui->subtitleTransparencyBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            ui->subtitleTransparencyLabel->setText(QString("%1%").arg(value / 100.0));
            videoWidget->videoCanvas()->setSubtitleOpacity(1.0 - value / 100.0);
        });
        QWidget::connect(ui->subtitleStrokeCheckBox, &QCheckBox::stateChanged, videoWidget, [videoWidget, this](int checked) {
            if (checked == Qt::Checked) {
                QColor color(ui->subtitleStrokeColor->text());
                // TODO(llhsdmd) : setSubtitleStroke
                qInfo() << "TODO(setSubtitleStroke)";
                ui->subtitleStrokeColor->setEnabled(true);
                ui->subtitleStrokeTransparencyBar->setEnabled(true);
            } else {
                // TODO(llhsdmd) : setSubtitleStroke
                qInfo() << "TODO(setSubtitleStroke)";
                ui->subtitleStrokeTransparencyBar->setEnabled(false);
                ui->subtitleStrokeColor->setEnabled(false);
            }
        });
        QWidget::connect(ui->subtitleStrokeColor, &QPushButton::clicked, videoWidget, [videoWidget, this](bool checked) {
            getColor(ui->subtitleStrokeColor, [videoWidget](const QColor& color) {
                // TODO(llhsdmd) : setSubtitleStroke
                qInfo() << "TODO(setSubtitleStroke)";
                videoWidget->videoCanvas()->setSubtitleOutlineColor(color);
            });
        });
        QWidget::connect(ui->subtitleStrokeTransparencyBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            // TODO(llhsdmd) : setSubtitleStrokeTransparency
            qInfo() << "TODO(setSubtitleStrokeTransparency)";
            ui->subtitleStrokeTransparencyLabel->setText(QString("%1%").arg(value));
        });
    }

    void getColor(QPushButton* colorButton, std::function<void(const QColor& color)> func) {
        self->setHideAfterLeave(false);

        PopupWidget* colorSelectWidget = new PopupWidget(self, Qt::Popup);
        QHBoxLayout* hboxlayout = new QHBoxLayout();
        QColorDialog* colorDiralog = new QColorDialog();

        QWidget::connect(colorDiralog, &QColorDialog::colorSelected, self, [colorButton, colorSelectWidget, func, this](const QColor& color){
            setColorForButton(colorButton, color);
            colorSelectWidget->hide();
            func(color);
            self->setHideAfterLeave(true);
        });
        QWidget::connect(colorDiralog, &QColorDialog::rejected, colorSelectWidget, &PopupWidget::hide);
        QWidget::connect(colorSelectWidget, &PopupWidget::hided, colorSelectWidget, [colorSelectWidget](){
            colorSelectWidget->deleteLater();
        });

        hboxlayout->addWidget(colorDiralog);
        hboxlayout->setContentsMargins(0, 0, 0, 0);
        colorSelectWidget->setLayout(hboxlayout);
        colorSelectWidget->setAssociateWidget(colorButton);
        colorSelectWidget->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        colorSelectWidget->setAuotLayout();
        colorSelectWidget->show();
    }
    
    void setColorForButton(QPushButton* colorButton,const QColor &color) {
        colorButton->setText(color.name());
        colorButton->setStyleSheet("QPushButton{background-color: " + color.name() + ";" 
        + "color: " + color.darker().name() + ";"
        + R"(border: 1px dashed white;})");
    }

    void resetPlaySetting() {
        ui->step10Button->click();
        ui->playbackRate1->click();
    }

    void resetScreenSetting() {
        ui->defaultAspectRatioButton->click();
        ui->imageQualityEnhancementButton->setCheckState(Qt::CheckState::Unchecked);
        ui->imageQualityEnhancementButton->stateChanged(Qt::CheckState::Unchecked);
    }

    void resetColorSetting() {
        ui->brightnessBar->setValue(0);
        ui->contrastBar->setValue(0);
        ui->hueBar->setValue(0);
        ui->saturationBar->setValue(0);
    }

    void resetDanmakuSetting() {
        ui->danmakuShowAreaBar->setValue(100);
        ui->danmakuSizeBar->setValue(80);
        ui->danmakuSpeedBar->setValue(2);
        ui->danmakuBoldCheckBox->setCheckState(Qt::CheckState::Unchecked);
        ui->danmakuBoldCheckBox->stateChanged(Qt::CheckState::Unchecked);
        ui->danmakuTransparencyBar->setValue(0);
        ui->danmakuProject45degree->setCheckState(Qt::CheckState::Unchecked);
        ui->danmakuStroke->setCheckState(Qt::CheckState::Unchecked);
    }

    void resetSubtitleSetting() {
        ui->subtitleDefualtsynchronizeTimeButton->click();
        ui->subtitlePositionBar->setValue(20);
        ui->subtitleSizeBox->setValue(40);
        ui->subtitleBoldButton->setChecked(false);
        ui->subtitleItalicsButton->setChecked(false);
        ui->subtitleUnderlineButton->setChecked(false);
        ui->subtitleTransparencyBar->setValue(0);
        ui->subtitleStrokeCheckBox->setCheckState(Qt::CheckState::Unchecked);
        ui->subtitleStrokeCheckBox->stateChanged(Qt::CheckState::Unchecked);
        setColorForButton(ui->subtitleStrokeColor, Qt::white);
        setColorForButton(ui->subtitleColor, Qt::black);
        ui->subtitleStrokeTransparencyBar->setValue(0);
    }

public:
    QScopedPointer<Ui::FullSettingView> ui;

private:
    FullSettingWidget *self;
};

FullSettingWidget::FullSettingWidget(QWidget* parent, Qt::WindowFlags f) : PopupWidget(parent, f), d(new FullSettingWidgetPrivate(this)) {
    d->setupUi();
}

FullSettingWidget::~FullSettingWidget() { }

void FullSettingWidget::initDanmakuSetting(VideoBLLPtr video) {
    d->ui->danmakuCombobox->addItems(video->danmakuSourceList());
}

void FullSettingWidget::initSubtitleSetting(VideoBLLPtr video) {
    d->ui->subtitleComboBox->clear();
    d->ui->subtitleComboBox->addItems(video->subtitleSourceList());
}

void FullSettingWidget::setupSetting(VideoWidget *videoWidget) {
    d->connect(videoWidget);
}

void FullSettingWidget::show() {
    PopupWidget::show();
}