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
	id: walletlist_

	property var infoDialog;
	property var controller;

	Component.onCompleted: list.prepare()

	PageIndicator {
		id: walletIndicator
		count: list.count
		currentIndex: list.currentIndex

		x: parent.width / 2 - width / 2
		y: 0

		Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
	}

	//
	// wallet list
	//

	QuarkListView {
		id: list

		x: 0
		y: walletIndicator.y + walletIndicator.height
		width: parent.width
		height: parent.height - walletIndicator.height

		clip: true
		orientation: Qt.Horizontal
		layoutDirection:  Qt.LeftToRight
		snapMode: ListView.SnapOneItem
		//highlightFollowsCurrentItem: true
		//highlightMoveDuration: -1
		//highlightMoveVelocity: -1

		model: ListModel { id: walletModel }

		//cacheBuffer: 10000

		onContentXChanged: {
			//
			if (contentX == 0) walletIndicator.currentIndex = 0;
			else walletIndicator.currentIndex = list.indexAt(list.contentX, 0);
		}

		delegate: Rectangle {
			//
			id: walletFrame
			color: "transparent"
			width: list.width
			height: list.height

			property var walletItem;

			Component.onCompleted: {
				//
				var lComponent;
				var lSource = "qrc:/qml/wallet.qml";
				lComponent = Qt.createComponent(lSource);
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				}

				walletFrame.walletItem = lComponent.createObject(walletFrame);
				walletFrame.walletItem.x = 0;
				walletFrame.walletItem.y = 0;
				walletFrame.walletItem.width = list.width;
				walletFrame.walletItem.height = list.height;

				walletFrame.walletItem.infoDialog = infoDialog;
				walletFrame.walletItem.controller = controller;
				walletFrame.walletItem.init();
			}
		}

		function addWallet(asset, lifePulse, scale, scaleNumber, unit, nanounit, scaleSend, nanounitSend, balanceBackground) {
			walletModel.append({
				asset_: asset,
				lifePulse_: lifePulse,
				scale_: scale,
				scaleNumber_: scaleNumber,
				unit_: unit,
				nanounit_: nanounit,
				scaleSend_: scaleSend,
				nanounitSend_: nanounitSend,
				balanceBackground_: balanceBackground});
		}

		function prepare() {
			if (list.count) return;

			//
			addWallet("*", true, 8, 100000000, "QBIT", "qBIT", 4, "qBIT", "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "wallet.balance.qbit"));
			addWallet(buzzerApp.qttAsset(), false, 0, 1, "QTT", "QTT", 0, "qBIT", "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "wallet.balance.qtt"));
		}
	}
}
