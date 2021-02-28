import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import "qrc:/fonts"
import "qrc:/components"

//
// toolbar
//
QuarkToolBar
{
	id: setupToolBar
	height: 40
	width: parent.width

	property int extraOffset: 0;
	property int totalHeight: height;

	Component.onCompleted: {
		localeCombo.prepare();
	}

	function activate()	{
		localeCombo.activate();
	}

	QuarkToolButton	{
		id: cancelButton
		Material.background: "transparent"
		visible: true
		// labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter
		symbol: window.isTop() === 1 ? Fonts.cancelSym : Fonts.leftArrowSym
		font.pointSize: 18

		onClicked: {
			if (window.isTop() === 1) window.close();
			window.popPage();
		}
	}

	QuarkToolButton	{
		id: themeButton
		symbol: getSymbol()
		Material.background: "transparent"
		visible: true
		//labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter
		font.pointSize: 16

		x: (cancelButton.x + cancelButton.width) - 5
		y: extraOffset

		onClicked: {
			if (symbol === Fonts.sunSym) {
				buzzerClient.setTheme("Nova", "light");
				buzzerClient.save();
			} else {
				buzzerClient.setTheme("Darkmatter", "dark");
				buzzerClient.save();
			}
		}

		function getSymbol() {
			if (buzzerClient.themeSelector == "light") return Fonts.moonSym;
			return Fonts.sunSym;
		}
	}

	/*
	Image {
		id: logo
		fillMode: Image.PreserveAspectFit
		width: 25
		x: parent.width / 2 - logo.width / 2
		y: extraOffset
		Layout.alignment: Qt.AlignCenter
		mipmap: true
		//source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "buzzer.logo")
		source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "lightning.logo")
	}
	*/

	QuarkToolButton	{
		id: networkButton
		symbol: Fonts.networkSym
		Material.background: "transparent"
		visible: true
		//labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter
		font.pointSize: 16

		x: (themeButton.x + themeButton.width) - 5
		y: extraOffset

		onClicked: {
			// TODO: dialog
			var lComponent = null;
			var lPage = null;
			lComponent = Qt.createComponent("qrc:/qml/peers.qml");
			if (lComponent.status === Component.Error) {
				showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(window);
				lPage.controller = window;

				addPage(lPage);
			}
		}
	}

	QuarkSimpleComboBox {
		id: localeCombo
		x: parent.width - width - 8
		y: extraOffset
		width: 50
		fontPointSize: 22
		itemLeftPadding: 8
		itemTopPadding: 4
		itemHorizontalAlignment: Text.AlignHCenter
		leftPadding: -3

		Material.background: "transparent"

		model: ListModel { id: languageModel_ }

		Component.onCompleted: {
		}

		indicator: Canvas {
		}

		clip: false

		onActivated: {
			var lEntry = languageModel_.get(localeCombo.currentIndex);
			if (lEntry.name !== buzzerClient.locale) {
				buzzerClient.locale = lEntry.id;
				buzzerClient.save();
			}
		}

		function prepare() {
			if (languageModel_.count) return;

			var lLanguages = buzzerApp.getLanguages().split("|");
			var lCurrentIdx = 0;
			for (var lIdx = 0; lIdx < lLanguages.length; lIdx++) {
				if (lLanguages[lIdx].length) {
					var lPair = lLanguages[lIdx].split(",");
					languageModel_.append({ id: lPair[1], name: lPair[0] });

					if (lPair[1] === buzzerClient.locale) lCurrentIdx = lIdx;
				}
			}

			localeCombo.currentIndex = lCurrentIdx;
		}
	}
}
