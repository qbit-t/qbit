// QuarkRoundSymbolButton.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls.impl 2.12
import QtQuick.Templates 2.12 as T
import QtQuick.Controls.Material 2.12
import QtQuick.Controls.Material.impl 2.12
import QtGraphicalEffects 1.0

import "qrc:/fonts"

Rectangle
{
	id: frame

	property string symbol;
	property int fontPointSize: 14
	property int spaceLeft: 0
	property int spaceRight: 0
	property int spaceTop: 0
	property int spaceBottom: 0
	property bool enabled: true
	property int defaultRadius: 25
	property string textColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

	signal click();

	/*
	TextMetrics {
		id: textMetrics
		text: frame.text
		font.pointSize: fontPointSize
	}
	*/

	radius: defaultRadius
	width: radius * 2
	height: radius * 2
	color: enabled ? Material.buttonColor : Material.buttonDisabledColor

	Rectangle
	{
		id: innerFrame
		x: 0
		y: 0
		width: frame.width
		height: frame.height
		radius: frame.radius
		clip: true

		color: "transparent"

		QuarkSymbolLabel {
			id: label
			anchors.centerIn: parent
			leftPadding: spaceLeft
			topPadding: spaceTop
			//x: frame.spaceLeft
			//y: frame.spaceTop
			//width: textMetrics.boundingRect.width
			color: frame.textColor
			symbol: frame.symbol
			font.pointSize: fontPointSize
		}

		layer.enabled: true
		layer.effect: OpacityMask {
			maskSource: Item {
				width: frame.width
				height: frame.height

				Rectangle {
					anchors.centerIn: parent
					width: frame.width
					height: frame.height
					radius: frame.radius
				}
			}
		}

		MouseArea {
			x: 0
			y: 0
			width: frame.width
			height: frame.height
			enabled: true
			hoverEnabled: true

			//onEntered: item.hovered = true;
			//onExited: item.hovered = false;

			ItemDelegate {
				id: item
				x: 0
				y: 0
				width: frame.width
				height: frame.height

				onClicked: {
					if (enabled) click();
				}
			}
		}
	}

	// reflect theme changes
	Connections
	{
		target: buzzerClient
		function onThemeChanged()
		{
			Material.theme = buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
			Material.accent = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
			Material.background = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.background");
			Material.foreground = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground");
			Material.primary = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
		}
	}

	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
}


