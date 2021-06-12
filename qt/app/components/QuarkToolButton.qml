// QuarkToolButton.qml

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

import "qrc:/fonts"

T.RoundButton
{
    id: button

    property alias symbol: button.text
    property string symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
	property alias symbolFont: label.font

	property int labelXOffset: 0;
	property int labelYOffset: 0;
	property real symbolFontPointSize: 18
	property real symbolRotation: 0

	font.family: Fonts.icons
    font.weight: Font.Normal
	font.pointSize: symbolFontPointSize

	implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
							implicitContentWidth + leftPadding + rightPadding)
	implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
							 implicitContentHeight + topPadding + bottomPadding)
	topInset: 6
	leftInset: 6
	rightInset: 6
	bottomInset: 6
	padding: 12
	spacing: 6

	icon.width: 24
	icon.height: 24
	icon.color: !enabled ? Material.hintTextColor :
		flat && highlighted ? Material.accentColor :
		highlighted ? Material.primaryHighlightedTextColor : Material.foreground

	Material.elevation: flat ? button.down || button.hovered ? 2 : 0
							: button.down ? 12 : 6

	// reflect theme changes
	Connections
	{
		target: buzzerClient
		function onThemeChanged()
		{
			Material.theme = buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
			Material.accent = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
			Material.background = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
			Material.foreground = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			Material.primary = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
		}
	}

	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	contentItem: Label
    {
        id: label
		topPadding: labelYOffset
        horizontalAlignment: Text.AlignHCenter
        text: button.symbol
        color: symbolColor
        font.pointSize: symbolFontPointSize
		antialiasing: buzzerApp.isDesktop ? false : antialiasing
		rotation: symbolRotation
    }

	background: Rectangle
	{
		implicitWidth: button.Material.buttonHeight
		implicitHeight: button.Material.buttonHeight

		radius: button.radius
		color: !button.enabled ? button.Material.buttonDisabledColor
				: button.checked || button.highlighted ? button.Material.highlightedButtonColor : button.Material.buttonColor

		Rectangle {
			width: parent.width
			height: parent.height
			radius: button.radius
			visible: button.hovered || button.visualFocus
			color: button.Material.rippleColor
		}

		Rectangle {
			width: parent.width
			height: parent.height
			radius: button.radius
			visible: button.down
			color: button.Material.rippleColor
		}

		layer.enabled: button.enabled && button.Material.buttonColor.a > 0
		layer.effect: QuarkElevationEffect {
			elevation: button.Material.elevation
			explicitHeight: (button.height * 60) / 100
			explicitWidth: (button.width * 60) / 100
		}
	}
}


