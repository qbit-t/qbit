// QuarkEmojiPopup.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4
import QtQuick 2.15

import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

QuarkPopup {
	id: popupBox

	Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
	Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
	Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
	Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

	property var model;
	signal click(var key);

	implicitHeight: contentItem.implicitHeight
	padding: 0

	signal emojiSelected(var emoji);

	contentItem: Rectangle {
		x: 0
		y: 0
		width: parent.width
		height: parent.height
		color: "transparent"

		TabBar {
			id: bar
			x: 0
			y: 0
			width: parent.width
			currentIndex: 0
			position: TabBar.Header

			onCurrentIndexChanged: {
				//
				if (currentIndex == 0 /*favorites*/) {
					favoritesTable.resetModel();
				}
			}

			TabButton {
				QuarkSymbolLabel {
					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
					symbol: Fonts.clockSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : font.pointSize
				}
			}
			TabButton {
				QuarkSymbolLabel {
					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
					symbol: Fonts.peopleEmojiSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : font.pointSize
				}
			}
			TabButton {
				QuarkSymbolLabel {
					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
					symbol: Fonts.natureEmojiSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : font.pointSize
				}
			}
			TabButton {
				QuarkSymbolLabel {
					id: eventsSymbol
					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
					symbol: Fonts.foodEmojiSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : font.pointSize
				}
			}
			TabButton {
				QuarkSymbolLabel {
					id: messagesSymbol
					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
					symbol: Fonts.activityEmojiSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : font.pointSize
				}
			}
			TabButton {
				QuarkSymbolLabel {
					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
					symbol: Fonts.placesEmojiSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : font.pointSize
				}
			}
			TabButton {
				QuarkSymbolLabel {
					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
					symbol: Fonts.objectsEmojiSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : font.pointSize
				}
			}
			TabButton {
				QuarkSymbolLabel {
					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
					symbol: Fonts.symbolsEmojiSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : font.pointSize
				}
			}
			TabButton {
				QuarkSymbolLabel {
					x: parent.width / 2 - width / 2
					y: parent.height / 2 - height / 2
					symbol: Fonts.flagsEmojiSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 18) : font.pointSize
				}
			}

			Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
			Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabInactive");
		}

		StackLayout {
			id: pages
			currentIndex: bar.currentIndex

			QuarkEmojiTable {
				id: favoritesTable
				x: 0
				y: bar.height
				height: popupBox.height - y
				width: popupBox.width

				model: BuzzerCommands.EmojiModel {
					id: favModel
					category: "favorites"
				}

				onEmojiSelected: {
					popupBox.emojiSelected(emoji);
				}

				Component.onCompleted: {
					if (favModel.rows() > 0) bar.currentIndex = 0;
					else bar.currentIndex = 1;
				}

				function resetModel() {
					model.resetModel();
				}
			}

			QuarkEmojiTable {
				x: 0
				y: bar.height
				height: popupBox.height - y
				width: popupBox.width

				model: BuzzerCommands.EmojiModel {
					category: "people"
				}

				onEmojiSelected: {
					popupBox.emojiSelected(emoji);
				}
			}

			QuarkEmojiTable {
				x: 0
				y: bar.height
				height: popupBox.height - y
				width: popupBox.width

				model: BuzzerCommands.EmojiModel {
					category: "nature"
				}

				onEmojiSelected: {
					popupBox.emojiSelected(emoji);
				}
			}

			QuarkEmojiTable {
				x: 0
				y: bar.height
				height: popupBox.height - y
				width: popupBox.width

				model: BuzzerCommands.EmojiModel {
					category: "foods"
				}

				onEmojiSelected: {
					popupBox.emojiSelected(emoji);
				}
			}

			QuarkEmojiTable {
				x: 0
				y: bar.height
				height: popupBox.height - y
				width: popupBox.width

				model: BuzzerCommands.EmojiModel {
					category: "activity"
				}

				onEmojiSelected: {
					popupBox.emojiSelected(emoji);
				}
			}

			QuarkEmojiTable {
				x: 0
				y: bar.height
				height: popupBox.height - y
				width: popupBox.width

				model: BuzzerCommands.EmojiModel {
					category: "places"
				}

				onEmojiSelected: {
					popupBox.emojiSelected(emoji);
				}
			}

			QuarkEmojiTable {
				x: 0
				y: bar.height
				height: popupBox.height - y
				width: popupBox.width

				model: BuzzerCommands.EmojiModel {
					category: "objects"
				}

				onEmojiSelected: {
					popupBox.emojiSelected(emoji);
				}
			}

			QuarkEmojiTable {
				x: 0
				y: bar.height
				height: popupBox.height - y
				width: popupBox.width

				model: BuzzerCommands.EmojiModel {
					category: "symbols"
				}

				onEmojiSelected: {
					popupBox.emojiSelected(emoji);
				}
			}

			QuarkEmojiTable {
				x: 0
				y: bar.height
				height: popupBox.height - y
				width: popupBox.width

				model: BuzzerCommands.EmojiModel {
					category: "flags"
				}

				onEmojiSelected: {
					popupBox.emojiSelected(emoji);
				}
			}
		}
	}
}

