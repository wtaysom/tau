/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

/*
 * Taken from plotter example in "C++ GUI Programming with Qt 4" by
 * Jasmin Blanchette and Mark Summerfield, pp 116-134.
 */

#include <QtGui>
#include <cmath>

#include "plotter.h"

using namespace std;

Plotter::Plotter(QWidget *parent)
	: QWidget(parent)
{
	setBackgroundRole(QPalette::Dark);
	setAutoFillBackground(true);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setFocusPolicy(Qt::StrongFocus);
	rubberBandIsShown = false;

	zoomInButton = new QToolButton(this);
	zoomInButton->setIcon(QIcon(":/images/zoomin.png"));
	zoomInButton->adjustSize();
	connect(zoomInButton, SIGNAL(clicked()), this, SLOT(zoomIn()));

	zoomOutButton = new QToolButton(this);
	zoomOutButton->setIcon(QIcon(":/images/zoomout.png"));
	zoomOutButton->adjustSize();
	connect(zoomOutButton, SIGNAL(clicked()), this, SLOT(zoomOut()));

	stopButton = new QToolButton(this);
	stopButton->setIcon(QIcon(":/images/Stop.png"));
	stopButton->adjustSize();
	connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));

	startButton = new QToolButton(this);
	startButton->setIcon(QIcon(":/images/StopPressed.png"));
	startButton->adjustSize();
	connect(startButton, SIGNAL(clicked()), this, SLOT(start()));

	x=0.0;
	timer = startTimer(Timer);
	stopped = false;

	setPlotSettings(PlotSettings());
}

void Plotter::setPlotSettings(const PlotSettings &settings)
{
	zoomStack.clear();
	zoomStack.append(settings);
	curZoom = 0;
	zoomInButton->hide();
	zoomOutButton->hide();
	stopButton->show();
	startButton->hide();
	refreshPixmap();
}

void Plotter::zoomOut()
{
	if (curZoom > 0) {
		--curZoom;
		zoomOutButton->setEnabled(curZoom > 0);
		zoomInButton->setEnabled(true);
		zoomInButton->show();
		refreshPixmap();
	}
}

void Plotter::zoomIn()
{
	if (curZoom < zoomStack.count() - 1) {
		++curZoom;
		zoomInButton->setEnabled(curZoom < zoomStack.count() - 1);
		zoomOutButton->setEnabled(true);
		zoomOutButton->show();
		refreshPixmap();
	}
}

void Plotter::stop()
{
#if 1
	killTimer(timer);
	timer = 0;
#endif
	stopped = true;
	stopButton->setEnabled(false);
	stopButton->hide();
	startButton->setEnabled(true);
	startButton->show();
	refreshPixmap();
}

void Plotter::start()
{
#if 1
	timer = startTimer(Timer);
#endif
	stopped = false;
	startButton->setEnabled(false);
	startButton->hide();
	stopButton->setEnabled(true);
	stopButton->show();
	refreshPixmap();
}

void Plotter::setCurveData(int id, Fn_f fn)
{
//	curveMap[id] = data;
	fnMap[id] = fn;
	refreshPixmap();
}

void Plotter::appendCurveData(int id, double x, double y)
{
	curveMap[id].append(QPointF(x, y));

	PlotSettings *settings = &zoomStack[curZoom];
	if (x > settings->maxX) settings->maxX = x;
	if (x < settings->minX) settings->minX = x;
	if (y > settings->maxY) settings->maxY = y;
	if (y < settings->minY) settings->minY = y;
	if (!stopped) {
		settings->adjust();
		refreshPixmap();
	}
}

void Plotter::clearCurve(int id)
{
	curveMap.remove(id);
	refreshPixmap();
}

QSize Plotter::minimumSizeHint() const
{
	return QSize(6 * Margin, 4 * Margin);
}

QSize Plotter::sizeHint() const
{
	return QSize(12 * Margin, 8 * Margin);
}

void Plotter::paintEvent(QPaintEvent * /* event */)
{
	QStylePainter painter(this);
	painter.drawPixmap(0, 0, pixmap);

	if (rubberBandIsShown) {
		painter.setPen(palette().light().color());
		painter.drawRect(rubberBandRect.normalized()
					   .adjusted(0, 0, -1, -1));
	}

	if (hasFocus()) {
		QStyleOptionFocusRect option;
		option.initFrom(this);
		option.backgroundColor = palette().dark().color();
		painter.drawPrimitive(QStyle::PE_FrameFocusRect, option);
	}
}

void Plotter::resizeEvent(QResizeEvent * /* event */)
{
	int x = width() - (zoomInButton->width()
			   + zoomOutButton->width()
			   + stopButton->width() + 15);
	zoomInButton->move(x, 5);
	zoomOutButton->move(x + zoomInButton->width() + 5, 5);
	stopButton->move(x + zoomInButton->width() + 5
				+ zoomOutButton->width() + 5, 5);
	startButton->move(x + zoomInButton->width() + 5
				+ zoomOutButton->width() + 5, 5);
	refreshPixmap();
}

void Plotter::mousePressEvent(QMouseEvent *event)
{
	QRect rect(Margin, Margin,
			   width() - 2 * Margin, height() - 2 * Margin);

	if (event->button() == Qt::LeftButton) {
		if (rect.contains(event->pos())) {
			rubberBandIsShown = true;
			rubberBandRect.setTopLeft(event->pos());
			rubberBandRect.setBottomRight(event->pos());
			updateRubberBandRegion();
			setCursor(Qt::CrossCursor);
		}
	}
}

void Plotter::mouseMoveEvent(QMouseEvent *event)
{
	if (rubberBandIsShown) {
		updateRubberBandRegion();
		rubberBandRect.setBottomRight(event->pos());
		updateRubberBandRegion();
	}
}

void Plotter::mouseReleaseEvent(QMouseEvent *event)
{
	if ((event->button() == Qt::LeftButton) && rubberBandIsShown) {
		rubberBandIsShown = false;
		updateRubberBandRegion();
		unsetCursor();

		QRect rect = rubberBandRect.normalized();
		if (rect.width() < 4 || rect.height() < 4)
			return;
		rect.translate(-Margin, -Margin);

		PlotSettings prevSettings = zoomStack[curZoom];
		PlotSettings settings;
		double dx = prevSettings.spanX() / (width() - 2 * Margin);
		double dy = prevSettings.spanY() / (height() - 2 * Margin);
		settings.minX = prevSettings.minX + dx * rect.left();
		settings.maxX = prevSettings.minX + dx * rect.right();
		settings.minY = prevSettings.maxY - dy * rect.bottom();
		settings.maxY = prevSettings.maxY - dy * rect.top();
		settings.adjust();

		zoomStack.resize(curZoom + 1);
		zoomStack.append(settings);
		zoomIn();
	}
}

void Plotter::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	default:
		QWidget::keyPressEvent(event);
	}
}

void Plotter::wheelEvent(QWheelEvent *event)
{
	int numDegrees = event->delta() / 8;
	int numTicks = numDegrees / 15;

	if (event->orientation() == Qt::Horizontal) {
		zoomStack[curZoom].scroll(numTicks, 0);
	} else {
		zoomStack[curZoom].scroll(0, numTicks);
	}
	refreshPixmap();
}

void Plotter::timerEvent(QTimerEvent *event)
{
	if (event->timerId() != timer) {
		QWidget::timerEvent(event);
		return;
	}
	QMapIterator<int, Fn_f> i(fnMap);
	while (i.hasNext()) {
		double	y;

		i.next();

		int id = i.key();
		y = fnMap[id](x, id);
		appendCurveData(id, x, y);
	}
	x += 1.0;
}

void Plotter::updateRubberBandRegion()
{
	QRect rect = rubberBandRect.normalized();
	update(rect.left(), rect.top(), rect.width(), 1);
	update(rect.left(), rect.top(), 1, rect.height());
	update(rect.left(), rect.bottom(), rect.width(), 1);
	update(rect.right(), rect.top(), 1, rect.height());
}

void Plotter::refreshPixmap()
{
	pixmap = QPixmap(size());
	pixmap.fill(this, 0, 0);

	QPainter painter(&pixmap);
	painter.initFrom(this);
	drawGrid(&painter);
	drawCurves(&painter);
	update();
}

void Plotter::drawGrid(QPainter *painter)
{
	QRect rect(Margin, Margin,
		   width() - 2 * Margin, height() - 2 * Margin);
	if (!rect.isValid())
		return;

	PlotSettings settings = zoomStack[curZoom];
	QPen quiteDark = palette().dark().color().light();
	QPen light = palette().light().color();

	for (int i = 0; i <= settings.numXTicks; ++i) {
		int x = rect.left() + (i * (rect.width() - 1)
						 / settings.numXTicks);
		double label = settings.minX + (i * settings.spanX()
						  / settings.numXTicks);
		painter->setPen(quiteDark);
		painter->drawLine(x, rect.top(), x, rect.bottom());
		painter->setPen(light);
		painter->drawLine(x, rect.bottom(), x, rect.bottom() + 5);
		painter->drawText(x - 50, rect.bottom() + 5, 100, 15,
					  Qt::AlignHCenter | Qt::AlignTop,
					  QString::number(label));
	}
	for (int j = 0; j <= settings.numYTicks; ++j) {
		int y = rect.bottom() - (j * (rect.height() - 1)
						   / settings.numYTicks);
		double label = settings.minY + (j * settings.spanY()
						  / settings.numYTicks);
		painter->setPen(quiteDark);
		painter->drawLine(rect.left(), y, rect.right(), y);
		painter->setPen(light);
		painter->drawLine(rect.left() - 5, y, rect.left(), y);
		painter->drawText(rect.left() - Margin, y - 10, Margin - 5, 20,
					  Qt::AlignRight | Qt::AlignVCenter,
					  QString::number(label));
	}
	painter->drawRect(rect.adjusted(0, 0, -1, -1));
}

void Plotter::drawCurves(QPainter *painter)
{
	static const QColor colorForIds[6] = {
		Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow
	};
	PlotSettings settings = zoomStack[curZoom];
	QRect rect(Margin, Margin,
			   width() - 2 * Margin, height() - 2 * Margin);
	if (!rect.isValid())
		return;

	painter->setClipRect(rect.adjusted(+1, +1, -1, -1));

	QMapIterator<int, QVector<QPointF> > i(curveMap);
	while (i.hasNext()) {
		i.next();

		int id = i.key();
		const QVector<QPointF> &data = i.value();
		QPolygonF polyline(data.count());

		//if (data.count() < 3) break;

		for (int j = 0; j < data.count(); ++j) {
			double dx = data[j].x() - settings.minX;
			double dy = data[j].y() - settings.minY;
			double x = rect.left() + (dx * (rect.width() - 1)
							/ settings.spanX());
			double y = rect.bottom() - (dy * (rect.height() - 1)
							/ settings.spanY());
			polyline[j] = QPointF(x, y);
		}
		painter->setPen(colorForIds[uint(id) % 6]);
		painter->drawPolyline(polyline);
	}
}

PlotSettings::PlotSettings()
{
	minX = 0.0;
	maxX = 10.0;
	numXTicks = 5;

	minY = 0.0;
	maxY = 10.0;
	numYTicks = 5;
}

void PlotSettings::scroll(int dx, int dy)
{
	double stepX = spanX() / numXTicks;
	minX += dx * stepX;
	maxX += dx * stepX;

	double stepY = spanY() / numYTicks;
	minY += dy * stepY;
	maxY += dy * stepY;
}

void PlotSettings::adjust()
{
	adjustAxis(minX, maxX, numXTicks);
	adjustAxis(minY, maxY, numYTicks);
}

void PlotSettings::adjustAxis(double &min, double &max, int &numTicks)
{
	const int MinTicks = 4;
	double grossStep = (max - min) / MinTicks;
	double step = pow(10.0, floor(log10(grossStep)));

	if (5 * step < grossStep) {
		step *= 5;
	} else if (2 * step < grossStep) {
		step *= 2;
	}

	numTicks = int(ceil(max / step) - floor(min / step));
	if (numTicks < MinTicks)
		numTicks = MinTicks;
	min = floor(min / step) * step;
	max = ceil(max / step) * step;
}
