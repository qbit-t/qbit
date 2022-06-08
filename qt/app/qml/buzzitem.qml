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
	id: buzzitem_

	property int calculatedHeight: 0 //calculateHeightInternal()
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
	property var buzzBodyLimited_: buzzBodyLimited
	property var buzzMedia_: buzzMedia
	property int replies_: replies
	property int rebuzzes_: rebuzzes
	property int likes_: likes
	property var rewards_: rewards
	property var originalBuzzId_: originalBuzzId
	property var originalBuzzChainId_: originalBuzzChainId
	property bool hasParent_: hasParent
	property var value_: value
	property var buzzInfos_: buzzInfos
	property var buzzerName_: buzzerName
	property var buzzerAlias_: buzzerAlias
	property int childrenCount_: childrenCount
	property bool hasPrevSibling_: hasPrevSibling
	property bool hasNextSibling_: hasNextSibling
	property var prevSiblingId_: prevSiblingId
	property var nextSiblingId_: nextSiblingId
	property var firstChildId_: firstChildId
	property var wrapped_: wrapped
	property var childLink_: false
	property var parentLink_: false
	property var lastUrl_: lastUrl
	property var self_: self
	property var rootId_
	property bool threaded_: threaded
	property bool hasPrevLink_: hasPrevLink
	property bool hasNextLink_: hasNextLink
	property bool dynamic_: dynamic
	property bool feeding_: feeding
	property bool onChain_: onChain
	property bool mistrusted_: false
	property bool endorsed_: false
	property bool hasMore_: false
	property bool ownLike_: ownLike
	property bool ownRebuzz_: ownRebuzz
	property bool adjustData_: adjustData

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
	readonly property int spaceSingleMedia_: 8
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 7
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4
	readonly property real defaultFontSize: 11

	signal calculatedHeightModified(var value);

	onCalculatedHeightChanged: {
		calculatedHeightModified(calculatedHeight);
	}

	Component.onCompleted: {
		//
		finalizeCreation();
	}

	function finalizeCreation() {
		//
		avatarDownloadCommand.process();

		if (!onChain_ && dynamic_) checkOnChain.start();

		// intial placement; dynamic "more" behaves strange
		if (replies_ > 0) {
			if (childrenCount_ < replies_)
				hasMore_ = true;
		}
	}

	function bindItem() {
		finalizeCreation();
		bodyControl.resetItem();
		bodyControl.forceExpand();
	}

	/*
	function initialize() {
		//
		avatarDownloadCommand.url = buzzerClient.getBuzzerAvatarUrl(buzzerInfoId_)
		avatarDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + buzzerClient.getBuzzerAvatarUrlHeader(buzzerInfoId_)
		avatarDownloadCommand.process();
	}
	*/

	function calculateHeightInternal() {
		var lCalculatedInnerHeight = spaceTop_ + headerInfo.getHeight() + spaceHeader_ + buzzerAliasControl.height +
											bodyControl.height + spaceItems_ +
											threadedControl.getHeight() + threadedControl.getSpacing() +
											replyButton.height + 1;
		return lCalculatedInnerHeight > avatarImage.displayHeight ?
										   lCalculatedInnerHeight : avatarImage.displayHeight;
	}

	function calculateHeight() {
		calculatedHeight = calculateHeightInternal();
		return calculatedHeight;
	}

	function cleanUp() {
		//
	}

	onAdjustData_Changed: {
	}

	onThreaded_Changed: {
		calculateHeight();
	}

	onReplies_Changed: {
		calculateHeight();
	}

	onChildrenCount_Changed: {
		calculateHeight();
	}

	onWrapped_Changed: {
		if (wrapped_) {
			expandBody.start();
		}
	}

	onSharedMediaPlayer_Changed: {
		bodyControl.setSharedMediaPlayer();
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
			// if (buzzfeedModel_) console.log("[getVisible]: " + buzzfeedModel_.itemToString(index));
			return type_ === buzzerClient.tx_BUZZ_LIKE_TYPE() ||
					type_ === buzzerClient.tx_BUZZ_REWARD_TYPE() ||
					(type_ === buzzerClient.tx_REBUZZ_TYPE() && buzzInfos_.length > 0
					 /*&& (!wrapped_ || buzzBody_.length === 0)*/); // if rebuzz without comments
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
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : 12
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

		TextMetrics	{
			id: bodyactionTextMetrics
			elide: Text.ElideRight
			text: actionText.actualText
			elideWidth: parent.width - (actionText.x + (buzzerApp.isDesktop ? 2 * spaceRight_ : spaceRight_))
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : 16
		}

		QuarkLabel {
			id: actionText
			x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
			y: -2
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : 12
			width: parent.width - x - spaceRight_
			elide: Text.ElideRight
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			text: bodyactionTextMetrics.elidedText +
				  (bodyactionTextMetrics.elidedText !== actualText && buzzerApp.isDesktop ? "..." : "")

			property var actualText: getText()

			function getText() {
				var lInfo;
				var lString;
				var lResult;

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

	//
	// NOTICE: for mobile versions we should consider to use ImageQx
	//
	BuzzerComponents.ImageQx {
		id: avatarImage

		x: spaceLeft_
		y: spaceTop_ + headerInfo.getHeight()
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop
		mipmap: true
		radius: avatarImage.displayWidth

		property bool rounded: true //!
		property int displayWidth: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 50 : 50
		property int displayHeight: displayWidth

		autoTransform: true

		/*
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
		*/

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

	QuarkRoundProgress {
		id: imageFrame
		x: avatarImage.x - 2
		y: avatarImage.y - 2
		size: avatarImage.displayWidth + 4
		colorCircle: getColor()
		colorBackground: "transparent"
		arcBegin: 0
		arcEnd: 360
		lineWidth: buzzerClient.scaleFactor * 3
		beginAnimation: false
		endAnimation: false

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

	QuarkSymbolLabel {
		id: endorseSymbol
		x: avatarImage.x + avatarImage.displayWidth / 2 - width / 2
		y: avatarImage.y + avatarImage.displayHeight + spaceItems_
		symbol: endorsed_ ? Fonts.endorseSym : Fonts.mistrustSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzitem_.defaultFontSize + 2)) : 14
		color: endorsed_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.endorsed") :
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.mistrusted");

		visible: endorsed_ || mistrusted_
	}

	QuarkVLine {
		id: childrenLine

		x1: avatarImage.x + avatarImage.width / 2
		y1: avatarImage.y + avatarImage.height + spaceLine_ + (endorseSymbol.visible ? (endorseSymbol.height + spaceItems_ + 2) : 0)
		x2: x1
		y2: bottomLine.y1 - (threaded_ || threadedControl.showMore() ? spaceThreaded_ : 0)
		penWidth: 2
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: hasNextLink_ || threadedControl.showMore()
	}

	Rectangle {
		id: threadedPoint0

		x: (avatarImage.x + avatarImage.width / 2) - spaceThreadedItems_ / 2
		y: (bottomLine.y1 - spaceThreaded_) + spaceThreadedItems_ - 1
		width: spaceThreadedItems_
		height: spaceThreadedItems_
		radius: width / 2

		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")

		visible: (hasNextLink_ || threadedControl.showMore()) && (threaded_ || threadedControl.showMore())
	}

	Rectangle {
		id: threadedPoint1

		x: (avatarImage.x + avatarImage.width / 2) - spaceThreadedItems_ / 2
		y: threadedPoint0.y + threadedPoint0.height + spaceThreadedItems_ - 1
		width: spaceThreadedItems_
		height: spaceThreadedItems_
		radius: width / 2

		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")

		visible: (hasNextLink_ || threadedControl.showMore()) && (threaded_ || threadedControl.showMore())
	}

	Rectangle {
		id: threadedPoint2

		x: (avatarImage.x + avatarImage.width / 2) - spaceThreadedItems_ / 2
		y: threadedPoint1.y + threadedPoint0.height + spaceThreadedItems_ - 1
		width: spaceThreadedItems_
		height: spaceThreadedItems_
		radius: width / 2

		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")

		visible: (hasNextLink_ || threadedControl.showMore()) && (threaded_ || threadedControl.showMore())
	}

	QuarkVLine {
		id: childrenLine1

		x1: avatarImage.x + avatarImage.width / 2
		y1: threadedPoint2.y + threadedPoint2.height + spaceThreadedItems_ - 1
		x2: x1
		y2: bottomLine.y1
		penWidth: 2
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")

		visible: hasNextLink_ && (threaded_ || threadedControl.showMore())
	}

	QuarkVLine {
		id: parentLine

		x1: avatarImage.x + avatarImage.width / 2
		y1: avatarImage.y - spaceLine_
		x2: x1
		y2: 0
		penWidth: 2
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: hasPrevLink_
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
		x: buzzerAliasControl.x + buzzerAliasControl.width + spaceItems_
		y: avatarImage.y
		width: parent.width - x - (agoControl.width + spaceItems_ * 2 + menuControl.width + spaceItems_) - spaceRightMenu_
		elide: Text.ElideRight
		text: buzzerName_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
	}

	QuarkLabel {
		id: agoControl
		x: menuControl.x - width - spaceItems_ * 2
		y: avatarImage.y
		text: ago_
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
		x: agoControl.x
		y: menuControl.y - spaceTop_
		width: agoControl.width + menuControl.width + spaceRightMenu_
		height: agoControl.height + spaceRightMenu_
		cursorShape: Qt.PointingHandCursor

		onClicked: {
			//
			if (headerMenu.visible) headerMenu.close();
			else { headerMenu.prepare(); headerMenu.open(); }
		}
	}

	QuarkSymbolLabel {
		id: onChainSymbol
		x: menuControl.x + 1
		y: menuControl.y + menuControl.height + (spaceItems_ - 2)
		symbol: !onChain_ ? Fonts.clockSym : Fonts.checkedCircleSym //linkSym
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 8) : 12
		color: !onChain_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.wait") :
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzz.done");

		visible: dynamic_
	}

	BuzzerCommands.LoadTransactionCommand {
		id: checkOnChainCommand

		onProcessed: {
			// tx, chain
			buzzfeedModel_.setOnChain(index);
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

	Timer {
		id: expandBodyRegular
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			bodyControl.forceExpand();
		}
	}

	Timer {
		id: expandBody
		interval: 300
		repeat: false
		running: false

		onTriggered: {
			bodyControl.expand();
		}
	}

	Timer {
		id: openConversation
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			//
			var lId = buzzerClient.getConversationsList().locateConversation(buzzerName_);
			// conversation found
			if (lId !== "") {
				openBuzzerConversation(lId);
			} else {
				start();
			}
		}
	}

	//
	// body
	//

	Rectangle {
		id: bodyControl
		x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
		y: buzzerAliasControl.y + buzzerAliasControl.height + spaceHeader_
		width: parent.width - (avatarImage.x + avatarImage.width + spaceAvatarBuzz_) - spaceRight_
		height: getHeight() //buzzText.height

		border.color: "transparent"
		color: "transparent"

		Component.onCompleted: {
			// expand();
			buzzitem_.calculateHeight();
		}

		Loader {
			id: wrapperLoader
			asynchronous: true

			onLoaded: {
				bodyControl.wrappedItem_ = item;
				bodyControl.wrappedItem_.calculatedHeightModified.connect(bodyControl.innerHeightChanged);

				bodyControl.wrappedItem_.x = 0;
				bodyControl.wrappedItem_.y = bodyControl.getY();
				bodyControl.wrappedItem_.width = bodyControl.width;
				bodyControl.wrappedItem_.controller_ = buzzitem_.controller_;
				bodyControl.wrappedItem_.sharedMediaPlayer_ = buzzitem_.sharedMediaPlayer_;

				//console.log("[WRAPPED]: wrapped_.buzzerId = " + wrapped_.buzzerId + ", wrapped_.buzzerInfoId = " + wrapped_.buzzerInfoId);

				bodyControl.wrappedItem_.timestamp_ = wrapped_.timestamp;
				bodyControl.wrappedItem_.score_ = wrapped_.score;
				bodyControl.wrappedItem_.buzzId_ = wrapped_.buzzId;
				bodyControl.wrappedItem_.buzzChainId_ = wrapped_.buzzChainId;
				bodyControl.wrappedItem_.buzzerId_ = wrapped_.buzzerId;
				bodyControl.wrappedItem_.buzzerInfoId_ = wrapped_.buzzerInfoId;
				bodyControl.wrappedItem_.buzzerInfoChainId_ = wrapped_.buzzerInfoChainId;
				bodyControl.wrappedItem_.buzzBody_ = buzzerClient.decorateBuzzBody(wrapped_.buzzBody);
				bodyControl.wrappedItem_.buzzMedia_ = wrapped_.buzzMedia;
				bodyControl.wrappedItem_.lastUrl_ = buzzerClient.extractLastUrl(wrapped_.buzzBody);
				bodyControl.wrappedItem_.ago_ = buzzerClient.timestampAgo(wrapped_.timestamp);
				bodyControl.wrappedItem_.initialize();

				bodyControl.height = bodyControl.getHeight();
				buzzitem_.calculateHeight();
			}
		}

		Loader {
			id: mediaLoader
			asynchronous: true

			onLoaded: {
				bodyControl.buzzMediaItem_ = item;
				bodyControl.buzzMediaItem_.calculatedHeightModified.connect(bodyControl.innerHeightChanged);

				bodyControl.buzzMediaItem_.x = 0;
				bodyControl.buzzMediaItem_.y = bodyControl.getNextY();
				bodyControl.buzzMediaItem_.calculatedWidth = bodyControl.width;
				bodyControl.buzzMediaItem_.width = bodyControl.width;
				bodyControl.buzzMediaItem_.controller_ = buzzitem_.controller_;
				bodyControl.buzzMediaItem_.buzzId_ = buzzitem_.buzzId_;
				bodyControl.buzzMediaItem_.buzzerAlias_ = buzzitem_.buzzerAlias_;
				bodyControl.buzzMediaItem_.buzzBody_ = buzzitem_.buzzBodyFlat_;
				bodyControl.buzzMediaItem_.buzzMedia_ = buzzitem_.buzzMedia_;
				bodyControl.buzzMediaItem_.sharedMediaPlayer_ = buzzitem_.sharedMediaPlayer_;
				bodyControl.buzzMediaItem_.initialize();

				bodyControl.height = bodyControl.getHeight();
				buzzitem_.calculateHeight();
			}
		}

		Loader {
			id: urlLoader
			asynchronous: true

			onLoaded: {
				bodyControl.urlInfoItem_ = item;
				bodyControl.urlInfoItem_.calculatedHeightModified.connect(bodyControl.innerHeightChanged);

				bodyControl.urlInfoItem_.x = 0;
				bodyControl.urlInfoItem_.y = bodyControl.getNextY();
				bodyControl.urlInfoItem_.calculatedWidth = bodyControl.width;
				bodyControl.urlInfoItem_.width = bodyControl.width;
				bodyControl.urlInfoItem_.controller_ = buzzitem_.controller_;
				bodyControl.urlInfoItem_.lastUrl_ = buzzitem_.lastUrl_;
				bodyControl.urlInfoItem_.initialize();

				bodyControl.height = bodyControl.getHeight();
				buzzitem_.calculateHeight();
			}
		}

		property var buzzMediaItem_;
		property var urlInfoItem_;
		property var wrappedItem_;

		function setFullyVisible(fullyVisible) {
			if (buzzMediaItem_) buzzMediaItem_.forceVisibilityCheck(fullyVisible);
			if (wrappedItem_) wrappedItem_.forceVisibilityCheck(fullyVisible);
		}

		function unbindCommonControls() {
			if (buzzMediaItem_) buzzMediaItem_.unbindCommonControls();
			if (wrappedItem_) wrappedItem_.unbindCommonControls();
		}

		function setSharedMediaPlayer() {
			if (buzzMediaItem_) {
				buzzMediaItem_.sharedMediaPlayer_ = buzzitem_.sharedMediaPlayer_;
			}

			if (wrappedItem_) {
				wrappedItem_.sharedMediaPlayer_ = buzzitem_.sharedMediaPlayer_;
			}
		}

		function forceExpand() {
			//
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

			buzzitem_.calculateHeight();
		}

		function resetItem() {
			buzzMediaItem_ = null;
			urlInfoItem_ = null;
			wrappedItem_ = null;
		}

		onWidthChanged: {
			//
			if (dynamic_) expandBodyRegular.start();
			else forceExpand();
		}

		onHeightChanged: {
			//
			if (dynamic_) expandBodyRegular.start();
			else forceExpand();
		}

		QuarkLabel {
			id: buzzText
			x: 0
			y: 0
			width: parent.width
			text: buzzBodyLimited_ // buzzBody_ ? (buzzBody_.length > 500 ? buzzBody_.slice(0, 500) + "..." : buzzBody_) : ""
			wrapMode: Text.Wrap
			textFormat: Text.RichText
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize * multiplicator_) : defaultFontPointSize * multiplicator_
			//lineHeight: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 1.1) : lineHeight

			property real multiplicator_: isEmoji ? 2.5 : 1.0

			MouseArea {
				anchors.fill: parent
				cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
				acceptedButtons: Qt.NoButton
			}

			onLinkHovered: {
				//
			}

			onLinkActivated: {
				//
				if (link[0] === '@') {
					// buzzer
					controller_.openBuzzfeedByBuzzer(link);
				} else if (link[0] === '#') {
					// tag
					controller_.openBuzzfeedByTag(link);
				} else if (link.slice(0, 7) === "buzz://") {
					// link
					var lParts = link.split("/");
					controller_.openThread(lParts[2], lParts[3]);
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

				buzzitem_.calculateHeight();
			}
		}

		function expand() {
			//
			var lSource;
			var lComponent;

			//console.info("[expane]: wrapped_ = " + wrapped_);

			// if rebuzz and wapped
			if (wrapped_ && !wrappedItem_) {
				//
				wrapperLoader.setSource("qrc:/qml/buzzitemlight.qml");
			}

			// expand media
			if (buzzMedia_.length) {
				if (!buzzMediaItem_) {
					//
					mediaLoader.setSource("qrc:/qml/buzzitemmedia.qml");
				}
			} else if (lastUrl_.length) {
				//
				if (!urlInfoItem_) {
					//
					urlLoader.setSource(buzzerApp.isDesktop ? "qrc:/qml/buzzitemurl-desktop.qml" : "qrc:/qml/buzzitemurl.qml");
				}
			}
		}

		function innerHeightChanged(value) {
			if (value <= 1) return;

			bodyControl.height = bodyControl.getHeight();
			buzzitem_.calculateHeight();

			//console.log("[innerHeightChanged]: value = " + value + ", bodyControl.height = " + bodyControl.height);
		}

		function getY() {
			// var lAdjust = buzzMedia_.length > 0 ? 0 : (buzzerClient.scaleFactor * 12);
			return (buzzBody_.length > 0 ? buzzText.height : 0) +
					(buzzBody_.length > 0 && buzzMedia_.length > 0 ? spaceMedia_ : spaceItems_) +
					(buzzMedia_.length > 1 ? spaceMediaIndicator_ : spaceSingleMedia_);
		}

		function getNextY() {
			var lAdjust = buzzMedia_.length > 0 || urlInfoItem_ || wrappedItem_ ? 0 : (buzzerClient.scaleFactor * 12);
			return (buzzBody_.length > 0 ? buzzText.height - lAdjust : 0) +
					(buzzBody_.length > 0 && buzzMedia_.length > 0 ? spaceMedia_ : spaceItems_) +
					(buzzMedia_.length > 1 ? spaceMediaIndicator_ : spaceSingleMedia_) +
					(wrappedItem_ ? wrappedItem_.y + wrappedItem_.calculatedHeight + spaceItems_ : 0);
		}

		function getHeight() {
			var lAdjust =
					buzzMedia_.length > 0 ||
					buzzMediaItem_ ||
					urlInfoItem_ && urlInfoItem_.calculatedHeight > 0 ||
					wrappedItem_ ? 0 : (buzzerClient.scaleFactor * 12);

			return (buzzBody_.length > 0 ? buzzText.height - lAdjust : 0) +
					(buzzMediaItem_ ? buzzMediaItem_.calculatedHeight : 0) +
					(urlInfoItem_ ? urlInfoItem_.calculatedHeight : 0) +
					(buzzBody_.length > 0 && buzzMedia_.length > 0 ? spaceMedia_ : spaceItems_) +
					(buzzMedia_.length > 1 ? spaceMediaIndicator_ : spaceSingleMedia_) +
					(wrappedItem_ ? wrappedItem_.calculatedHeight + spaceItems_: 0);
		}
	}

	//
	// buttons
	//

	QuarkToolButton	{
		id: replyButton
		x: bodyControl.x - spaceLeft_
		y: bodyControl.y + bodyControl.getHeight() + spaceItems_
		symbol: Fonts.replySym
		Material.background: "transparent"
		visible: true
		labelYOffset: buzzerClient.scaleFactor * 2
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		Layout.alignment: Qt.AlignHCenter
		font.family: Fonts.icons
		symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

		onClicked: {
			//
			controller.openReplyEditor(self_, buzzfeedModel_);

			/*
			var lComponent = null;
			var lPage = null;

			lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml", controller_) :
											   Qt.createComponent("qrc:/qml/buzzeditor.qml");
			if (lComponent.status === Component.Error) {
				showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(controller_);
				lPage.controller = controller_;
				lPage.initializeReply(self_, buzzfeedModel_);
				addPage(lPage);
			}
			*/
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
		x: bodyControl.x - spaceLeft_ + (bodyControl.width + spaceLeft_) / 4 // second group
		y: bodyControl.y + bodyControl.getHeight() + spaceItems_
		symbol: Fonts.rebuzzSym
		Material.background: "transparent"
		visible: true
		labelYOffset: buzzerClient.scaleFactor * 2
		symbolColor: ownRebuzz_ ?
			buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.rebuzz") :
			buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
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
		x: bodyControl.x - spaceLeft_ + ((bodyControl.width + spaceLeft_) / 4) * 2 // third group
		y: bodyControl.y + bodyControl.getHeight() + spaceItems_
		symbol: Fonts.likeSym
		Material.background: "transparent"
		visible: true
		labelYOffset: 3
		symbolColor: ownLike_ ?
			buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.like") :
			buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
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
		x: bodyControl.x - spaceLeft_ + ((bodyControl.width + spaceLeft_) / 4) * 3 // forth group
		y: bodyControl.y + bodyControl.getHeight() + spaceItems_
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

	//
	// threaded
	//

	QuarkLabelRegular {
		id: threadedControl
		x: bodyControl.x
		y: replyButton.y + replyButton.height // + getSpacing()
		width: bodyControl.width
		elide: Text.ElideRight
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.threaded")
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : 14
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
		visible: getVisible() //threaded_ || showMore()

		function getVisible() {
			//console.log("threaded_ = " + threaded_ + ", replies_ = " + replies_ + ", childenCount_ = " + childrenCount_);
			return threaded_ || showMore();
		}

		function showMore() {
			//
			if (dynamic_) return false;

			return hasMore_;
		}

		function getHeight() {
			if (threaded_ || showMore()) {
				return height;
			}

			return 0;
		}

		function getSpacing() {
			if (threaded_ || showMore()) {
				return spaceBottom_;
			}

			return 0;
		}
	}

	//
	// bottom line
	//

	QuarkHLine {
		id: bottomLine
		x1: 0
		y1: threadedControl.y + threadedControl.getHeight() + threadedControl.getSpacing()
		x2: parent.width
		y2: threadedControl.y + threadedControl.getHeight() + threadedControl.getSpacing()
		penWidth: 1
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: true

		onY1Changed: {
			calculateHeight();
		}
	}

	//
	// menus
	//

	QuarkPopupMenu {
		id: headerMenu
		x: parent.width - width - spaceRight_
		y: menuControl.y + menuControl.height + spaceItems_
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 150) : 150
		visible: false
		freeSizing: true

		model: ListModel { id: menuModel }

		//Component.onCompleted: prepare()

		onAboutToShow: {
			prepare();
		}

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
				clipboard.setText("buzz://" + buzzChainId_ + "/" + buzzId_);
			} else if (key === "hide") {
				buzzHideCommand.process(buzzId_);
			} else if (key === "conversation") {
				//
				var lId = buzzerClient.getConversationsList().locateConversation(buzzerName_);
				// conversation found
				if (lId !== "") {
					openBuzzerConversation(lId);
				} else {
					createConversation.process(buzzerName_);
				}
			}
		}

		function prepare() {
			//
			menuModel.clear();

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

				menuModel.append({
					key: "conversation",
					keySymbol: Fonts.conversationMessageSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.conversation")});
			} else {
				menuModel.append({
					key: "hide",
					keySymbol: Fonts.trashSym,
					name: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.hide")});
			}

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

		//Component.onCompleted: prepare()

		onAboutToShow: {
			prepare();
		}

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
		freeSizing: true

		model: ListModel { id: rebuzzModel }

		//Component.onCompleted: prepare()

		onAboutToShow: {
			prepare();
		}

		onClick: {
			if (key === "rebuzz") {
				rebuzzCommand.buzzId = buzzId_;
				rebuzzCommand.process();
			} else if (key === "rebuzz-comment") {
				//
				controller.openRebuzzEditor(self_, buzzfeedModel_, index);

				/*
				var lComponent = null;
				var lPage = null;

				lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzeditor-desktop.qml", controller_) :
												   Qt.createComponent("qrc:/qml/buzzeditor.qml");
				if (lComponent.status === Component.Error) {
					showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(controller_);
					lPage.controller = controller_;
					lPage.initializeRebuzz(self_, buzzfeedModel_, index);
					addPage(lPage);
				}
				*/
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

	BuzzerCommands.BuzzHideCommand {
		id: buzzHideCommand
		model: buzzfeedModel_

		onProcessed: {
			// signal to remove from model
			buzzfeedModel_.remove(index);
		}
		onError: {
			handleError(code, message);
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
			endorsed_ = true;
		}
		onRetry: {
			// was specific error, retrying...
			retryBuzzerEndorseCommand.start();
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

	Timer {
		id: retryBuzzerEndorseCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			buzzerEndorseCommand.reprocess();
		}
	}

	BuzzerCommands.BuzzerMistrustCommand {
		id: buzzerMistrustCommand

		onProcessed: {
			mistrusted_ = true;
		}
		onRetry: {
			// was specific error, retrying...
			retryBuzzerEndorseCommand.start();
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				//buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
			} else if (code === "E_INSUFFICIENT_QTT_BALANCE") {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_INSUFFICIENT_QTT_BALANCE"), true);
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_MISTRUST"), true);
			}
		}
	}

	Timer {
		id: retryBuzzerMistrustCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			buzzerMistrustCommand.reprocess();
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
			buzzfeedModel_.setHasLike(index);
		}

		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				buzzerClient.resync();
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
			buzzfeedModel_.setHasRebuzz(index);
		}

		onRetry: {
			// was specific error, retrying...
			retryReBuzzCommand.start();
		}

		onError: {
			// mostly silent
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				//buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
			}
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

	BuzzerCommands.CreateConversationCommand {
		id: createConversation

		onProcessed: {
			// open conversation
			openConversation.start();
		}

		onError: {
			// code, message
			handleError(code, message);
		}
	}

	function openBuzzerConversation(conversationId) {
		//
		var lConversation = buzzerClient.locateConversation(conversationId);
		if (lConversation) {
			controller_.openConversation(conversationId, lConversation, buzzerClient.getConversationsList());
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
