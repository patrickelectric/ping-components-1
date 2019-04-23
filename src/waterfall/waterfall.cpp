#include "filemanager.h"
#include "waterfall.h"

#include <limits>

#include <QtConcurrent>
#include <QPainter>
#include <QtMath>
#include <QVector>

PING_LOGGING_CATEGORY(waterfall, "ping.waterfall")

// Number of samples to display
uint16_t Waterfall::displayWidth = 500;

Waterfall::Waterfall(QQuickItem *parent):
    QQuickPaintedItem(parent),
    _image(2500, 2500, QImage::Format_RGBA8888),
    _painter(nullptr),
    _maxDepthToDrawInPixels(0),
    _minDepthToDrawInPixels(0),
    _mouseDepth(0),
    _containsMouse(false),
    _smooth(true),
    _updateTimer(new QTimer(this)),
    currentDrawIndex(displayWidth)
{
    // This is the max depth that ping returns
    setWaterfallMaxDepth(70);
    _DCRing.fill({static_cast<float>(_image.height()), 0, 0, 0}, displayWidth);
    setAntialiasing(_smooth);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    _image.fill(QColor(Qt::transparent));
    setGradients();
    setTheme("Thermal 5");

    connect(_updateTimer, &QTimer::timeout, this, [&] {update();});
    _updateTimer->setSingleShot(true);
    _updateTimer->start(50);
}

void Waterfall::clear()
{
    qCDebug(waterfall) << "Cleaning waterfall and restarting internal variables";
    _maxDepthToDrawInPixels = 0;
    _minDepthToDrawInPixels = 0;
    _mouseDepth = 0;
    _DCRing.fill({static_cast<float>(_image.height()), 0, 0, 0}, displayWidth);
    _image.fill(Qt::transparent);
}

void Waterfall::setGradients()
{
    WaterfallGradient thermal5Blue(QStringLiteral("Thermal blue"), {
        QColor("#05225f"),
        QColor("#6aa84f"),
        QColor("#ffff00"),
        QColor("#7f6000"),
        QColor("#5c0f08"),
    });
    _gradients.append(thermal5Blue);

    WaterfallGradient thermal5Black(QStringLiteral("Thermal black"), {
        Qt::black,
        QColor("#6aa84f"),
        QColor("#ffff00"),
        QColor("#7f6000"),
        QColor("#5c0f08"),
    });
    _gradients.append(thermal5Black);

    WaterfallGradient thermal5White(QStringLiteral("Thermal white"), {
        Qt::white,
        QColor("#6aa84f"),
        QColor("#ffff00"),
        QColor("#7f6000"),
        QColor("#5c0f08"),
    });
    _gradients.append(thermal5White);

    WaterfallGradient monochromeBlack(QStringLiteral("Monochrome black"), {
        Qt::black,
        Qt::white,
    });
    _gradients.append(monochromeBlack);

    WaterfallGradient monochromeWhite(QStringLiteral("Monochrome white"), {
        Qt::white,
        Qt::black,
    });
    _gradients.append(monochromeWhite);

    WaterfallGradient monochromeSepia(QStringLiteral("Monochrome sepia"), {
        QColor("#302113"),
        QColor("#e8c943"),
    });
    _gradients.append(monochromeSepia);

    loadUserGradients();

    for(auto &gradient : _gradients) {
        _themes.append(gradient.name());
    }
    qCDebug(waterfall) << "Gradients:" << _themes;
    emit themesChanged();
}

void Waterfall::loadUserGradients()
{
    auto fileInfoList = FileManager::self()->getFilesFrom(FileManager::Folder::Gradients);
    for(auto& fileInfo : fileInfoList) {
        qCDebug(waterfall) << fileInfo.fileName();
        QFile file(fileInfo.absoluteFilePath());
        WaterfallGradient gradient(file);
        if(!gradient.isOk()) {
            qCDebug(waterfall) << "Invalid gradient file:" << fileInfo.fileName();
            continue;
        }
        _gradients.append(gradient);
    }
}

void Waterfall::setWaterfallMaxDepth(float maxDepth)
{
    _waterfallDepth = maxDepth;
    _minPixelsPerMeter = _image.height()/_waterfallDepth;
}

void Waterfall::setTheme(const QString& theme)
{
    _theme = theme;
    for(auto &gradient : _gradients) {
        if(gradient.name() == theme) {
            _gradient = gradient;
            break;
        }
    }
}

void Waterfall::paint(QPainter *painter)
{
    static QPixmap pix;
    if(painter != _painter) {
        _painter = painter;
    }

    // http://blog.qt.io/blog/2006/05/13/fast-transformed-pixmapimage-drawing/
    pix = QPixmap::fromImage(_image, Qt::NoFormatConversion);
    // Code for debug, draw the entire waterfall
    _painter->drawPixmap(_painter->viewport(), pix, QRect(0, 0, _image.width(), _image.height()));
    /*
    _painter->drawPixmap(QRect(0, 0, width(), height()), pix,
                         QRect(0, _minDepthToDrawInPixels, displayWidth, _maxDepthToDrawInPixels));
                         */
}

void Waterfall::setImage(const QImage &image)
{
    _image = image;
    emit imageChanged();
    setImplicitWidth(image.width());
    setImplicitHeight(image.height());
}

QColor Waterfall::valueToRGB(float point)
{
    return _gradient.getColor(point);
}

float Waterfall::RGBToValue(const QColor& color)
{
    return _gradient.getValue(color);
}

void Waterfall::draw(const QVector<double>& points, float confidence, float initPoint, float length, float distance)
{
    static float currentDrawAngle(1);
    static QVector<double> oldPoints = points;
    static QPoint center(_image.width()/2, _image.height()/2);
    static const float d2r = M_PI/180.0;
    static QColor pointColor;
    static float step;
    static float angleStep;
    float actualAngle = currentDrawAngle*d2r;

    for(int i = 1; i < center.x(); i++) {
        pointColor = valueToRGB(points[i%200]);
        step = ceil(i*2*d2r)*1.15;
        for(float u = -0.25; u <= 0.25; u += 1/step) {
            angleStep = u*d2r+actualAngle;
            _image.setPixelColor(center.x() + i*cos(angleStep), center.y() + i*sin(angleStep), pointColor);
        }
    }

    currentDrawAngle += 0.5;
    qDebug() << currentDrawAngle;
    // Fix max update in 20Hz at max
    if(!_updateTimer->isActive()) {
        _updateTimer->start(50);
    }
}

void Waterfall::hoverMoveEvent(QHoverEvent *event)
{
    event->accept();
    auto pos = event->pos();
    _mousePos = pos;
    emit mousePosChanged();

    static uint16_t first;
    if (currentDrawIndex < displayWidth) {
        first = 0;
    } else {
        first = currentDrawIndex - displayWidth;
    }

    int widthPos = pos.x()*displayWidth/width();
    pos.setX(pos.x()*displayWidth/width() + first);
    pos.setY(pos.y()*(_maxDepthToDrawInPixels-_minDepthToDrawInPixels)/(float)height());

    // depth
    _mouseDepth = pos.y()/(float)_minPixelsPerMeter;
    emit mouseMove();

    const auto& depthAndConfidence = _DCRing[displayWidth - widthPos];
    _mouseColumnConfidence = depthAndConfidence.confidence;
    _mouseColumnDepth = depthAndConfidence.distance;
    emit mouseColumnConfidenceChanged();
    emit mouseColumnDepthChanged();
}

void Waterfall::hoverLeaveEvent(QHoverEvent *event)
{
    Q_UNUSED(event)
    // The mouse is not inside the waterfall area, so set the depth under the mouse to an invalid value
    _mouseDepth = -1;
    _containsMouse = false;
    emit containsMouseChanged();
}

void Waterfall::hoverEnterEvent(QHoverEvent *event)
{
    Q_UNUSED(event)
    _containsMouse = true;
    emit containsMouseChanged();
}
