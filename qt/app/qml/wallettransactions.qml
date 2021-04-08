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

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

Item
{
	id: transactions_

	property var walletModel;
	property var container;
	property var controller;

	x: 0
	y: 0
	width: parent.width
	height: parent.height

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

	onWalletModelChanged: {
		list.model = walletModel;
	}

	function disableAnmation() {
		if (list) {
			if (list.add) list.add.enabled = false;
			if (list.remove) list.remove.enabled = false;
			if (list.displaced) list.displaced.enabled = false;
		}
	}

	function enableAnmation() {
		if (list.add) list.add.enabled = true;
		if (list.remove) list.remove.enabled = true;
		if (list.displaced) list.displaced.enabled = true;
		adjustData();
	}

	function updateAgo() {
		for (var lIdx = 0; lIdx < list.count; lIdx++) {
			var lItem = list.itemAtIndex(lIdx);
			if (lItem) {
				walletModel.updateAgo(lItem.originalIndex);
			}
		}
	}

	QuarkListView {
		id: list
		focus: true
		currentIndex: -1
		x: 0
		y: 0

		width: parent.width
		height: parent.height
		clip: true

		property int fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : 14;

		onWidthChanged: {
		}

		add: Transition {
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
			NumberAnimation { property: "height"; from: 0; /*to: list.totalItemHeight;*/ duration: 400 }
		}

		remove: Transition {
			NumberAnimation { property: "opacity"; from: 1.0; to: 0; duration: 400 }
			NumberAnimation { property: "height"; /*from: list.totalItemHeight;*/ to: 0; duration: 400 }
		}

		displaced: Transition {
			NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.OutBounce }
		}

		onFeedReady: {
			//
			if (!walletModel.noMoreData) walletModel.feed(true);
		}

		delegate: ItemDelegate {
			id: txDelegate
			width: list.width
			leftPadding: 0
			rightPadding: 0

			property var originalIndex: index;

			onClicked: {
				list.currentIndex = index;
				// TODO: open transaction
			}

			Component.onDestruction: {
			}

			onHeightChanged: {
				bottomLine.adjust();
			}

			contentItem: Rectangle {
				id: containerItem
				color: "transparent"
				width: list.width

				onHeightChanged: {
					bottomLine.adjust();
				}

				Component.onCompleted: {
					//
					if (!confirmed && direction === 2 /*out*/) {
						checkOnChain.start();
					}
				}

				//
				// time
				//

				QuarkSymbolLabel {
					id: timeSymbol
					x: spaceItems_
					y: spaceItems_
					font.pointSize: list.fontPointSize
					symbol: Fonts.clockSym
				}

				QuarkLabel {
					id: timeLabel
					x: timeSymbol.x + timeSymbol.width + spaceItems_
					y: spaceItems_
					text: getLocalDateTime()
					font.pointSize: list.fontPointSize

					function getLocalDateTime() {
						//
						return localDateTime;
					}
				}

				Rectangle {
					id: point0

					x: timeLabel.x + timeLabel.width + spaceItems_
					y: timeLabel.y + timeLabel.height / 2 - height / 2
					width: spaceItems_
					height: spaceItems_
					radius: width / 2

					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
					border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
				}

				QuarkLabel {
					id: agoLabel
					x: point0.x + point0.width + spaceItems_
					y: timeLabel.y
					font.pointSize: list.fontPointSize
					text: ago
				}

				//
				// confirms
				//

				QuarkSymbolLabel {
					id: onChainSymbol
					x: parent.width - (width + spaceItems_)
					y: timeLabel.y
					symbol: confirmed ? Fonts.checkedCircleSym : Fonts.clockSym
					font.pointSize: list.fontPointSize
					color: confirmed ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.done") :
									   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.wait");

					visible: true
				}

				QuarkSymbolLabel {
					id: privateSymbol
					x: onChainSymbol.x - (width + spaceItems_)
					y: onChainSymbol.y
					symbol: Fonts.eyeSlashSym
					font.pointSize: list.fontPointSize
					visible: parentType === buzzerClient.tx_SPEND_PRIVATE() || blinded
				}

				BuzzerCommands.LoadTransactionCommand {
					id: checkOnChainCommand

					onProcessed: {
						// tx, chain
						console.log("[checkOnChainCommand]: tx = " + tx);
						checkOnChainCommand.updateWalletInfo();
					}

					onTransactionNotFound: {
						checkOnChain.start();
					}

					onError: {
						checkOnChain.start();
					}
				}

				Timer {
					id: checkOnChain
					interval: 2000
					repeat: false
					running: false

					onTriggered: {
						checkOnChainCommand.process(txid, chain);
					}
				}

				//
				// transaction
				//

				QuarkSymbolLabel {
					id: txSymbol
					x: spaceItems_
					y: timeLabel.y + timeLabel.height + spaceItems_
					font.pointSize: list.fontPointSize
					symbol: Fonts.hashSym
				}

				QuarkLabelRegular {
					id: txLabel
					x: txSymbol.x + txSymbol.width + spaceItems_
					y: txSymbol.y
					text: txid
					width: parent.width - (x + spaceItems_ + txExpandSymbol.width + spaceItems_)
					elide: Text.ElideRight
					font.pointSize: list.fontPointSize
				}

				QuarkSymbolLabel {
					id: txExpandSymbol
					x: parent.width - (width + spaceItems_) - 2
					y: txSymbol.y
					font.pointSize: list.fontPointSize
					symbol: Fonts.expandSimpleDownSym
				}

				MouseArea {
					x: txLabel.x
					y: txLabel.y
					width: txExpandSymbol.x + txExpandSymbol.width + spaceItems_ - x
					height: txLabel.height

					onClicked: {
						txMenu.popup(parent.width - txMenu.width + spaceItems_, y + height + spaceItems_, txLabel.text);
					}
				}

				function hasParent() {
					if (parentId !== "" &&
							parentType !== buzzerClient.tx_SPEND() &&
							parentType !== buzzerClient.tx_SPEND_PRIVATE()) return true;
					return false;
				}

				function getHeight() {
					// amountLabel
					if (feeNumber.isVisible())
						return feeNumber.y + feeNumber.height + spaceItems_ + spaceBottom_;
					else if (timelockedNumber.isVisible())
						return timelockedNumber.y + timelockedNumber.height + spaceItems_ + spaceBottom_;
					return amountLabel.y + amountLabel.height + spaceItems_ + spaceBottom_;
				}

				function getAmountY() {
					if (addressLabel.isVisible()) return addressLabel.y + addressLabel.height + spaceTop_;
					if (hasParent()) return parentTxLabel.y + parentTxLabel.height + spaceTop_;
					return txLabel.y + txLabel.height + spaceTop_;
				}

				function calculateHeight() {
					var lHeight = getHeight();
					txDelegate.height = lHeight;
					containerItem.height = lHeight;
					bottomLine.adjust();
				}

				QuarkVLine {
					id: vLine
					x1: txSymbol.x + txSymbol.width / 2
					y1: txSymbol.y + txSymbol.height + 2
					x2: x1
					y2: txLabel.y + txLabel.height + txLabel.height / 2 + 2 + 3
					penWidth: 1
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledLine")

					visible: containerItem.hasParent()
				}

				QuarkHLine {
					id: hLine
					x1: txSymbol.x + txSymbol.width / 2
					y1: txLabel.y + txLabel.height + txLabel.height / 2 + 2 + 3
					x2: x1 + spaceLeft_
					y2: y1
					penWidth: 1
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledLine")

					visible: containerItem.hasParent()
				}

				Rectangle {
					id: parentTypeContainer
					x: parentTypeLabel.x - 3
					y: parentTypeLabel.y - 3
					width: parentTypeLabel.width + 6
					height: parentTypeLabel.height + 6
					radius: 5
					visible: containerItem.hasParent()
					color: getColor()

					function getColor() {
						//
						if (parentType === buzzerClient.tx_BUZZ_TYPE()) {
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.buzz");
						} else if (parentType === buzzerClient.tx_REBUZZ_TYPE() || parentType === buzzerClient.tx_REBUZZ_REPLY_TYPE()) {
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.rebuzz");
						} else if (parentType === buzzerClient.tx_BUZZ_REPLY_TYPE()) {
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.reply");
						} else if (parentType === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.mistrust");
						} else if (parentType === buzzerClient.tx_BUZZER_ENDORSE_TYPE()) {
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.endorse");
						} else if (parentType === buzzerClient.tx_BUZZER_TYPE()) {
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.buzzer");
						} else if (parentType === buzzerClient.tx_CUBIX_MEDIA_SUMMARY()) {
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.media");
						} else if (parentType === buzzerClient.tx_BUZZER_CONVERSATION()) {
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.conversation");
						} else if (parentType === buzzerClient.tx_BUZZER_MESSAGE() ||
								   parentType === buzzerClient.tx_BUZZER_MESSAGE_REPLY()) {
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.message");
						}

						return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.undefined");
					}
				}

				QuarkLabel {
					id: parentTypeLabel
					x: txSymbol.x + txSymbol.width / 2 + spaceLeft_ + spaceItems_
					y: (txLabel.y + txLabel.height + txLabel.height / 2 + 2 + 3) - parentTypeLabel.height / 2
					font.pointSize: list.fontPointSize
					text: getText()
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

					visible: containerItem.hasParent()

					function getText() {
						//
						if (parentType === buzzerClient.tx_BUZZ_TYPE()) {
							return "Buzz";
						} else if (parentType === buzzerClient.tx_REBUZZ_TYPE() || parentType === buzzerClient.tx_REBUZZ_REPLY_TYPE()) {
							return "Rebuzz";
						} else if (parentType === buzzerClient.tx_BUZZ_REPLY_TYPE()) {
							return "Reply";
						} else if (parentType === buzzerClient.tx_BUZZER_MISTRUST_TYPE()) {
							return "Mistrust";
						} else if (parentType === buzzerClient.tx_BUZZER_ENDORSE_TYPE()) {
							return "Endorse";
						} else if (parentType === buzzerClient.tx_BUZZER_TYPE()) {
							return "Buzzer";
						} else if (parentType === buzzerClient.tx_CUBIX_MEDIA_SUMMARY()) {
							return "Media";
						} else if (parentType === buzzerClient.tx_BUZZER_CONVERSATION()) {
							return "Chat";
						} else if (parentType === buzzerClient.tx_BUZZER_MESSAGE() || parentType === buzzerClient.tx_BUZZER_MESSAGE_REPLY()) {
							return "Message";
						}

						return parentType;
					}
				}

				QuarkLabelRegular {
					id: parentTxLabel
					x: parentTypeLabel.x + parentTypeLabel.width + spaceItems_
					y: parentTypeLabel.y
					text: parentId
					width: parent.width - (x + spaceItems_ + txParentExpandSymbol.width + spaceItems_)
					elide: Text.ElideRight
					font.pointSize: list.fontPointSize

					visible: containerItem.hasParent()
				}

				QuarkSymbolLabel {
					id: txParentExpandSymbol
					x: parent.width - (width + spaceItems_) - 2
					y: parentTypeLabel.y
					font.pointSize: list.fontPointSize
					symbol: Fonts.expandSimpleDownSym

					visible: containerItem.hasParent()
				}

				MouseArea {
					x: parentTxLabel.x
					y: parentTxLabel.y
					width: txParentExpandSymbol.x + txParentExpandSymbol.width + spaceItems_ - x
					height: parentTxLabel.height
					enabled: containerItem.hasParent()

					onClicked: {
						txMenu.popup(parent.width - txMenu.width + spaceItems_, y + height + spaceItems_, parentTxLabel.text);
					}
				}

				//
				// address
				//

				QuarkSymbolLabel {
					id: addresSymbol
					x: spaceItems_
					y: getY()
					font.pointSize: list.fontPointSize
					symbol: Fonts.tagSym

					visible: addressLabel.isVisible()

					function getY() {
						//
						if (parentTypeLabel.visible) return parentTypeLabel.y + parentTypeLabel.height + spaceItems_;
						return txLabel.y + txLabel.height + spaceItems_;
					}
				}

				QuarkLabelRegular {
					id: addressLabel
					x: addresSymbol.x + addresSymbol.width + spaceItems_
					y: addresSymbol.y
					text: address
					width: parent.width - (x + spaceItems_ /*+ txExpandSymbol.width + spaceItems_*/)
					elide: Text.ElideRight
					font.pointSize: list.fontPointSize

					visible: addressLabel.isVisible()

					function isVisible() {
						//
						if (address !== "" && (type === buzzerClient.tx_SPEND() || type === buzzerClient.tx_SPEND_PRIVATE())) {
							return true;
						}

						return false;
					}
				}

				//
				// amount
				//

				QuarkNumberLabel {
					id: amountLabel
					number: amount / buzzerClient.getQbitBase()
					fillTo: 8
					useSign: false
					units: ""
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 4)) :
														  list.fontPointSize + 5

					x: parent.width - (amountLabel.calculatedWidth + spaceItems_)
					y: containerItem.getAmountY()

					zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes")

					onYChanged: {
						containerItem.calculateHeight();
					}

					onNumberChanged: {
						containerItem.calculateHeight();
					}
				}

				QuarkSymbolLabel {
					id: directionLabel
					x: amountLabel.x - (width + spaceItems_)
					y: amountLabel.y
					symbol: timelocked ? Fonts.lockedSym : (direction == 1 ? Fonts.inboxInSym : Fonts.inboxOutSym)
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 2)) :
														  list.fontPointSize + 5
					color: getColor()

					function getColor() {
						//
						if (timelocked) {
							console.log("[directionLabel]: utxo = " + utxo);
							if (buzzerClient.isTimelockReached(utxo)) {
								timelockChecher.stop();
								return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4");
							}

							timelockChecher.start();
							return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.2");
						}

						return direction == 1 ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4") :
												buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.1");
					}
				}

				Timer {
					//
					id: timelockChecher
					interval: 5000
					repeat: true
					running: false

					onTriggered: {
						directionLabel.color = directionLabel.getColor();
					}
				}

				//
				// fee
				//

				QuarkLabel {
					id: feeLabel
					x: feeNumber.x - (width + spaceItems_)
					y: feeNumber.y
					text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.transactions.fee")
					font.pointSize: list.fontPointSize
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
					visible: feeNumber.isVisible()
				}

				QuarkLabel {
					id: feeNumber
					font.pointSize: list.fontPointSize
					x: qBitLabel.x - (width + spaceItems_)
					y: qBitLabel.y
					text: "" + fee
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
					visible: isVisible()

					function isVisible() {
						if (!timelockedNumber.isVisible() && fee > 0) return true;
						return false;
					}

					onYChanged: {
						containerItem.calculateHeight();
					}

					onTextChanged: {
						containerItem.calculateHeight();
					}
				}

				QuarkLabel {
					id: qBitLabel
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 3)) :
														  list.fontPointSize - 4
					x: amountLabel.x + amountLabel.calculatedWidth - width
					y: amountLabel.y + amountLabel.calculatedHeight + spaceItems_
					text: "qBIT"
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
					visible: feeNumber.isVisible()
				}

				//
				// if timelocked - target height
				//

				QuarkSymbolLabel {
					id: timelockedLabel
					x: timelockedNumber.x - (width + spaceItems_)
					y: timelockedNumber.y
					symbol: Fonts.targetMapSym
					font.pointSize: list.fontPointSize
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
					visible: timelockedNumber.isVisible()
				}

				QuarkLabel {
					id: timelockedNumber
					font.pointSize: list.fontPointSize
					x: amountLabel.x + amountLabel.calculatedWidth - width
					y: amountLabel.y + amountLabel.calculatedHeight + spaceItems_
					text: "" + timelock
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
					visible: isVisible()

					function isVisible() {
						if (timelocked > 0) return true;
						return false;
					}

					onYChanged: {
						containerItem.calculateHeight();
					}

					onTextChanged: {
						containerItem.calculateHeight();
					}
				}

				//
				// bottom line
				//

				QuarkHLine {
					id: bottomLine
					x1: 0
					y1: 0
					x2: parent.width
					y2: y1
					penWidth: 1
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
					visible: false

					function adjust() {
						bottomLine.y1 = txDelegate.height - (spaceItems_ + 3); //containerItem.height + spaceItems_ + 3;
						bottomLine.visible = true;
					}

					Timer {
						id: adjuster
						interval: 500
						repeat: true
						running: true

						onTriggered: {
							if (txDelegate.height - (spaceItems_ + 3) != bottomLine.y1 && bottomLine.y1 != 0) {
								//console.log("[onTriggered]: adjusted, txDelegate.height = " + txDelegate.height + ", bottomLine.y1 = " + bottomLine.y1);
								bottomLine.adjust();
							} else {
								//console.log("[onTriggered]: finished");
								adjuster.stop();
							}
						}
					}
				}

				//
				// Menu
				//
				QuarkPopupMenu {
					id: txMenu
					width: 135
					visible: false

					property var info;

					model: ListModel { id: model }

					Component.onCompleted: prepare()

					onClick: {
						if (key === "explore") {
							//
							var lUrl = buzzerApp.getExploreTxRaw();
							lUrl = lUrl.replace("{tx}", info);
							Qt.openUrlExternally(lUrl);
						} else if (key === "copy") {
							//
							clipboard.setText(info);
						}
					}

					function popup(nx, ny, info) {
						//
						txMenu.info = info;
						x = nx;
						y = ny;

						open();
					}

					function prepare() {
						if (model.count) return;

						model.append({
							key: "explore",
							keySymbol: Fonts.hashSym,
							name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.explore")});

						model.append({
							key: "copy",
							keySymbol: Fonts.clipboardSym,
							name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.copy")});
					}
				}
			}
		}
	}
}

