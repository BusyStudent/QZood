#include "subtitleSetting.hpp"

#include "../videoWidget.hpp"
#include "../../../common/myGlobalLog.hpp"
#include "../fullSettingWidget.hpp"
#include "ui_subtitleSetting.h"

#include <QFileDialog>
#include <QColorDialog>

void setColorForButton(QPushButton* colorButton,const QColor &color) {
    colorButton->setText(color.name());
    colorButton->setStyleSheet("QPushButton{background-color: " + color.name() + ";"
        + "color: " + color.darker().name() + ";"
        + R"(border: 1px dashed white;})");
}

void getColor(QPushButton* colorButton, std::function<void(const QColor& color)> func) {
    PopupWidget* colorSelectWidget = new PopupWidget(nullptr, Qt::Popup);
    QHBoxLayout* hboxlayout = new QHBoxLayout();
    QColorDialog* colorDiralog = new QColorDialog();

    QWidget::connect(colorDiralog, &QColorDialog::colorSelected, [colorButton, colorSelectWidget, func](const QColor& color) {
            setColorForButton(colorButton, color);
            colorSelectWidget->hide();
            func(color);
    });
    QWidget::connect(colorDiralog, &QColorDialog::rejected, colorSelectWidget, &PopupWidget::hide);
    QWidget::connect(colorSelectWidget, &PopupWidget::hided, colorSelectWidget, [colorSelectWidget]() {
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

class SubtitleSettingPrivate {
public:
    SubtitleSettingPrivate(SubtitleSetting *self) : self(self), ui(new Ui::SubtitleSetting()) {
        ui->setupUi(self);
    }

    void setup(VideoWidget *videoWidget) {
        this->videoWidget = videoWidget;
        connectToVideoWidget();
    }
    void reset() {
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
    void refresh() {
        if (nullptr != videoWidget) {
            auto video = videoWidget->currentVideo();
            if (nullptr != video) {
                int subtitlePosition = video->getStatus<int>("subtitlePosition").value_or(20);
                ui->subtitlePositionBar->setValue(subtitlePosition);

                int subtitleSize = video->getStatus<int>("subtitleSize").value_or(40);
                ui->subtitleSizeBox->setValue(subtitleSize);

                bool subtitleBold = video->getStatus<bool>("subtitleBold").value_or(false);
                ui->subtitleBoldButton->setChecked(subtitleBold);
                
                bool subtitleItalics = video->getStatus<bool>("subtitleItalics").value_or(false);
                ui->subtitleItalicsButton->setChecked(subtitleItalics);

                bool subtitleUnderline = video->getStatus<bool>("subtitleUnderline").value_or(false);
                ui->subtitleUnderlineButton->setChecked(subtitleUnderline);

                int subtitleTransparency = video->getStatus<int>("subtitleTransparency").value_or(0);
                ui->subtitleTransparencyBar->setValue(subtitleTransparency);

                auto subtitleStroke = video->getStatus<bool>("subtitleStroke");
                Qt::CheckState stroke = Qt::Unchecked;
                if (subtitleStroke.has_value()) {
                    stroke = subtitleStroke.value() ? Qt::Checked : Qt::Unchecked;
                }
                ui->subtitleStrokeCheckBox->setCheckState(stroke);
                ui->subtitleStrokeCheckBox->stateChanged(stroke);
                
                QColor subtitleStrokeColor = video->getStatus<QColor>("subtitleStrokeColor").value_or(Qt::white);
                setColorForButton(ui->subtitleStrokeColor, subtitleStrokeColor);

                QColor subtitleColor = video->getStatus<QColor>("subtitleColor").value_or(Qt::black);
                setColorForButton(ui->subtitleColor, subtitleColor);

                auto subtitleStrokeTransparency = video->getStatus<int>("subtitleStrokeTransparency").value_or(0);
                ui->subtitleStrokeTransparencyBar->setValue(subtitleStrokeTransparency);

                ui->subtitleComboBox->clear();
                ui->subtitleComboBox->addItems(video->subtitleSourceList());
            }
        }
    }

public:
    VideoWidget *videoWidget = nullptr;

private:
    void connectToVideoWidget() {
        // 选择字幕
        ui->subtitleFontComboBox->setCurrentFont(videoWidget->videoCanvas()->subtitleFont());
        ui->subtitleSizeBox->setValue(videoWidget->videoCanvas()->subtitleFont().pixelSize());

        QWidget::connect(ui->subtitleComboBox, &QComboBox::currentTextChanged, videoWidget, [this](const QString& text) {
            int index = videoWidget->currentVideo()->subtitleSourceList().indexOf(text) - 1;
            if (index >= 0 && index < videoWidget->player()->subtitleTracks().size()) {
                videoWidget->currentVideo()->setCurrentSubtitleSource(text);
                videoWidget->player()->setActiveSubtitleTrack(index);
            }
        });
        QWidget::connect(ui->loadSubtitleButton, &QToolButton::clicked, videoWidget, [this](bool checked) {
            QString filePath = QFileDialog::getOpenFileName(videoWidget, videoWidget->tr("请选择字幕文件"), "./", ".ass;;.*");
            if (!filePath.isEmpty()) {
                videoWidget->currentVideo()->loadSubtitleFromFile(filePath);
                videoWidget->currentVideo()->setCurrentSubtitleSource(filePath);
            }
        });
        QWidget::connect(ui->subtitleSynchronizeTimeBox, &QDoubleSpinBox::valueChanged, videoWidget, [this](double value) {
            // TODO(llhsdmd) : setSubtitleSynchronizeTime
            qInfo() << "TODO(setSubtitleSynchronizeTime)";
        });
        QWidget::connect(ui->subtitleDefualtsynchronizeTimeButton, &QToolButton::clicked, videoWidget, [this](bool checked) {
            ui->subtitleSynchronizeTimeBox->setValue(0);
        });
        QWidget::connect(ui->subtitlePositionBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            // TODO(llhsdmd) : setSubtitlePosition
            qInfo() << "TODO(setSubtitlePosition)";
        });
        QWidget::connect(ui->subtitleFontComboBox, &QFontComboBox::currentFontChanged, videoWidget, [this](const QFont& f) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setFamilies(f.families());
            videoWidget->videoCanvas()->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleSizeBox, &QSpinBox::valueChanged, videoWidget, [this](double v) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setPixelSize(v);
            videoWidget->videoCanvas()->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleBoldButton, &QToolButton::clicked, videoWidget, [this](bool checked) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setBold(!checked);
            videoWidget->videoCanvas()->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleItalicsButton, &QToolButton::clicked, videoWidget, [this](bool checked) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setItalic(!checked);
            videoWidget->videoCanvas()->setSubtitleFont(font);
        });
        QWidget::connect(ui->subtitleUnderlineButton, &QToolButton::clicked, videoWidget, [this](bool checked) {
            auto font = videoWidget->videoCanvas()->subtitleFont();
            font.setUnderline(!checked);
            videoWidget->videoCanvas()->setFont(font);
        });
        QWidget::connect(ui->subtitleColor, &QPushButton::clicked, videoWidget, [this](bool checked) {
            getColor(ui->subtitleColor, [this](const QColor& color) {
                videoWidget->videoCanvas()->setSubtitleColor(color);
            });
        });
        QWidget::connect(ui->subtitleTransparencyBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            ui->subtitleTransparencyLabel->setText(QString("%1%").arg(value / 100.0));
            videoWidget->videoCanvas()->setSubtitleOpacity(1.0 - value / 100.0);
        });
        QWidget::connect(ui->subtitleStrokeCheckBox, &QCheckBox::stateChanged, videoWidget, [this](int checked) {
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
        QWidget::connect(ui->subtitleStrokeColor, &QPushButton::clicked, videoWidget, [this](bool checked) {
            getColor(ui->subtitleStrokeColor, [this](const QColor& color) {
                // TODO(llhsdmd) : setSubtitleStroke
                qInfo() << "TODO(setSubtitleStroke)";
                videoWidget->videoCanvas()->setSubtitleOutlineColor(color);
            });
        });
        QWidget::connect(ui->subtitleStrokeTransparencyBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            // TODO(llhsdmd) : setSubtitleStrokeTransparency
            qInfo() << "TODO(setSubtitleStrokeTransparency)";
            ui->subtitleStrokeTransparencyLabel->setText(QString("%1%").arg(value));
        });
    }

private:
    SubtitleSetting *self;
    QScopedPointer<Ui::SubtitleSetting> ui;
};

SubtitleSetting::SubtitleSetting(QWidget *parent) : QWidget(parent), SettingItem(), d(new SubtitleSettingPrivate(this)) {

}
SubtitleSetting::~SubtitleSetting() {

}
void SubtitleSetting::initialize(VideoWidget *videoWidget) {
    d->setup(videoWidget);
}
QString SubtitleSetting::title() {
    return "字幕";
}
QWidget *SubtitleSetting::widget() {
    return this;
}
void SubtitleSetting::refresh() {
    d->refresh();
}
void SubtitleSetting::reset() {
    d->reset();
}