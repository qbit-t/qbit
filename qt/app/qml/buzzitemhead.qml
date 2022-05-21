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
	id: buzzitemhead_

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
	property var buzzBodyFlat_: buzzBodyFlat
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
	property var self_: self
	property var rootId_

	property var controller_: controller
	property var buzzfeedModel_: buzzfeedModel
	property var listView_
	property var sharedMediaPlayer_

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property real defaultFontSize: 11

	signal calculatedHeightModified(var value);

	onCalculatedHeightChanged: {
		calculatedHeightModified(calculatedHeight);
	}

	Component.onCompleted: {
		avatarDownloadCommand.process();
	}

	onSharedMediaPlayer_Changed: {
		bodyControl.setSharedMediaPlayer();
	}

	function calculateHeightInternal() {
		return bottomLine.y1;
		//spaceTop_ + headerInfo.getHeight() + avatarImage.displayHeight + spaceTop_ +
		//							bodyControl.height + spaceItems_ + localDateTimeControl.height + spaceBottom_ +
		//							replyButton.height + 1;
	}

	function calculateHeight() {
		calculatedHeight = calculateHeightInternal();
		return calculatedHeight;
	}

	function forceVisibilityCheck(isFullyVisible) {
		bodyControl.setFullyVisible(isFullyVisible);
	}

	function unbindCommonControls() {
		bodyControl.unbindCommonControls();
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
	// header info
	//
	Rectangle {
		id: headerInfo
		color: "transparent"
		x: 0
		y: spaceTop_
		width: parent.width
		height: actionText.height + spaceHeader_ - 2
		visible: getVisible()

		function getVisible() {
			return type_ === buzzerClient.tx_BUZZ_LIKE_TYPE() ||
					type_ === buzzerClient.tx_BUZZ_REWARD_TYPE() ||
					(type_ === buzzerClient.tx_REBUZZ_TYPE() && !wrapped_); // if rebuzz without comments
		}

		function getHeight() {
			if (getVisible()) {
				return height;
			}

			return 0;
		}

		QuarkSymbolLabel {
			id: actionSymbol
			x: avatarImage.x + avatarImage.width - width
			y: -1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			font.pointSize: 12
			symbol: getSymbol()

			function getSymbol() {
				if (type_ === buzzerClient.tx_BUZZ_LIKE_TYPE()) {
					return Fonts.likeSym;
				} else if (type_ === buzzerClient.tx_REBUZZ_TYPE()) {
					return Fonts.rebuzzSym;
				} else if (type_ === buzzerClient.tx_BUZZ_REPLY_TYPE()) {
					return Fonts.replySym;
				} else if (type_ === buzzerClient.tx_BUZZ_REWARD_TYPE()) {
					return Fonts.coinSym;
				}

				return "";
			}
		}

		QuarkLabelRegular {
			id: actionText
			x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
			y: -2
			font.pointSize: 12
			width: parent.width - x - spaceRight_
			elide: Text.ElideRight
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			text: getText()

			function getText() {
				var lInfo;
				var lString;
				var lResult;

				console.log("[actionText/getText]: buzzInfos_.length = " + buzzInfos_.length);

				if (type_ === buzzerClient.tx_BUZZ_LIKE_TYPE()) {
					if (buzzInfos_.length > 1) {
						lInfo = buzzInfos_[0];
						lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.like.many");
						lResult = lString.replace("{alias}", buzzerClient.resolveBuzzerAlias(lInfo.buzzerInfoId));
						return lResult.replace("{count}", buzzInfos_.length);
					} else if (buzzInfos_.length === 1) {
						lInfo = buzzInfos_[0];
						lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.like.one");
						return lString.replace("{alias}", buzzerClient.resolveBuzzerAlias(lInfo.buzzerInfoId));
					}
				} else if (type_ === buzzerClient.tx_REBUZZ_TYPE()) {
					if (buzzInfos_.length > 1) {
						lInfo = buzzInfos_[0];
						lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.rebuzz.many");
						lResult = lString.replace("{alias}", buzzerClient.resolveBuzzerAlias(lInfo.buzzerInfoId));
						return lResult.replace("{count}", buzzInfos_.length);
					} else if (buzzInfos_.length === 1) {
						lInfo = buzzInfos_[0];
						lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.rebuzz.one");
						return lString.replace("{alias}", buzzerClient.resolveBuzzerAlias(lInfo.buzzerInfoId));
					}
				} else if (type_ === buzzerClient.tx_BUZZ_REPLY_TYPE()) {
					if (buzzInfos_.length > 1) {
						lInfo = buzzInfos_[0];
						lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.reply.many");
						lResult = lString.replace("{alias}", buzzerClient.resolveBuzzerAlias(lInfo.buzzerInfoId));
						return lResult.replace("{count}", buzzInfos_.length);
					} else if (buzzInfos_.length === 1) {
						lInfo = buzzInfos_[0];
						lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.reply.one");
						return lString.replace("{alias}", buzzerClient.resolveBuzzerAlias(lInfo.buzzerInfoId));
					}
				} else if (type_ === buzzerClient.tx_BUZZ_REWARD_TYPE()) {
					if (buzzInfos_.length > 1) {
						lInfo = buzzInfos_[0];
						lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.reward.many");
						lResult = lString.replace("{alias}", buzzerClient.resolveBuzzerAlias(lInfo.buzzerInfoId));
						return lResult.replace("{count}", buzzInfos_.length);
					} else if (buzzInfos_.length === 1) {
						lInfo = buzzInfos_[0];
						lString = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.reward.one");
						return lString.replace("{alias}", buzzerClient.resolveBuzzerAlias(lInfo.buzzerInfoId));
					}
				}

				return "";
			}
		}
	}

	QuarkRoundState {
		id: imageFrame
		x: avatarImage.x - 2
		y: avatarImage.y - 2
		size: avatarImage.displayWidth + 4
		color: getColor()
		background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

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

	Image {
		id: avatarImage

		x: spaceLeft_
		y: spaceTop_ + headerInfo.getHeight()
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop
		mipmap: true

		property bool rounded: true
		property int displayWidth: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 50 : 50
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

			onClicked: {
				//
				controller_.openBuzzfeedByBuzzer(buzzerName_);
			}
		}
	}

	//
	// header
	//

	QuarkLabel {
		id: buzzerAliasControl
		x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
		y: avatarImage.y
		text: buzzerAlias_
		font.bold: true
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	QuarkLabelRegular {
		id: buzzerNameControl
		x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
		y: buzzerAliasControl.y + buzzerAliasControl.height + spaceItems_
		width: parent.width - x - (menuControl.width + spaceItems_) - spaceRightMenu_
		elide: Text.ElideRight
		text: buzzerName_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 2)) : (defaultFontPointSize - 2)
	}

	QuarkSymbolLabel {
		id: menuControl
		x: parent.width - width - spaceRightMenu_
		y: avatarImage.y
		symbol: Fonts.shevronDownSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 7)) : 12
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
	}

	MouseArea {
		x: menuControl.x - 2 * spaceLeft_
		y: menuControl.y - spaceTop_
		width: menuControl.width + 4 * spaceLeft_
		height: menuControl.height + 2 * spaceTop_

		onClicked: {
			//
			if (headerMenu.visible) headerMenu.close();
			else { headerMenu.prepare(); headerMenu.popup(); }
		}
	}

	//
	// body
	//

	Rectangle {
		id: bodyControl
		x: spaceLeft_
		y: avatarImage.y + avatarImage.height + spaceTop_
		width: parent.width - (spaceLeft_ + spaceRight_)
		height: getHeight() //buzzText.height

		border.color: "transparent"
		color: "transparent"

		Component.onCompleted: {
			// expand();
			buzzitemhead_.calculateHeight();
		}

		property var buzzMediaItem_;
		property var urlInfoItem_;
		property var wrappedItem_;

		function setFullyVisible(fullyVisible) {
			if (buzzMediaItem_) buzzMediaItem_.forceVisibilityCheck(fullyVisible);
		}

		function unbindCommonControls() {
			if (buzzMediaItem_) buzzMediaItem_.unbindCommonControls();
		}

		function setSharedMediaPlayer() {
			if (buzzMediaItem_) {
				buzzMediaItem_.sharedMediaPlayer_ = buzzitemhead_.sharedMediaPlayer_;
			}
		}

		onWidthChanged: {
			expand();

			if (buzzMediaItem_ && bodyControl.width > 0) {
				buzzMediaItem_.calculatedWidth = bodyControl.width;
			}

			if (urlInfoItem_ && bodyControl.width > 0) {
				urlInfoItem_.calculatedWidth = bodyControl.width;
			}

			if (wrappedItem_ && bodyControl.width > 0) {
				wrappedItem_.width = bodyControl.width;
			}

			buzzitemhead_.calculateHeight();
		}

		onHeightChanged: {
			expand();
			buzzitemhead_.calculateHeight();
		}

		QuarkTextEdit {
			id: buzzText
			x: 0
			y: 0
			width: parent.width
			text: buzzBody_
			wrapMode: Text.Wrap
			textFormat: Text.RichText
			font.pointSize: buzzerApp.isDesktop ? Math.round(buzzerClient.scaleFactor * 13) : 20
			//lineHeight: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 1.1) : lineHeight
			readOnly: true
			selectByMouse: true
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			selectionColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.selected")

			MouseArea {
				anchors.fill: parent
				cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
				acceptedButtons: Qt.RightButton

				onClicked: {
					//
					if (headerMenu.visible) headerMenu.close();
					else {
						headerMenu.prepare(buzzText.selectedText.length);
						headerMenu.popup(mouseX, mouseY);
					}
				}
			}

			onLinkActivated: {
				var lComponent = null;
				var lPage = null;
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
					bodyControl.buzzMediaItem_.y = bodyControl.getNextY();
					bodyControl.height = bodyControl.getHeight();
				}

				if (bodyControl.urlInfoItem_) {
					bodyControl.urlInfoItem_.y = bodyControl.getNextY();
					bodyControl.height = bodyControl.getHeight();
				}

				if (bodyControl.wrappedItem_) {
					bodyControl.wrappedItem_.y = bodyControl.getY();
					bodyControl.height = bodyControl.getHeight();
				}

				buzzitemhead_.calculateHeight();
			}
		}

		function expand() {
			//
			var lSource;
			var lComponent;

			// if rebuzz and wrapped
			if (wrapped_ && !wrappedItem_) {
				// buzzText
				lSource = "qrc:/qml/buzzitemlight.qml";
				lComponent = Qt.createComponent(lSource);
				wrappedItem_ = lComponent.createObject(bodyControl);
				wrappedItem_.calculatedHeightModified.connect(innerHeightChanged);

				wrappedItem_.x = 0;
				wrappedItem_.y = bodyControl.getY();
				wrappedItem_.width = bodyControl.width;
				wrappedItem_.controller_ = buzzitemhead_.controller_;
				wrappedItem_.sharedMediaPlayer_ = buzzitemhead_.sharedMediaPlayer_;

				wrappedItem_.timestamp_ = wrapped_.timestamp;
				wrappedItem_.score_ = wrapped_.score;
				wrappedItem_.buzzId_ = wrapped_.buzzId;
				wrappedItem_.buzzChainId_ = wrapped_.buzzChainId;
				wrappedItem_.buzzerId_ = wrapped_.buzzerId;
				wrappedItem_.buzzerInfoId_ = wrapped_.buzzerInfoId;
				wrappedItem_.buzzerInfoChainId_ = wrapped_.buzzerInfoChainId;
				wrappedItem_.buzzBody_ = buzzerClient.decorateBuzzBody(wrapped_.buzzBody);
				wrappedItem_.buzzMedia_ = wrapped_.buzzMedia;
				wrappedItem_.lastUrl_ = buzzerClient.extractLastUrl(wrapped_.buzzBody);
				wrappedItem_.ago_ = buzzerClient.timestampAgo(wrapped_.timestamp);
				wrappedItem_.initialize();
			}

			// expand media
			if (buzzMedia_ && buzzMedia_.length) {
				if (!buzzMediaItem_) {
					lSource = "qrc:/qml/buzzitemmedia.qml";
					lComponent = Qt.createComponent(lSource);
					buzzMediaItem_ = lComponent.createObject(bodyControl);
					buzzMediaItem_.calculatedHeightModified.connect(innerHeightChanged);

					buzzMediaItem_.x = 0;
					buzzMediaItem_.y = bodyControl.getNextY();
					buzzMediaItem_.calculatedWidth = bodyControl.width;
					buzzMediaItem_.width = bodyControl.width;
					buzzMediaItem_.controller_ = buzzitemhead_.controller_;
					buzzMediaItem_.buzzId_ = buzzitemhead_.buzzId_;
					buzzMediaItem_.buzzMedia_ = buzzitemhead_.buzzMedia_;
					buzzMediaItem_.sharedMediaPlayer_ = buzzitemhead_.sharedMediaPlayer_;
					buzzMediaItem_.initialize();

					bodyControl.height = bodyControl.getHeight();
					buzzitemhead_.calculateHeight();
				}
			} else if (lastUrl_ && lastUrl_.length) {
				//
				if (!urlInfoItem_) {
					lSource = buzzerApp.isDesktop ? "qrc:/qml/buzzitemurl-desktop.qml" : "qrc:/qml/buzzitemurl.qml";
					lComponent = Qt.createComponent(lSource);
					urlInfoItem_ = lComponent.createObject(bodyControl);
					urlInfoItem_.calculatedHeightModified.connect(innerHeightChanged);

					urlInfoItem_.x = 0;
					urlInfoItem_.y = bodyControl.getNextY();
					urlInfoItem_.calculatedWidth = bodyControl.width;
					urlInfoItem_.width = bodyControl.width;
					urlInfoItem_.controller_ = buzzitemhead_.controller_;
					urlInfoItem_.lastUrl_ = buzzitemhead_.lastUrl_;
					urlInfoItem_.initialize();

					bodyControl.height = bodyControl.getHeight();
					buzzitemhead_.calculateHeight();
				}
			}
		}

		function innerHeightChanged(value) {
			//bodyControl.height = (buzzBody_.length > 0 ? buzzText.height : 0) + value +
			//							(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
			//							(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0);
			bodyControl.height = bodyControl.getHeight();
			buzzitemhead_.calculateHeight();
		}

		function getY() {
			return (buzzBody_.length > 0 ? buzzText.height : 0) +
					(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
					(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0);
		}

		function getNextY() {
			return (buzzBody_.length > 0 ? buzzText.height : 0) +
					(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
					(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0) +
					(wrappedItem_ ? wrappedItem_.y + wrappedItem_.calculatedHeight + spaceItems_ : 0);
		}

		function getHeight() {
			return (buzzBody_.length > 0 ? buzzText.height : 0) +
					(buzzMediaItem_ ? buzzMediaItem_.calculatedHeight : 0) +
					(urlInfoItem_ ? urlInfoItem_.calculatedHeight : 0) +
					(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
					(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0) +
					(wrappedItem_ ? wrappedItem_.calculatedHeight + spaceItems_: 0);
		}
	}

	//
	// footer info
	//
	QuarkLabelRegular {
		id: localDateTimeControl
		x: spaceLeft_
		y: bodyControl.y + bodyControl.height +
		   (bodyControl.buzzMediaItem_ || bodyControl.urlInfoItem_ || bodyControl.wrappedItem_ ? 2 * spaceItems_ : 0)
		width: bodyControl.width
		elide: Text.ElideRight
		text: localDateTime_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 3)) : (defaultFontPointSize - 3)
	}

	QuarkHLine {
		id: footerLine
		x1: 0
		y1: tipButton.y
		x2: parent.width
		y2: tipButton.y
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true
	}

	//
	// buttons
	//

	QuarkToolButton	{
		id: replyButton
		x: bodyControl.x
		y: localDateTimeControl.y + localDateTimeControl.height + spaceBottom_
		symbol: Fonts.replySym
		Material.background: "transparent"
		visible: true
		labelYOffset: /*buzzerApp.isDesktop ? 0 :*/ buzzerClient.scaleFactor * 2
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		Layout.alignment: Qt.AlignHCenter
		font.family: Fonts.icons
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

		onClicked: {
			//
			var lComponent = null;
			var lPage = null;

			lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml") :
											   Qt.createComponent("qrc:/qml/buzzeditor.qml");
			if (lComponent.status === Component.Error) {
				showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(controller);
				lPage.controller = controller;
				lPage.initializeReply(self_, buzzfeedModel_);
				addPage(lPage);
			}
		}
	}

	QuarkLabel {
		id: repliesCountControl
		x: replyButton.x + replyButton.width + spaceStats_
		y: replyButton.y + (replyButton.height / 2 - height / 2)
		text: NumberFunctions.numberToCompact(replies_).toString()
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		visible: replies_ > 0
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	QuarkToolButton	{
		id: rebuzzButton
		x: bodyControl.x + bodyControl.width / 4 // second group
		y: localDateTimeControl.y + localDateTimeControl.height + spaceBottom_
		symbol: Fonts.rebuzzSym
		Material.background: "transparent"
		visible: true
		labelYOffset: /*buzzerApp.isDesktop ? 0 :*/ buzzerClient.scaleFactor * 2
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		Layout.alignment: Qt.AlignHCenter
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

		onClicked: {
			if (rebuzzMenu.visible) rebuzzMenu.close();
			else rebuzzMenu.open();
		}
	}

	QuarkLabel {
		id: rebuzzesCountControl
		x: rebuzzButton.x + rebuzzButton.width + spaceStats_
		y: rebuzzButton.y + (rebuzzButton.height / 2 - height / 2)
		text: NumberFunctions.numberToCompact(rebuzzes_).toString()
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		visible: rebuzzes_ > 0
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	QuarkToolButton	{
		id: likeButton
		x: bodyControl.x + (bodyControl.width / 4) * 2 // third group
		y: localDateTimeControl.y + localDateTimeControl.height + spaceBottom_
		symbol: Fonts.likeSym
		Material.background: "transparent"
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		Layout.alignment: Qt.AlignHCenter
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

		onClicked: {
			buzzLikeCommand.process(buzzId_);
		}
	}

	QuarkLabel {
		id: likesCountControl
		x: likeButton.x + likeButton.width + spaceStats_
		y: likeButton.y + (likeButton.height / 2 - height / 2)
		text: NumberFunctions.numberToCompact(likes_).toString()
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		visible: likes_ > 0
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	QuarkToolButton	{
		id: tipButton
		x: bodyControl.x + (bodyControl.width / 4) * 3 // forth group
		y: localDateTimeControl.y + localDateTimeControl.height + spaceBottom_
		symbol: Fonts.tipSym
		Material.background: "transparent"
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		Layout.alignment: Qt.AlignHCenter
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

		onClicked: {
			if (tipMenu.visible) tipMenu.close();
			else tipMenu.open();
		}
	}

	QuarkLabel {
		id: rewardsCountControl
		x: tipButton.x + tipButton.width + spaceStats_
		y: tipButton.y + (tipButton.height / 2 - height / 2)
		text: NumberFunctions.numberToCompact(rewards_).toString()
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		visible: rewards_ > 0
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: tipButton.y + tipButton.height
		x2: parent.width
		y2: tipButton.y + tipButton.height
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true
	}

	//
	// menus
	//

	QuarkPopupMenu {
		id: headerMenu
		x: parent.width - width - spaceRight_
		y: menuControl.y + menuControl.height + spaceItems_
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 160) : 160
		visible: false

		model: ListModel { id: menuModel }

		onAboutToShow: prepare()

		onClick: {
			//
			if (key === "subscribe") {
				buzzerSubscribeCommand.process(buzzerName_);
			} else if (key === "unsubscribe") {
				buzzerUnsubscribeCommand.process(buzzerName_);
			} else if (key === "endorse") {
				buzzerEndorseCommand.process(buzzerName_);
			} else if (key === "mistrust") {
				buzzerMistrustCommand.process(buzzerName_);
			} else if (key === "copytx") {
				clipboard.setText(buzzId_);
			} else if (key === "copytext") {
				clipboard.setText(buzzBodyFlat_);
			} else if (key === "copyselection") {
				clipboard.setText(buzzText.selectedText);
			}
		}

		function popup(nx, ny) {
			//
			if (nx !== undefined && nx + width > parent.width) nx = parent.width - (width + spaceRight_);

			x = nx === undefined ? parent.width - width - spaceRight_ : nx;
			y = ny === undefined ? menuControl.y + menuControl.height + spaceItems_ : ny;
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
			if (buzzerId_ !== buzzerClient.getCurrentBuzzerId()) {
				menuModel.append({
					key: "endorse",
					keySymbol: Fonts.endorseSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.endorse")});

				menuModel.append({
					key: "mistrust",
					keySymbol: Fonts.mistrustSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.mistrust")});

				if (!buzzerClient.subscriptionExists(buzzerId_)) {
					menuModel.append({
						key: "subscribe",
						keySymbol: Fonts.subscribeSym,
						name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.subscribe")});
				} else {
					menuModel.append({
						key: "unsubscribe",
						keySymbol: Fonts.unsubscribeSym,
						name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.unsubscribe")});
				}
			}

			if (!selection) menuModel.append({
				key: "copytext",
				keySymbol: Fonts.copySym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.copytext")});
			menuModel.append({
				key: "copytx",
				keySymbol: Fonts.clipboardSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.copytransaction")});
		}
	}

	QuarkPopupMenu {
		id: tipMenu
		x: (tipButton.x + tipButton.width) - width
		y: tipButton.y + tipButton.height
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 135) : 135
		visible: false

		model: ListModel { id: tipModel }

		Component.onCompleted: prepare()

		onClick: {
			buzzRewardCommand.process(buzzId_, key);
		}

		function prepare() {
			if (tipModel.count) return;

			tipModel.append({
				key: "1000",
				keySymbol: Fonts.coinSym,
				name: "1000 qBIT"});

			tipModel.append({
				key: "2000",
				keySymbol: Fonts.coinSym,
				name: "2000 qBIT"});

			tipModel.append({
				key: "3000",
				keySymbol: Fonts.coinSym,
				name: "3000 qBIT"});
		}
	}

	QuarkPopupMenu {
		id: rebuzzMenu
		x: rebuzzButton.x
		y: rebuzzButton.y + rebuzzButton.height
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 190) : 190
		visible: false

		model: ListModel { id: rebuzzModel }

		Component.onCompleted: prepare()

		onClick: {
			if (key === "rebuzz") {
				rebuzzCommand.buzzId = buzzId_;
				rebuzzCommand.process();
			} else if (key === "rebuzz-comment") {
				//
				var lComponent = null;
				var lPage = null;

				lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml") :
												   Qt.createComponent("qrc:/qml/buzzeditor.qml");
				if (lComponent.status === Component.Error) {
					showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(controller);
					lPage.controller = controller;
					lPage.initializeRebuzz(self_, buzzfeedModel_);
					addPage(lPage);
				}
			}
		}

		function prepare() {
			if (rebuzzModel.count) return;

			rebuzzModel.append({
				key: "rebuzz",
				keySymbol: Fonts.rebuzzSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.rebuzz")});

			rebuzzModel.append({
				key: "rebuzz-comment",
				keySymbol: Fonts.rebuzzWithCommantSym,
				name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.rebuzz.comment")});
		}
	}

	BuzzerCommands.BuzzerSubscribeCommand {
		id: buzzerSubscribeCommand

		onProcessed: {
		}
		onError: {
			handleError(code, message);
		}
	}

	BuzzerCommands.BuzzerUnsubscribeCommand {
		id: buzzerUnsubscribeCommand

		onProcessed: {
		}
		onError: {
			handleError(code, message);
		}
	}

	BuzzerCommands.BuzzerEndorseCommand {
		id: buzzerEndorseCommand

		onProcessed: {
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
			} else if (code === "E_INSUFFICIENT_QTT_BALANCE") {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_INSUFFICIENT_QTT_BALANCE"), true);
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_ENDORSE"), true);
			}
		}
	}

	BuzzerCommands.BuzzerMistrustCommand {
		id: buzzerMistrustCommand

		onProcessed: {
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
			} else if (code === "E_INSUFFICIENT_QTT_BALANCE") {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_INSUFFICIENT_QTT_BALANCE"), true);
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_MISTRUST"), true);
			}
		}
	}

	BuzzerCommands.BuzzRewardCommand {
		id: buzzRewardCommand
		model: buzzfeedModel_

		onProcessed: {
		}
		onRetry: {
			// was specific error, retrying...
			retryRewardCommand.start();
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				//buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_REWARD_YOURSELF"));
			}
		}
	}

	Timer {
		id: retryRewardCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			buzzRewardCommand.reprocess();
		}
	}

	BuzzerCommands.BuzzLikeCommand {
		id: buzzLikeCommand
		model: buzzfeedModel_

		onProcessed: {
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				//buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_LIKE_YOURSELF"));
			}
		}
	}

	BuzzerCommands.UploadMediaCommand {
		id: uploadMediaCommand_

		onProcessed: {
		}
		onError: {
			handleError(code, message);
		}
	}

	BuzzerCommands.ReBuzzCommand {
		id: rebuzzCommand
		model: buzzfeedModel_
		uploadCommand: uploadMediaCommand_

		onProcessed: {
		}
		onRetry: {
			// was specific error, retrying...
			retryReBuzzCommand.start();
		}
		onError: {
			handleError(code, message);
		}
	}

	Timer {
		id: retryReBuzzCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			rebuzzCommand.reprocess();
		}
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			//buzzerClient.resync();
			controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			controller_.showError(message);
		}
	}
}
