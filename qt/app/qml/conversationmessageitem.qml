import QtQuick 2.15
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
	property var hasPrevLink_: hasPrevLink
	property var hasNextLink_: hasNextLink
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
	property bool adjustData_: adjustData

	property var controller_: controller
	property var listView_
	property var model_
	property var conversationId_
	property var sharedMediaPlayer_

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
	readonly property real defaultFontSize: 11

	property bool myMessage_
	property bool enableAvatar_: false
	property bool leftSide_: false

	signal calculatedHeightModified(var value);
	signal forceChangeCursorShape(var shape);

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

		//
		adjustAvatar();
	}

	onAdjustData_Changed: {
		//
		adjustAvatar();
	}

	onEnableAvatar_Changed: {
		//
		if (enableAvatar_) avatarDownloadCommand.process();
	}

	onLeftSide_Changed: {
		//
		if (enableAvatar_) avatarDownloadCommand.process();
	}

	onBuzzerId_Changed: {
		//
		myMessage_ = buzzerClient.getCurrentBuzzerId() === buzzerId_;
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

	function adjustAvatar() {
		if (model_) {
			var lNextBuzzerId = index > 0 ? model_.buzzerId(index - 1) : "";
			enableAvatar_ = (index == 0 || lNextBuzzerId !== "" && lNextBuzzerId !== buzzerId_);
		}
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
		if (listView_ && width > 0) {
			//
			leftSide_ = listView_.width > listView_.height && ((!buzzerApp.isDesktop && listView_.width - listView_.height > 100) ||
																	buzzerApp.isDesktop);
			//
			if (!leftSide_ /*listView_.width < listView_.height*/) {
				backgroundContainer.x = myMessage_ ? messageMetrics.getX(width) : spaceLeft_;
				backgroundContainer.width = messageMetrics.getWidth(width);
				buzzItemContainer.x = myMessage_ ? messageMetrics.getX(width) : spaceLeft_;
				buzzItemContainer.width = messageMetrics.getWidth(width);
			} else {
				backgroundContainer.x = avatarImage.x + avatarImage.width + spaceLeft_;
				backgroundContainer.width = messageMetrics.getWidth(width);
				buzzItemContainer.x = avatarImage.x + avatarImage.width + spaceLeft_;
				buzzItemContainer.width = messageMetrics.getWidth(width);
			}

			//
			adjustAvatar();
		}
	}

	function forceVisibilityCheck(isFullyVisible) {
		bodyControl.setFullyVisible(isFullyVisible);
	}

	function unbindCommonControls() {
		bodyControl.unbindCommonControls();
	}

	BuzzerCommands.DecryptBuzzerMessageCommand {
		id: decryptCommand

		onProcessed: {
			//
			console.info("[decryptCommand]: key = " + key + ", body = " + body + ", hasEmoji = " + hasEmoji);
			// key, body
			buzzText.text = body;
			messageMetrics.text = body;
			isEmoji = hasEmoji;
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
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize * multiplicator_) : buzzText.defaultFontPointSize * multiplicator_

		property real multiplicator_: isEmoji ? 2.5 : 1.0

		function getX(pwidth) {
			//
			var lWidth;
			if (buzzMedia_.length || (lastUrl_ && lastUrl_.length)) {
				lWidth = pwidth * maxWidth - spaceRight_;
				if (buzzMedia_.length && lWidth > 600)
					return pwidth - (600 + spaceRight_);
				else if (lastUrl_ && lastUrl_.length && lWidth > 500)
					return pwidth - (500 + spaceRight_);

				return pwidth * minSpace;
			}

			//
			if (boundingRect.width > pwidth * maxWidth - (spaceLeft_ /*+ spaceRight_*/)) {
				return pwidth * minSpace;
			}

			lWidth = boundingRect.width;
			if (lWidth == 0) lWidth = pwidth * maxWidth / 2;
			return pwidth - (lWidth + spaceLeft_ + spaceRight_ + spaceItems_ + spaceRight_);
		}

		function getWidth(pwidth) {
			//
			var lWidth;
			if (buzzMedia_.length || (lastUrl_ && lastUrl_.length)) {
				lWidth = pwidth * maxWidth - spaceRight_;
				if (buzzMedia_.length && lWidth > 600) lWidth = 600;
				else if (lastUrl_ && lastUrl_.length && lWidth > 500) lWidth = 500;
				return lWidth;
			}

			//
			if (boundingRect.width > (pwidth * maxWidth - (spaceLeft_ /*+ spaceRight_*/))) {
				return pwidth * maxWidth - (spaceLeft_ /*+ spaceRight_*/);
			}

			lWidth = boundingRect.width;
			if (lWidth == 0) lWidth = pwidth * maxWidth / 2;
			return lWidth + spaceLeft_ + spaceRight_ + spaceItems_ /*extra*/;
		}
	}

	//
	// avatar download
	//

	BuzzerCommands.DownloadMediaCommand {
		id: avatarDownloadCommand
		url: buzzerClient.getBuzzerAvatarUrl(buzzerInfoId_)
		localFile: buzzerClient.getTempFilesPath() + "/" + buzzerClient.getBuzzerAvatarUrlHeader(buzzerInfoId_)
		preview: true
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile
			avatarImage.source = "file://" + previewFile
		}
	}

	//
	// NOTICE: for mobile versions we should consider to use ImageQx
	//
	Image { //BuzzerComponents.ImageQx
		id: avatarImage

		x: spaceLeft_
		y: backgroundContainer.height - (height)

		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop
		mipmap: true
		//radius: avatarImage.displayWidth

		visible: enableAvatar_ && leftSide_
		enabled: enableAvatar_ && leftSide_

		property bool rounded: true //!
		property int displayWidth: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 40 : 40
		property int displayHeight: displayWidth

		autoTransform: true

		layer.enabled: rounded
		layer.effect: OpacityMask {
			maskSource: Item {
				width: avatarImage.displayWidth
				height: avatarImage.displayHeight

				Rectangle {
					anchors.centerIn: parent
					width: avatarImage.displayWidth
					height: avatarImage.displayHeight
					radius: avatarImage.displayWidth
				}
			}
		}

		MouseArea {
			id: buzzerInfoClick
			anchors.fill: parent
			cursorShape: Qt.PointingHandCursor
			hoverEnabled: true

			onEntered: {
				forceChangeCursorShape(Qt.PointingHandCursor);
			}

			onExited: {
				forceChangeCursorShape(Qt.ArrowCursor);
			}

			onClicked: {
				//
				controller_.openBuzzfeedByBuzzer(buzzerName_);
			}
		}
	}

	QuarkRoundProgress {
		id: imageFrame
		x: avatarImage.x - 2
		y: avatarImage.y - 2
		size: avatarImage.displayWidth + 4
		colorCircle: getColor()
		colorBackground: "transparent"
		arcBegin: 0
		arcEnd: 360
		lineWidth: buzzerClient.scaleFactor * 2
		beginAnimation: false
		endAnimation: false

		visible: enableAvatar_ && leftSide_
		enabled: enableAvatar_ && leftSide_

		function getColor() {
			var lScoreBase = buzzerClient.getTrustScoreBase() / 10;
			var lIndex = score_ / lScoreBase;

			// TODO: consider to use 4 basic colours:
			// 0 - red
			// 1 - 4 - orange
			// 5 - green
			// 6 - 9 - teal
			// 10 -

			switch(Math.round(lIndex)) {
				case 0: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.0");
				case 1: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.1");
				case 2: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.2");
				case 3: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.3");
				case 4: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4");
				case 5: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.5");
				case 6: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.6");
				case 7: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.7");
				case 8: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.8");
				case 9: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.9");
				default: return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.10");
			}
		}
	}

	//
	// header info
	//

	QuarkRoundRectangle {
		//
		id: backgroundContainer
		y: spaceItems_
		color: "transparent"
		backgroundColor: isEmoji ? "transparent" :
								   (conversationMessage().length ?
									 (myMessage_ ?
										 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.message.my") :
										 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.message.other")) :
									 "transparent")
		height: calculatedHeight - (spaceItems_ + spaceItems_)
		bottomLeftCorner: leftSide_ ? false : myMessage_
		bottomRightCorner: leftSide_ ? true : !myMessage_
		radius: 8

		visible: true
	}

	Rectangle {
		//
		id: buzzItemContainer
		y: spaceItems_
		color: "transparent"
		height: calculatedHeight - (spaceItems_ + spaceItems_)
		radius: 8

		visible: true

		clip: true

		//
		// body
		//

		Rectangle {
			id: bodyControl
			x: conversationMessage().length ? spaceLeft_ : 0
			y: conversationMessage().length ? spaceTop_ : 0
			width: parent.width - (conversationMessage().length ? (spaceLeft_ + spaceRight_) : 0)
			height: getHeight()

			border.color: "transparent"
			color: "transparent"

			Component.onCompleted: {
				// expand();
				conversationmessageitem_.calculateHeight();
			}

			property var buzzMediaItem_;
			property var urlInfoItem_;

			function setFullyVisible(fullyVisible) {
				if (buzzMediaItem_) buzzMediaItem_.forceVisibilityCheck(fullyVisible);
			}

			function unbindCommonControls() {
				if (buzzMediaItem_) buzzMediaItem_.unbindCommonControls();
			}

			function setSharedMediaPlayer() {
				if (buzzMediaItem_) {
					buzzMediaItem_.sharedMediaPlayer_ = conversationmessageitem_.sharedMediaPlayer_;
				}
			}

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

			QuarkTextEdit {
				id: buzzText
				x: 0
				y: 0
				width: parent.width
				text: conversationMessage()
				wrapMode: Text.Wrap
				textFormat: Text.RichText
				font.italic: !accepted_
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize * multiplicator_) : defaultFontPointSize * multiplicator_
				// lineHeight: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 1.1) : lineHeight
				readOnly: true
				selectByMouse: true
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
				selectionColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.selected")

				property real multiplicator_: isEmoji ? 2.5 : 1.0

				MouseArea {
					anchors.fill: buzzText
					cursorShape: buzzText.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
					acceptedButtons: Qt.RightButton

					onCursorShapeChanged: forceChangeCursorShape(cursorShape)

					onClicked: {
						//
						if (headerMenu.visible) headerMenu.close();
						else {
							headerMenu.prepare(buzzText.selectedText.length);
							headerMenu.popup(buzzItemContainer.x + mouseX, mouseY);
						}
					}
				}

				onLinkActivated: {
					//
					if (link[0] === '@') {
						// buzzer
						controller_.openBuzzfeedByBuzzer(link);
					} else if (link[0] === '#') {
						// tag
						controller_.openBuzzfeedByTag(link);
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
			}

			function expand(key) {
				//
				var lSource;
				var lComponent;

				//
				if (!accepted_ && key === "") return;
				if (!accepted_ && buzzerClient.getCurrentBuzzerId() !== buzzerId_) return;

				// expand media
				if (buzzMedia_.length && key !== "") {
					if (!buzzMediaItem_) {
						//
						lSource = "qrc:/qml/buzzitemmedia.qml";
						lComponent = Qt.createComponent(lSource);
						buzzMediaItem_ = lComponent.createObject(bodyControl);
						buzzMediaItem_.calculatedHeightModified.connect(innerHeightChanged);

						buzzMediaItem_.frameColor = myMessage_ ?
											 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.message.my") :
											 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.message.other");
						buzzMediaItem_.fillColor = myMessage_ ?
											 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.message.my") :
											 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.conversation.message.other");
						buzzMediaItem_.x = 0;
						buzzMediaItem_.y = bodyControl.getY();
						buzzMediaItem_.calculatedWidth = bodyControl.width;
						buzzMediaItem_.width = bodyControl.width;
						buzzMediaItem_.controller_ = conversationmessageitem_.controller_;
						buzzMediaItem_.buzzId_ = conversationmessageitem_.buzzId_;
						buzzMediaItem_.playerKey_ = conversationmessageitem_.conversationId_;
						buzzMediaItem_.buzzMedia_ = conversationmessageitem_.buzzMedia_;
						buzzMediaItem_.sharedMediaPlayer_ = conversationmessageitem_.sharedMediaPlayer_;
						buzzMediaItem_.initialize(key);

						bodyControl.height = bodyControl.getHeight();
						conversationmessageitem_.calculateHeight();
					}
				} else if (lastUrl_ && lastUrl_.length) {
					//
					if (!urlInfoItem_) {
						lSource = buzzerApp.isDesktop ? "qrc:/qml/buzzitemurl-desktop.qml" : "qrc:/qml/buzzitemurl.qml";
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
				//
				//bodyControl.height = (buzzBody_.length > 0 ? buzzText.height : 0) + value +
				//							(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
				//							(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0);
				bodyControl.height = bodyControl.getHeight();
				conversationmessageitem_.calculateHeight();
			}

			function getY() {
				return (buzzBody_.length > 0 ? buzzText.height : 0) +
						(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
						(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0);
			}

			function getHeight() {
				var lAdjust = 0;
						//buzzMedia_.length > 0 ||
						//buzzMediaItem_ ||
						//urlInfoItem_ && urlInfoItem_.calculatedHeight > 0 ? 0 : (buzzerClient.scaleFactor * 12);

				return (buzzBody_.length > 0 || !accepted_ ? buzzText.height - lAdjust : 0) +
						(buzzMediaItem_ ? buzzMediaItem_.calculatedHeight : 0) +
						(urlInfoItem_ ? urlInfoItem_.calculatedHeight + (buzzerApp.isDesktop ? spaceItems_ + 2 : spaceItems_) : 0) +
						(buzzBody_.length > 0 && buzzMedia_.length ? spaceMedia_ : 0 /*spaceItems_*/) +
						(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0 /*spaceBottom_*/);
			}

			MouseArea {
				id: menuItem
				x: 0
				y: 0
				width: parent.width
				height: parent.height
				enabled: !buzzerApp.isDesktop

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
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			font.family: Fonts.icons
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

			onClicked: {
				//
				decryptCommand.process(buzzId_);
			}
		}

		//
		// state
		//
		QuarkLabel {
			id: agoControl
			x: dynamic_ ? (onChainSymbol.x - (width + spaceItems_)) : (parent.width - (width + spaceItems_))
			y: bodyControl.y + bodyControl.height + (conversationMessage().length ? spaceHalfItems_ : spaceItems_)
			text: ago_
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 3)) : 14
		}

		QuarkSymbolLabel {
			id: onChainSymbol
			//x: agoControl.x - (width + spaceItems_)
			//y: agoControl.y + 2
			x: parent.width - (width + spaceItems_)
			y: bodyControl.y + bodyControl.height + (conversationMessage().length ? spaceHalfItems_ : spaceItems_) + 2
			symbol: !onChain_ ? Fonts.clockSym : Fonts.checkedCircleSym
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 10)) : 12
			color: !onChain_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.wait.chat") :
							   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.done.chat");

			visible: dynamic_
		}

		/*
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
		*/
	}

	//
	// menus
	//

	QuarkPopupMenu {
		id: headerMenu
		x: buzzItemContainer.x + width > parent.width ? parent.width - (width + spaceRight_ + spaceLeft_) :
														buzzItemContainer.x + spaceLeft_
		y: spaceTop_ + spaceItems_
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 170) : 150
		visible: false

		model: ListModel { id: menuModel }

		onAboutToShow: prepare()

		onClick: {
			//
			if (key === "copy") {
				clipboard.setText(buzzText.text);
			} else if (key === "copytx") {
				clipboard.setText(buzzId_);
			} else if (key === "copyselection") {
				clipboard.setText(buzzText.selectedText);
			}
		}

		function popup(nx, ny) {
			//
			if (nx !== undefined && nx + width > parent.width) nx = parent.width - (width + spaceRight_);

			x = (nx === undefined ? parent.width - width - spaceRight_ : nx);
			y = (ny === undefined ? menuControl.y + menuControl.height + spaceItems_ : ny);
			open();
		}

		function prepare(selection) {
			//
			menuModel.clear();

			//
			if (selection) menuModel.append({
				key: "copyselection",
				keySymbol: Fonts.copySym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.copyselection")});

			//
			if (!selection) menuModel.append({
				key: "copy",
				keySymbol: Fonts.copySym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation.copy.message")});
			//
			menuModel.append({
				key: "copytx",
				keySymbol: Fonts.clipboardSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.copytransaction")});
		}
	}
}
