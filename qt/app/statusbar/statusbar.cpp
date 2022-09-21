/*
 * Copyright (c) 2016 J-P Nurmi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "statusbar.h"
#include "statusbar_p.h"

#include <QDebug>

QColor StatusBarPrivate::color;
QColor StatusBarPrivate::navigationBarColor;
StatusBar::Theme StatusBarPrivate::theme = StatusBar::Light;
StatusBar::Theme StatusBarPrivate::navigatorTheme = StatusBar::Light;

StatusBar::StatusBar(QObject *parent) : QObject(parent)
{
}

bool StatusBar::isAvailable()
{
    return StatusBarPrivate::isAvailable_sys();
}

QColor StatusBar::color()
{
    return StatusBarPrivate::color;
}

QColor StatusBar::navigationBarColor()
{
    return StatusBarPrivate::navigationBarColor;
}

void StatusBar::setColor(const QColor &color)
{
	if (!enabled_) return;
	qInfo() << "[StatusBar::setColor]:" << color;
    StatusBarPrivate::color = color;
    StatusBarPrivate::setColor_sys(color);
}

void StatusBar::setNavigationBarColor(const QColor &color)
{
	if (!enabled_) return;
	qInfo() << "[StatusBar::setNavigationBarColor]:" << color;
    StatusBarPrivate::navigationBarColor = color;
    StatusBarPrivate::setNavigationBarColor_sys(color);
}

StatusBar::Theme StatusBar::theme()
{
    return StatusBarPrivate::theme;
}

void StatusBar::setTheme(Theme theme)
{
	if (!enabled_) return;
    StatusBarPrivate::theme = theme;
    StatusBarPrivate::setTheme_sys(theme);
}

StatusBar::Theme StatusBar::navigationTheme()
{
	return StatusBarPrivate::navigatorTheme;
}

void StatusBar::setNavigationTheme(Theme theme)
{
	if (!enabled_) return;
	qInfo() << "[StatusBar::setNavigationTheme]:" << theme;
	StatusBarPrivate::navigatorTheme = theme;
	StatusBarPrivate::setNavigatorTheme_sys(theme);
	emit navigationThemeChanged();
}

int StatusBar::extraPadding()
{
    return StatusBarPrivate::getPadding();
}

int StatusBar::extraBottomPadding()
{
    return StatusBarPrivate::getBottomPadding();
}
