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

#ifndef PLOTTER_H
#define PLOTTER_H

#include <QMap>
#include <QPixmap>
#include <QVector>
#include <QWidget>

class QToolButton;
class PlotSettings;

typedef double	(*Fn_f)(double x, int id);

class Plotter : public QWidget
{
	Q_OBJECT

public:
	Plotter(QWidget *parent = 0);

	void setPlotSettings(const PlotSettings &settings);
	void setCurveData(int id, Fn_f fn);
	void appendCurveData(int id, double x, double y);
	void clearCurve(int id);
	QSize minimumSizeHint() const;
	QSize sizeHint() const;

public slots:
	void zoomIn();
	void zoomOut();
	void stop();
	void start();

protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void wheelEvent(QWheelEvent *event);
	void timerEvent(QTimerEvent *event);

private:
	void updateRubberBandRegion();
	void refreshPixmap();
	void drawGrid(QPainter *painter);
	void drawCurves(QPainter *painter);

	enum { Margin = 50, Timer = 1000 };

	QToolButton			*zoomInButton;
	QToolButton			*zoomOutButton;
	QToolButton			*stopButton;
	QToolButton			*startButton;
	QMap<int, QVector<QPointF> >	curveMap;
	QMap<int, Fn_f >		fnMap;
	QVector<PlotSettings>		zoomStack;
	int				curZoom;
	bool				rubberBandIsShown;
	QRect				rubberBandRect;
	QPixmap				pixmap;
	int				timer;
	bool				stopped;
	double				x;
};

class PlotSettings
{
public:
	PlotSettings();

	void scroll(int dx, int dy);
	void adjust();
	double spanX() const { return maxX - minX; }
	double spanY() const { return maxY - minY; }

	double minX;
	double maxX;
	int numXTicks;
	double minY;
	double maxY;
	int numYTicks;

private:
	static void adjustAxis(double &min, double &max, int &numTicks);
};
#endif
