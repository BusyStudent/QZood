#include "fullSettingWidget.hpp"
#include "ui_fullSettingView.h"

#include "videoWidget.hpp"

#include <QFileDialog>
#include <QColorDialog>

class FullSettingWidgetPrivate {
public:
    FullSettingWidgetPrivate(FullSettingWidget *parent) : ui(new Ui::FullSettingView()), self(parent) {}
    ~FullSettingWidgetPrivate() {
        delete ui;
    }

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
            videoWidget->setAspectRationMode(ScalingMode::NONE);
        });
        QWidget::connect(ui->aspectRation43Button, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->setAspectRationMode(ScalingMode::_4X3);
        });
        QWidget::connect(ui->aspectRation169Button, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->setAspectRationMode(ScalingMode::_16X9);
        });
        QWidget::connect(ui->aspectRationFullButton, &QRadioButton::clicked, videoWidget, [videoWidget](bool clicked){
            videoWidget->setAspectRationMode(ScalingMode::FILLING);
        });
        // 画面旋转
        QWidget::connect(ui->clockwiseRotationButton, &QPushButton::clicked, videoWidget, [videoWidget](bool clicked) {
            videoWidget->RotationScreen(Rotation::COLOCKWISE);
        });
        QWidget::connect(ui->anticlockwiseRotationButton, &QPushButton::clicked, videoWidget, [videoWidget](bool clicked) {
            videoWidget->RotationScreen(Rotation::ANTICLOCKWISE);
        });
        QWidget::connect(ui->horizontalFilpButton, &QPushButton::clicked, videoWidget, [videoWidget](bool clicked) {
            videoWidget->RotationScreen(Rotation::HORIZEON_FILP);
        });
        QWidget::connect(ui->verticallyFilpButton, &QPushButton::clicked, videoWidget, [videoWidget](bool clicked) {
            videoWidget->RotationScreen(Rotation::VERTICALLY_FILP);
        });
        // 画质增强
        QWidget::connect(ui->imageQualityEnhancementButton, &QCheckBox::stateChanged, videoWidget, [videoWidget](int status) {
            videoWidget->setImageQualityEnhancement(status == Qt::Checked);
        });
    }

    void connectColorSetting(VideoWidget* videoWidget) {
        // 色彩设置
        QWidget::connect(ui->brightnessBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setBrightness(value);
            ui->brightnessLabel->setNum(value);
        });
        QWidget::connect(ui->contrastBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setContrast(value);
            ui->contrastLabel->setNum(value);
        });
        QWidget::connect(ui->hueBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setHue(value);
            ui->hueLabel->setNum(value);
        });
        QWidget::connect(ui->saturationBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setSaturation(value);
            ui->saturationLabel->setNum(value);
        });
    }

    void connectDanmakuSetting(VideoWidget* videoWidget) {
        // 弹幕选择
        QWidget::connect(ui->danmakuCombobox, &QComboBox::currentTextChanged, videoWidget, [videoWidget](const QString &danmakuSource) {
            videoWidget->currentVideo()->setCurrentDanmakuSource(danmakuSource);
        });
        QWidget::connect(ui->loadDanmakuButton, &QToolButton::clicked, videoWidget, [videoWidget](bool clicked){
            auto filePath = QFileDialog::getOpenFileName(videoWidget, videoWidget->tr("请选择弹幕文件"), "./", ".*");
            if (!filePath.isEmpty()) {
                videoWidget->currentVideo()->loadDanmakuFromFile(filePath);
                videoWidget->currentVideo()->setCurrentDanmakuSource(filePath);
            }
        });
        QWidget::connect(ui->danmakuShowAreaBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setDanmakuShowArea((qreal) value / 100.0);
            ui->danmakuShowAreaLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuSizeBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setDanmakuSize((qreal)value / 100.0);
            ui->danmakuSizeLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuSpeedBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setDanmakuSpeed(value);
            static QStringList list{"极慢", "较慢", "正常", "较快", "极快"};
            ui->danmakuSpeedLabel->setText(list[value]);
        });
        QWidget::connect(ui->danmakuFontComboBox, &QFontComboBox::currentFontChanged, videoWidget, [videoWidget, this](const QFont &f) {
            auto font = videoWidget->danmakuFont();
            font.setFamilies(f.families());
            videoWidget->setDanmakuFont(font);
        });
        QWidget::connect(ui->danmakuBoldCheckBox, &QCheckBox::stateChanged, videoWidget, [videoWidget, this](int checked) {
            auto font = videoWidget->danmakuFont();
            font.setBold(checked == Qt::Checked);
        });
        QWidget::connect(ui->danmakuTransparencyBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setDanmakuTransparency(value / 100.0);
            ui->danmakuTransparencyLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuStroke, &QCheckBox::stateChanged, videoWidget, [videoWidget, this](int state) {
            if (state == Qt::Checked) {
                videoWidget->setDanmakuStroke(StrokeType::STROKE);
            } else {
                videoWidget->setDanmakuStroke(StrokeType::NONE);
            }
        });
        QWidget::connect(ui->danmakuProject45degree, &QCheckBox::stateChanged, videoWidget, [videoWidget, this](int state) {
            if (state == Qt::Checked) {
                videoWidget->setDanmakuStroke(StrokeType::PROJECT);
            } else {
                videoWidget->setDanmakuStroke(StrokeType::NONE);
            }
        });
    }

    void connectSubtitleSetting(VideoWidget *videoWidget) {
        // 选择字幕
        QWidget::connect(ui->subtitleComboBox, &QComboBox::currentTextChanged, videoWidget, [videoWidget](const QString& text) {
            int index = videoWidget->currentVideo()->subtitleSourceList().indexOf(text);
            videoWidget->setSubtitle(index);
        });
        QWidget::connect(ui->loadSubtitleButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            QString filePath = QFileDialog::getOpenFileName(videoWidget, videoWidget->tr("请选择字幕文件"), "./", ".ass;;.*");
            if (!filePath.isEmpty()) {
                videoWidget->currentVideo()->loadSubtitleFromFile(filePath);
                videoWidget->currentVideo()->setCurrentSubtitleSource(filePath);
            }
        });
        QWidget::connect(ui->subtitleSynchronizeTimeBox, &QDoubleSpinBox::valueChanged, videoWidget, [videoWidget](double value) {
            videoWidget->setSubtitleSynchronizeTime(value);
        });
        QWidget::connect(ui->subtitleDefualtsynchronizeTimeButton, &QToolButton::clicked, videoWidget, [this](bool checked) {
            ui->subtitleSynchronizeTimeBox->setValue(0);
        });
        QWidget::connect(ui->subtitlePositionBar, &QSlider::valueChanged, videoWidget, [videoWidget](int value) {
            videoWidget->setSubtitlePosition(value / 100.0);
        });
        QWidget::connect(ui->subtitleFontComboBox, &QFontComboBox::currentFontChanged, videoWidget, [videoWidget](const QFont& f) {
            auto font = videoWidget->subtitleFont();
            font.setFamilies(f.families());
            videoWidget->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleSizeBox, &QSpinBox::valueChanged, videoWidget, [videoWidget](double v) {
            auto font = videoWidget->subtitleFont();
            font.setPixelSize(v);
            videoWidget->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleBoldButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            auto font = videoWidget->subtitleFont();
            font.setBold(!checked);
            videoWidget->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleItalicsButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            auto font = videoWidget->subtitleFont();
            font.setItalic(!checked);
            videoWidget->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleUnderlineButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            auto font = videoWidget->subtitleFont();
            font.setUnderline(!checked);
            videoWidget->setFont(font);
        });
        QWidget::connect(ui->subtitleColor, &QPushButton::clicked, videoWidget, [videoWidget, this](bool checked) {
            getColor(ui->subtitleColor, [videoWidget](const QColor& color) {
                videoWidget->setSubtitleColor(color);
            });
        });
        QWidget::connect(ui->subtitleTransparencyBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            ui->subtitleTransparencyLabel->setText(QString("%1%").arg(value / 100.0));
            videoWidget->setSubtitleTransparency(value / 100.0);
        });
        QWidget::connect(ui->subtitleStrokeCheckBox, &QCheckBox::stateChanged, videoWidget, [videoWidget, this](int checked) {
            if (checked == Qt::Checked) {
                QColor color(ui->subtitleStrokeColor->text());
                videoWidget->setSubtitleStroke(true);
                videoWidget->setSubtitleStrokeColor(color);
                ui->subtitleStrokeColor->setEnabled(true);
                ui->subtitleStrokeTransparencyBar->setEnabled(true);
            } else {
                videoWidget->setSubtitleStroke(false);
                ui->subtitleStrokeTransparencyBar->setEnabled(false);
                ui->subtitleStrokeColor->setEnabled(false);
            }
        });
        QWidget::connect(ui->subtitleStrokeColor, &QPushButton::clicked, videoWidget, [videoWidget, this](bool checked) {
            getColor(ui->subtitleStrokeColor, [videoWidget](const QColor& color) {
                videoWidget->setSubtitleStrokeColor(color);
            });
        });
        QWidget::connect(ui->subtitleStrokeTransparencyBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setSubtitleStrokeTransparency(value / 100.0);
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
        ui->subtitleSizeBox->setValue(12);
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
    Ui::FullSettingView* ui;

private:
    FullSettingWidget *self;
};

FullSettingWidget::FullSettingWidget(QWidget* parent, Qt::WindowFlags f) : PopupWidget(parent, f), d(new FullSettingWidgetPrivate(this)) {
    d->setupUi();
}

void FullSettingWidget::initDanmakuSetting(VideoBLLPtr video) {
    d->ui->danmakuCombobox->addItems(video->danmakuSourceList());
}

void FullSettingWidget::initSubtitleSetting(VideoBLLPtr video) {
    d->ui->subtitleComboBox->addItems(video->subtitleSourceList());
}

void FullSettingWidget::setupSetting(VideoWidget *videoWidget) {
    d->connect(videoWidget);
}

void FullSettingWidget::show() {
    PopupWidget::show();
}