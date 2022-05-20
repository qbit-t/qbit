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

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceSingleMedia_: 8
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4
	readonly property real defaultFontSize: 11

	Component.onCompleted: list.prepare()

	onWidthChanged: {
		if (buzzerApp.isDesktop) {
			list.adjustItems();
		}
	}

	Rectangle {
		id: walletIndicatorBackground
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, /*buzzerApp.isDesktop ? "Page.background" :*/
																										"Page.statusBar")
		x: 0
		y: buzzerApp.isDesktop ? (-1) * (height + 2) : 0
		width: parent.width
		height: walletIndicator.height

		visible: !buzzerApp.isDesktop
	}

	PageIndicator {
		id: walletIndicator
		count: list.count
		currentIndex: list.currentIndex

		x: parent.width / 2 - width / 2
		y: buzzerApp.isDesktop ? (-1) * (height + 2) : 0

		Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
	}

	//
	// wallet list
	//

	QuarkListView {
		id: list

		x: 0
		y: buzzerApp.isDesktop ? 0 : walletIndicator.y + walletIndicator.height
		width: parent.width
		height: buzzerApp.isDesktop ? parent.height : parent.height - (walletIndicator.y + walletIndicator.height)

		clip: true
		orientation: Qt.Horizontal
		layoutDirection:  Qt.LeftToRight
		snapMode: ListView.SnapOneItem

		model: ListModel { id: walletModel }

		onContentXChanged: {
			//
			if (contentX == 0) walletIndicator.currentIndex = 0;
			else walletIndicator.currentIndex = list.indexAt(list.contentX, 0);
		}

		function adjustItems() {
			//
			for (var lIdx = 0; lIdx < list.count; lIdx++) {
				var lItem = list.itemAtIndex(lIdx);
				if (lItem) {
					lItem.width = list.width;
					lItem.walletItem.adjustWidth(list.width);
				}
			}
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
