import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0
import Qt.labs.folderlistmodel 2.11
import QtMultimedia 5.8
import QtQuick.Window 2.15

import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

Item
{
	id: wallet_

	property var infoDialog;
	property var controller;

	function init() {
		walletBalance.init();
		walletSend.init();
	}

	function adjustWidth(width) {
		walletBar.width = width;
		walletPages.width = width;
		walletBalance.width = width;
		walletReceive.width = width;
		walletSend.width = width;
	}

	TabBar {
		id: walletBar
		x: 0
		y: 0
		width: parent.width
		currentIndex: 0
		position: TabBar.Header

		TabButton {
			QuarkSymbolLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.tipSym
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : defaultFontSize
			}
		}
		TabButton {
			QuarkSymbolLabel {
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.inboxInSym
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : defaultFontSize
			}
		}
		TabButton {
			QuarkSymbolLabel {
				id: eventsSymbol
				x: parent.width / 2 - width / 2
				y: parent.height / 2 - height / 2
				symbol: Fonts.inboxOutSym
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : defaultFontSize
			}
		}

		onCurrentIndexChanged: {
			wallet_.init(); // try to refill balances
		}

		Component.onCompleted: {
		}

		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabInactive");
	}

	StackLayout {
		id: walletPages
		currentIndex: walletBar.currentIndex

		WalletBalance {
			id: walletBalance
			x: 0
			y: walletBar.height
			width: wallet_.width
			height: wallet_.height - (walletBar.y + 50)
			controller: wallet_.controller
		}
		WalletReceive {
			id: walletReceive
			x: 0
			y: walletBar.height
			width: wallet_.width
			height: wallet_.height - (walletBar.y + 50)
			controller: wallet_.controller
		}
		WalletSend {
			id: walletSend
			x: 0
			y: walletBar.height
			width: wallet_.width
			height: wallet_.height - (walletBar.y + 50)
			controller: wallet_.controller
		}
	}
}
