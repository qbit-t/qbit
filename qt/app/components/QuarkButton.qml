// QuarkButton.qml

import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Controls.Material.impl 2.12

T.Button {
	id: control

	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
							implicitContentWidth + leftPadding + rightPadding)
	implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
							 implicitContentHeight + topPadding + bottomPadding)

	property real defaultFontPointSize: buzzerApp.isDesktop ? buzzerApp.defaultFontSize() : (buzzerApp.defaultFontSize() + 5)

	font.pointSize: defaultFontPointSize

	topInset: 6
	bottomInset: 6
	padding: 12
	horizontalPadding: padding - 4
	spacing: 6

	icon.width: 24
	icon.height: 24
	icon.color: !enabled ? Material.hintTextColor :
		flat && highlighted ? Material.accentColor :
		highlighted ? Material.primaryHighlightedTextColor : Material.foreground

	Material.elevation: flat ? control.down || control.hovered ? 2 : 0
							 : control.down ? 8 : 2
	// Material.background: flat ? "transparent" : undefined

	contentItem: IconLabel {
		spacing: control.spacing
		mirrored: control.mirrored
		display: control.display

		icon: control.icon
		text: control.text
		font: control.font
		color: !control.enabled ? control.Material.hintTextColor :
			control.flat && control.highlighted ? control.Material.accentColor :
			control.highlighted ? control.Material.primaryHighlightedTextColor : control.Material.foreground
	}

	background: Rectangle {
		implicitWidth: 64
		implicitHeight: control.Material.buttonHeight

		radius: 2
		color: !control.enabled ? control.Material.buttonDisabledColor :
				control.highlighted ? control.Material.highlightedButtonColor : control.Material.buttonColor
		PaddedRectangle {
			y: parent.height - 4
			width: parent.width
			height: 4
			radius: 2
			topPadding: -2
			clip: true
			visible: control.checkable && (!control.highlighted || control.flat)
			color: control.checked && control.enabled ? control.Material.accentColor : control.Material.secondaryTextColor
		}

		// The layer is disabled when the button color is transparent so you can do
		// Material.background: "transparent" and get a proper flat button without needing
		// to set Material.elevation as well
		layer.enabled: control.enabled && control.Material.buttonColor.a > 0
		layer.effect: ElevationEffect {
			elevation: control.Material.elevation
		}

		Ripple {
			clipRadius: 2
			//width: parent.width
			//height: parent.height
			pressed: control.pressed
			//anchor: control
			x: 0
			y: 0
			width: parent.width
			height: parent.height
			active: control.down || control.visualFocus || control.hovered
			color: control.flat && control.highlighted ? control.Material.highlightedRippleColor : control.Material.rippleColor
		}
	}
}


