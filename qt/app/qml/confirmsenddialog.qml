import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import "qrc:/fonts"
import "qrc:/components"

Dialog {
	id: confirmSendDialog_
	modal: true
	focus: true
	title: "Buzzer"
	x: (parent.width - width) / 2
	y: parent.height / 3
	width: Math.min(parent.width, parent.height) - 50
	contentHeight: infoColumn.height
	standardButtons: Dialog.Yes | Dialog.Cancel

	property string label;

	Material.theme: buzzerClient.themeSelector === "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	function show(text) {
		textLabel.text = text;
		open();
    }

	Column {
		id: infoColumn
		spacing: 10

		QuarkLabel {
			id: textLabel
			font.pixelSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 3)) : (buzzerApp.defaultFontSize() + 5)
			wrapMode: Label.Wrap
			width: confirmSendDialog_.availableWidth
		}
	}
}
