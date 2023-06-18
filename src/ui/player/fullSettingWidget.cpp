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
        

        // ====================色彩选择界面设置====================
    }

    void connect(VideoWidget* videoWidget) {
        connectPlaySetting(videoWidget);
        connectScreenSetting(videoWidget);
        connectColorSetting(videoWidget);
        connectDanmakuSetting(videoWidget);
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
        QWidget::connect(ui->imageQualityEnhancementButton, &QCheckBox::clicked, videoWidget, [videoWidget](bool clicked) {
            videoWidget->setImageQualityEnhancement(clicked);
        });
    }

    void connectColorSetting(VideoWidget* videoWidget) {
        // 色彩设置
        QWidget::connect(ui->brightnessBar, &QSlider::valueChanged, videoWidget, [videoWidget](int value) {
            videoWidget->setBrightness(value);
        });
        QWidget::connect(ui->contrastBar, &QSlider::valueChanged, videoWidget, [videoWidget](int value) {
            videoWidget->setContrast(value);
        });
        QWidget::connect(ui->hueBar, &QSlider::valueChanged, videoWidget, [videoWidget](int value) {
            videoWidget->setHue(value);
        });
        QWidget::connect(ui->saturationBar, &QSlider::valueChanged, videoWidget, [videoWidget](int value) {
            videoWidget->setSaturation(value);
        });
    }

    void connectDanmakuSetting(VideoWidget* videoWidget) {
        // 弹幕选择
        QWidget::connect(ui->danmakuCombobox, &QComboBox::currentTextChanged, videoWidget, [videoWidget](const QString &danmakuSource) {
            videoWidget->currentVideo()->setCurrentDanmakuSource(danmakuSource);
        });
        QWidget::connect(ui->loadDanmakuButton, &QToolButton::clicked, videoWidget, [videoWidget](bool clicked){
            // TODO(llhsdmd): 添加视频，暂时只能添加本地视频到播放列表算了。
            auto filePath = QFileDialog::getOpenFileName(videoWidget, videoWidget->tr("请选择弹幕文件"), "./", ".*");
            videoWidget->currentVideo()->loadDanmakuFromFile(filePath);
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
        QWidget::connect(ui->danmakuBoldCheckBox, &QCheckBox::clicked, videoWidget, [videoWidget, this](bool checked) {
            auto font = videoWidget->danmakuFont();
            font.setBold(checked);
        });
        QWidget::connect(ui->danmakuTransparencyBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setDanmakuTransparency(value / 100.0);
            ui->danmakuTransparencyLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuBackgroundCheckBox, &QCheckBox::clicked, videoWidget, [videoWidget, this](bool checked) {
            if (checked) {
                QColor color(ui->danmakuBackgroundColor->text());
                videoWidget->setDanmakuBackground(true);
                videoWidget->setDanmakuBackgroundColor(color);
                ui->danmakuBackgroundColor->setDisabled(true);
                ui->danmakuBackgroundBar->setDisabled(true);
            } else {
                ui->danmakuBackgroundBar->setDisabled(false);
                videoWidget->setDanmakuBackground(false);
                ui->danmakuBackgroundColor->setDisabled(false);
            }
        });
        QWidget::connect(ui->danmakuBackgroundColor, &QPushButton::clicked, videoWidget, [videoWidget, this](bool checked) {
            getColor(ui->danmakuBackgroundColor, [videoWidget](const QColor& color) {
                videoWidget->setDanmakuBackgroundColor(color);
            });
        });
        QWidget::connect(ui->danmakuStrokeCheckBox, &QCheckBox::clicked, videoWidget, [videoWidget, this](bool checked) {
            if (checked) {
                QColor color(ui->danmakuStroke->text());
                videoWidget->setDanmakuStroke(true);
                videoWidget->setDanmakuStrokeColor(color);
                ui->danmakuStroke->setDisabled(true);
                ui->danmakuStrokeTransparencyBar->setDisabled(true);
            } else {
                videoWidget->setDanmakuStroke(false);
                ui->danmakuBackgroundColor->setDisabled(false);
                ui->danmakuStrokeTransparencyBar->setDisabled(false);
            }
        });
        QWidget::connect(ui->danmakuStroke, &QPushButton::clicked, videoWidget, [videoWidget, this](bool checked) {
            getColor(ui->danmakuStroke, [videoWidget](const QColor& color) {
                videoWidget->setDanmakuStrokeColor(color);
            });
        });
        QWidget::connect(ui->danmakuBackgroundBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setDanmakuBackgroundTransparency(value / 100.0);
            ui->danmakuBackgroundLabel->setText(QString("%1%").arg(value));
        });
        QWidget::connect(ui->danmakuStrokeTransparencyBar, &QSlider::valueChanged, videoWidget, [videoWidget, this](int value) {
            videoWidget->setDanmakuStrokeTransparency(value / 100.0);
            ui->danmakuStrokeTransparencyLabel->setText(QString("%1%").arg(value));
        });
    }

    void connectSubtitleSetting(VideoWidget *videoWidget) {
        // 选择字幕
        QWidget::connect(ui->subtitleComboBox, &QComboBox::currentTextChanged, videoWidget, [videoWidget](const QString& text) {
            videoWidget->currentVideo()->setCurrentSubtitleSource(text);
        });
        QWidget::connect(ui->loadSubtitleButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            QString filePath = QFileDialog::getOpenFileName(videoWidget, videoWidget->tr("请选择字幕文件"), "./", ".ass;;.*");
            videoWidget->currentVideo()->loadSubtitleFromFile(filePath);
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
            font.setBold(checked);
            videoWidget->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleItalicsButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            auto font = videoWidget->subtitleFont();
            font.setItalic(checked);
            videoWidget->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleUnderlineButton, &QToolButton::clicked, videoWidget, [videoWidget](bool checked) {
            auto font = videoWidget->subtitleFont();
            font.setUnderline(checked);
            videoWidget->setFont(font);
        });
        QWidget::connect(ui->subtitleColor, &QPushButton::clicked, videoWidget, [videoWidget, this](bool checked) {
            getColor(ui->subtitleColor, [videoWidget](const QColor& color) {
                videoWidget->setSubtitleColor(color);
            });
        });
    }

    void getColor(QPushButton* colorButton, std::function<void(const QColor& color)> func) {
        self->setHideAfterLeave(false);

        PopupWidget* colorSelectWidget = new PopupWidget(self, Qt::Popup);
        QHBoxLayout* hboxlayout = new QHBoxLayout();
        QColorDialog* colorDiralog = new QColorDialog();

        QWidget::connect(colorDiralog, &QColorDialog::colorSelected, self, [colorButton, colorSelectWidget, func, this](const QColor& color){
            colorButton->setText(color.name());
            colorButton->setStyleSheet("QPushButton{background-color: " + color.name() + ";" 
            + "color: " + color.darker().name() + ";"
            + R"(border: 1px dashed white;})");
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


public:
    Ui::FullSettingView* ui;

private:
    FullSettingWidget *self;
};

FullSettingWidget::FullSettingWidget(QWidget* parent, Qt::WindowFlags f) : PopupWidget(parent, f), d(new FullSettingWidgetPrivate(this)) {
    d->setupUi();
}

void FullSettingWidget::setupSetting(VideoWidget *videoWidget) {
    d->connect(videoWidget);
}

void FullSettingWidget::show() {
    PopupWidget::show();
}