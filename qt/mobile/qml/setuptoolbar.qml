﻿import QtQuick 2.9
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
	property int totalHeight: logo.height;

	Component.onCompleted: {
		localeCombo.prepare();
	}

	function activate()	{
		localeCombo.activate();
	}

	QuarkToolButton	{
		id: themeButton
		symbol: getSymbol()
		Material.background: "transparent"
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

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

	QuarkSimpleComboBox {
		id: localeCombo
		x: parent.width - width - 8
		y: extraOffset
		width: 40
		fontPointSize: 25
		itemLeftPadding: 8
		itemTopPadding: 8
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

		function activate() {
			if (!languageModel_.count) prepare();
			else {
				var lLanguages = buzzerApp.getLanguages().split("|");
				var lCurrentIdx = 0;
				for (var lIdx = 0; lIdx < lLanguages.length; lIdx++) {
					if (lLanguages[lIdx].length) {
						var lPair = lLanguages[lIdx].split(",");
						if (lPair[1] === buzzerClient.locale) lCurrentIdx = lIdx;
					}
				}

				localeCombo.currentIndex = lCurrentIdx;
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
