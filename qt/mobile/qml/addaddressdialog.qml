import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import "qrc:/fonts"
import "qrc:/components"

Dialog {
	id: addAddressDialog_
	modal: true
	focus: true
	title: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.addAddress")
	x: (parent.width - width) / 2
	y: parent.height / 3
	width: Math.min(parent.width, parent.height) - 50
	contentHeight: infoColumn.height
	standardButtons: Dialog.Save | Dialog.Cancel

	property string label;

	Material.theme: buzzerClient.themeSelector === "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	function show(buzzer, address) {
		labelEdit.text = buzzer;
		if (buzzer !== "" && buzzer[0] === '@') {
			label = buzzer;
			addressLabel.text = address;
		} else addressLabel.text = buzzer;

		open();
    }

	Column {
		id: infoColumn
		spacing: 10

		QuarkTextField {
			id: labelEdit
			width: addAddressDialog_.availableWidth
			placeholderText: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.address.label")

			onTextChanged: {
				label = labelEdit.text;
			}

			onTextEdited: {
				label = labelEdit.text;
			}
		}

		QuarkLabel {
			id: addressLabel
			font.pixelSize: 14
			wrapMode: Label.Wrap
			width: addAddressDialog_.availableWidth
		}
	}
}
