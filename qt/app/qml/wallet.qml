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
	property var offset_: 50;

	function init() {
		walletBalance.init();
		walletSend.init();
		walletPages.y = 1; // trick
	}

	// TODO: adjust for mobile?
	function adjustWidth(w) {
		wallet_.width = w;
		walletBar.width = w;
		walletPages.width = w;
		walletBalance.width = w;
		walletReceive.width = w;
		walletSend.width = w;
	}

	// TODO: adjust for mobile?
	function adjustHeight(h) {
		//
		wallet_.height = h;
		walletPages.height = h - (walletBar.y + offset_);
		walletBalance.height = h - (walletBar.y + offset_);
		walletReceive.height = h - (walletBar.y + offset_);
		walletSend.height = h - (walletBar.y + offset_);
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

			walletPages.y = offset_;
			walletBalance.y = offset_;
			walletReceive.y = offset_;
			walletSend.y = offset_;
		}

		Component.onCompleted: {
		}

		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabInactive");
	}

	StackLayout {
		id: walletPages
		anchors.top: walletBar.bottom
		currentIndex: walletBar.currentIndex
		width: wallet_.width
		height: wallet_.height - (walletBar.y + offset_)

		WalletBalance {
			id: walletBalance
			anchors.fill: parent
			controller: wallet_.controller
		}
		WalletReceive {
			id: walletReceive
			anchors.fill: parent
			controller: wallet_.controller
		}
		WalletSend {
			id: walletSend
			anchors.fill: parent
			controller: wallet_.controller
		}
	}
}
