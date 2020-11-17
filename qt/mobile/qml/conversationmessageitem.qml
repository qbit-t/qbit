﻿import QtQuick 2.15
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

import "qrc:/lib/numberFunctions.js" as NumberFunctions

Item {
	id: conversationmessageitem_

	property int calculatedHeight: calculateHeightInternal()
	property var type_: type
	property var buzzId_: buzzId
	property var buzzChainId_: buzzChainId
	property var timestamp_: timestamp
	property var ago_: ago
	property var localDateTime_: localDateTime
	property var score_: score
	property var buzzerId_: buzzerId
	property var buzzerInfoId_: buzzerInfoId
	property var buzzerInfoChainId_: buzzerInfoChainId
	property var buzzBody_: buzzBody
	property var buzzMedia_: buzzMedia
	property var replies_: replies
	property var rebuzzes_: rebuzzes
	property var likes_: likes
	property var rewards_: rewards
	property var originalBuzzId_: originalBuzzId
	property var originalBuzzChainId_: originalBuzzChainId
	property var hasParent_: hasParent
	property var value_: value
	property var buzzInfos_: buzzInfos
	property var buzzerName_: buzzerName
	property var buzzerAlias_: buzzerAlias
	property var childrenCount_: childrenCount
	property var hasPrevSibling_: hasPrevSibling
	property var hasNextSibling_: hasNextSibling
	property var prevSiblingId_: prevSiblingId
	property var nextSiblingId_: nextSiblingId
	property var firstChildId_: firstChildId
	property var wrapped_: wrapped
	property var childLink_: false
	property var parentLink_: false
	property var lastUrl_: lastUrl
	property var accepted_: false
	property bool dynamic_: dynamic
	property bool onChain_: onChain

	property var controller_: controller
	property var listView_
	property var model_
	property var conversationId_

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceHalfItems_: 3
	readonly property int spaceMedia_: 10
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property real maxWidth: 0.8
	readonly property real minSpace: 0.2

	property bool myMessage_//: buzzerClient.getCurrentBuzzerId() === buzzerId_

	signal calculatedHeightModified(var value);

	onCalculatedHeightChanged: {
		calculatedHeightModified(calculatedHeight);
	}

	onAccepted_Changed: {
		//
		calculateHeight();
	}

	onConversationId_Changed: {
		//
		bodyControl.expand(buzzerClient.getCounterpartyKey(conversationId_));
	}

	onModel_Changed: {
		decryptCommand.model = model_;

		// in case of counterparty pkey was not cached
		if (buzzBody_ === "" && accepted_) {
			// try to decrypt
			decryptCommand.process(buzzId_);
		}
	}

	onBuzzerId_Changed: {
		//
		myMessage_ = buzzerClient.getCurrentBuzzerId() === buzzerId_;
		//adjust();
	}

	onMyMessage_Changed: {
		//
		adjust();
	}

	onWidthChanged: {
		//
		adjust();
	}

	Component.onCompleted: {
		if (!onChain_ && dynamic_) checkOnChain.start();
	}

	function calculateHeightInternal() {
		var lCalculatedInnerHeight = spaceItems_ + (spaceTop_ + bodyControl.height +
				(decryptButton.visible ? decryptButton.height : 0) + spaceBottom_ +
													(!decryptButton.visible ? spaceItems_ * 2 : 0)) + spaceItems_;
		return lCalculatedInnerHeight;
	}

	function calculateHeight() {
		calculatedHeight = calculateHeightInternal();
		return calculatedHeight;
	}

	function conversationMessage() {
		//
		if (!accepted_ && buzzerClient.getCurrentBuzzerId() !== buzzerId_) {
			return buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.message.encrypted");
		}

		return buzzBody_;
	}

	function adjust() {
		//
		if (width > 0) {
			backgroundContainer.x = myMessage_ ? messageMetrics.getX(width) : spaceLeft_;
			backgroundContainer.width = messageMetrics.getWidth(width);
			buzzItemContainer.x = myMessage_ ? messageMetrics.getX(width) : spaceLeft_;
			buzzItemContainer.width = messageMetrics.getWidth(width);
		}
	}

	BuzzerCommands.DecryptBuzzerMessageCommand {
		id: decryptCommand

		onProcessed: {
			// key, body
			buzzText.text = body;
			messageMetrics.text = body;
			//
			calculateHeight();
			adjust();

			bodyControl.expand(key);

			//
			decryptButton.enabled = false;
		}
	}

	BuzzerCommands.LoadTransactionCommand {
		id: checkOnChainCommand

		onProcessed: {
			// tx, chain
			model_.setOnChain(index);
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
			checkOnChainCommand.process(buzzId_, buzzChainId_);
		}
	}

	//
	// text metrics
	//
	TextMetrics {
		id: messageMetrics
		font.family: buzzText.font.family
		text: conversationMessage()

		function getX(pwidth) {
			//
			if (buzzMedia_.length || lastUrl_ && lastUrl_.length) return pwidth * minSpace;

			//
			if (boundingRect.width > pwidth * maxWidth - (spaceLeft_ /*+ spaceRight_*/)) {
				return pwidth * minSpace;
			}

			return pwidth - (boundingRect.width + spaceLeft_ + spaceRight_ + spaceItems_ + spaceRight_);
		}

		function getWidth(pwidth) {
			//
			if (buzzMedia_.length || lastUrl_ && lastUrl_.length) return pwidth * maxWidth - spaceRight_;

			//
			if (boundingRect.width > (pwidth * maxWidth - (spaceLeft_ /*+ spaceRight_*/))) {
				return pwidth * maxWidth - (spaceLeft_ /*+ spaceRight_*/);
			}

			return boundingRect.width + spaceLeft_ + spaceRight_ + spaceItems_ /*extra*/;
		}
	}

	//
	// header info
	//

	QuarkRoundRectangle {
		//
		id: backgroundContainer
		//x: myMessage_ ? messageMetrics.getX() : spaceLeft_
		y: spaceItems_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		backgroundColor: myMessage_ ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.message.my") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.message.other")
		//width: messageMetrics.getWidth()
		height: calculatedHeight - (spaceItems_ + spaceItems_)
		bottomLeftCorner: myMessage_
		bottomRightCorner: !myMessage_
		radius: 8

		visible: true
	}

	Rectangle {
		//
		id: buzzItemContainer
		//x: myMessage_ ? messageMetrics.getX() : spaceLeft_
		y: spaceItems_
		color: "transparent"
		//width: messageMetrics.getWidth()
		height: calculatedHeight - (spaceItems_ + spaceItems_)
		radius: 8

		visible: true

		clip: true

		//
		// state
		//
		QuarkLabel {
			id: agoControl
			x: parent.width - (width + spaceItems_)
			y: bodyControl.y + bodyControl.height + spaceHalfItems_
			text: ago_
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
			font.pointSize: 14
		}

		QuarkSymbolLabel {
			id: onChainSymbol
			x: parent.width - (width + spaceHalfItems_ + 1)
			y: spaceHalfItems_
			symbol: !onChain_ ? Fonts.clockSym : Fonts.checkedCircleSym //linkSym
			font.pointSize: 12
			color: !onChain_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.wait") :
							   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.done");

			visible: dynamic_
		}

		//
		// body
		//

		Rectangle {
			id: bodyControl
			x: spaceLeft_
			y: spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			height: getHeight()

			border.color: "transparent"
			color: "transparent"

			Component.onCompleted: {
				// expand();
				conversationmessageitem_.calculateHeight();
			}

			property var buzzMediaItem_;
			property var urlInfoItem_;

			onWidthChanged: {
				expand(buzzerClient.getCounterpartyKey(conversationId_));

				if (buzzMediaItem_ && bodyControl.width > 0) {
					buzzMediaItem_.calculatedWidth = bodyControl.width;
				}

				if (urlInfoItem_ && bodyControl.width > 0) {
					urlInfoItem_.calculatedWidth = bodyControl.width;
				}

				conversationmessageitem_.calculateHeight();
			}

			onHeightChanged: {
				expand(buzzerClient.getCounterpartyKey(conversationId_));
				conversationmessageitem_.calculateHeight();
			}

			QuarkLabel {
				id: buzzText
				x: 0
				y: 0
				width: parent.width
				text: conversationMessage()
				wrapMode: Text.Wrap
				textFormat: Text.RichText
				font.italic: !accepted_

				onLinkActivated: {
					var lComponent = null;
					var lPage = null;
					//
					if (link[0] === '@') {
						// buzzer
						lComponent = Qt.createComponent("qrc:/qml/buzzfeedbuzzer.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(controller);
							lPage.controller = controller;
							lPage.start(link);
							addPage(lPage);
						}
					} else if (link[0] === '#') {
						// tag
						lComponent = Qt.createComponent("qrc:/qml/buzzfeedtag.qml");
						if (lComponent.status === Component.Error) {
							showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(controller);
							lPage.controller = controller;
							lPage.start(link);
							addPage(lPage);
						}
					} else {
						Qt.openUrlExternally(link);
					}
				}

				onHeightChanged: {
					if (bodyControl.buzzMediaItem_) {
						bodyControl.buzzMediaItem_.y = bodyControl.getY();
						bodyControl.height = bodyControl.getHeight();
					}

					if (bodyControl.urlInfoItem_) {
						bodyControl.urlInfoItem_.y = bodyControl.getY();
						bodyControl.height = bodyControl.getHeight();
					}

					conversationmessageitem_.calculateHeight();
				}

				/*
				TextMetrics {
					id: buzzBodyMetrics
					font.family: buzzText.font.family

					text: buzzBody
				}
				*/
			}

			function expand(key) {
				//
				var lSource;
				var lComponent;

				//
				if (!accepted_ && key === "") return;

				// expand media
				if (buzzMedia_.length && key !== "") {
					if (!buzzMediaItem_) {
						//
						lSource = "qrc:/qml/buzzitemmedia.qml";
						lComponent = Qt.createComponent(lSource);
						buzzMediaItem_ = lComponent.createObject(bodyControl);
						buzzMediaItem_.calculatedHeightModified.connect(innerHeightChanged);

						buzzMediaItem_.x = 0;
						buzzMediaItem_.y = bodyControl.getY();
						buzzMediaItem_.calculatedWidth = bodyControl.width;
						buzzMediaItem_.width = bodyControl.width;
						buzzMediaItem_.controller_ = conversationmessageitem_.controller_;
						buzzMediaItem_.buzzMedia_ = conversationmessageitem_.buzzMedia_;
						buzzMediaItem_.initialize(key);

						bodyControl.height = bodyControl.getHeight();
						conversationmessageitem_.calculateHeight();
					}
				} else if (lastUrl_ && lastUrl_.length) {
					//
					if (!urlInfoItem_) {
						lSource = "qrc:/qml/buzzitemurl.qml";
						lComponent = Qt.createComponent(lSource);
						urlInfoItem_ = lComponent.createObject(bodyControl);
						urlInfoItem_.calculatedHeightModified.connect(innerHeightChanged);

						urlInfoItem_.x = 0;
						urlInfoItem_.y = bodyControl.getY();
						urlInfoItem_.calculatedWidth = bodyControl.width;
						urlInfoItem_.width = bodyControl.width;
						urlInfoItem_.controller_ = conversationmessageitem_.controller_;
						urlInfoItem_.lastUrl_ = conversationmessageitem_.lastUrl_;
						urlInfoItem_.initialize();

						bodyControl.height = bodyControl.getHeight();
						conversationmessageitem_.calculateHeight();
					}
				}
			}

			function innerHeightChanged(value) {				
				bodyControl.height = (buzzBody_.length > 0 ? buzzText.height : 0) + value +
											(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
											(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0);
				conversationmessageitem_.calculateHeight();
			}

			function getY() {
				return (buzzBody_.length > 0 ? buzzText.height : 0) +
						(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
						(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0);
			}

			function getHeight() {
				return (buzzBody_.length > 0 || !accepted_ ? buzzText.height : 0) +
						(buzzMediaItem_ ? buzzMediaItem_.calculatedHeight : 0) +
						(urlInfoItem_ ? urlInfoItem_.calculatedHeight : 0) +
						(buzzBody_.length > 0 && buzzMedia_.length ? spaceMedia_ : 0 /*spaceItems_*/) +
						(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0 /*spaceBottom_*/);
			}

			MouseArea {
				id: menuItem
				x: 0
				y: 0
				width: parent.width
				height: parent.height

				onClicked: {
					//
					if (headerMenu.visible) headerMenu.close();
					else { headerMenu.prepare(); headerMenu.open(); }
				}
			}
		}

		//
		// buttons
		//

		QuarkToolButton	{
			id: decryptButton
			x: bodyControl.x - spaceLeft_
			y: bodyControl.y + bodyControl.getHeight() + spaceBottom_
			symbol: Fonts.keySym
			Material.background: "transparent"
			visible: !accepted_ && buzzerClient.getCurrentBuzzerId() !== buzzerId_
			labelYOffset: 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			font.family: Fonts.icons

			onClicked: {
				//
				decryptCommand.process(buzzId_);
			}
		}

		layer.enabled: true
		layer.effect: OpacityMask {
			maskSource: Item {
				width: buzzItemContainer.width
				height: buzzItemContainer.height

				Rectangle {
					anchors.centerIn: parent
					width: buzzItemContainer.width
					height: buzzItemContainer.height
					radius: buzzItemContainer.radius
				}
			}
		}
	}

	//
	// menus
	//

	QuarkPopupMenu {
		id: headerMenu
		x: buzzItemContainer.x + width > parent.width ? parent.width - (width + spaceRight_ + spaceLeft_) :
														buzzItemContainer.x + spaceLeft_
		y: spaceTop_ + spaceItems_
		width: 150
		visible: false

		model: ListModel { id: menuModel }

		Component.onCompleted: prepare()

		onClick: {
			//
			if (key === "copy") {
				clipboard.setText(buzzText.text);
			} else if (key === "copytx") {
				clipboard.setText(buzzId_);
			}
		}

		function prepare() {
			//
			menuModel.clear();

			//
			menuModel.append({
				key: "copy",
				keySymbol: Fonts.copySym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.copy.message")});
			menuModel.append({
				key: "copytx",
				keySymbol: Fonts.clipboardSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.copytransaction")});
		}
	}
}