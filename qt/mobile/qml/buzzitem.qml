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
	property bool onChain_: onChain
	property bool mistrusted_: false
	property bool endorsed_: false

	property var controller_: controller
	property var buzzfeedModel_: buzzfeedModel
	property var listView_

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

	signal calculatedHeightModified(var value);

	onCalculatedHeightChanged: {
		calculatedHeightModified(calculatedHeight);
	}

	Component.onCompleted: {
		avatarDownloadCommand.process();

		if (!onChain_ && dynamic_) checkOnChain.start();
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

	onThreaded_Changed: {
		calculateHeight();
	}

	onReplies_Changed: {
		calculateHeight();
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

		QuarkLabel {
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

			switch(parseInt(lIndex)) {
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

		property bool rounded: true
		property int displayWidth: 50
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

			onClicked: {
				// buzzer
				var lComponent = null;
				var lPage = null;

				lComponent = Qt.createComponent("qrc:/qml/buzzfeedbuzzer.qml");
				if (lComponent.status === Component.Error) {
					showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(controller);
					lPage.controller = controller;
					lPage.start(buzzerName_);
					addPage(lPage);
				}
			}
		}
	}

	QuarkSymbolLabel {
		id: endorseSymbol
		x: avatarImage.x + avatarImage.displayWidth / 2 - width / 2
		y: avatarImage.y + avatarImage.displayHeight + spaceItems_
		symbol: endorsed_ ? Fonts.endorseSym : Fonts.mistrustSym
		font.pointSize: 14
		color: endorsed_ ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.endorsed") :
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.mistrusted");

		visible: endorsed_ || mistrusted_
	}

	QuarkVLine {
		id: childrenLine

		x1: avatarImage.x + avatarImage.width / 2
		y1: avatarImage.y + avatarImage.height + spaceLine_ + (endorseSymbol.visible ? (endorseSymbol.height + spaceItems_ + 2) : 0)
		x2: x1
		y2: bottomLine.y - (threaded_ || threadedControl.showMore() ? spaceThreaded_ : 0)
		penWidth: 2
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		visible: hasNextLink_ || threadedControl.showMore()
	}

	Rectangle {
		id: threadedPoint0

		x: (avatarImage.x + avatarImage.width / 2) - spaceThreadedItems_ / 2
		y: (bottomLine.y - spaceThreaded_) + spaceThreadedItems_ - 1
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
		y2: bottomLine.y
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
	}

	QuarkLabel {
		id: buzzerNameControl
		x: buzzerAliasControl.x + buzzerAliasControl.width + spaceItems_
		y: avatarImage.y
		width: parent.width - x - (agoControl.width + spaceItems_ * 2 + menuControl.width + spaceItems_) - spaceRightMenu_
		elide: Text.ElideRight
		text: buzzerName_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
	}

	QuarkLabel {
		id: agoControl
		x: menuControl.x - width - spaceItems_ * 2
		y: avatarImage.y
		text: ago_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
	}

	QuarkSymbolLabel {
		id: menuControl
		x: parent.width - width - spaceRightMenu_
		y: avatarImage.y
		symbol: Fonts.shevronDownSym
		font.pointSize: 12
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
	}
	MouseArea {
		x: agoControl.x
		y: menuControl.y - spaceTop_
		width: agoControl.width + menuControl.width + spaceRightMenu_
		height: agoControl.height + spaceRightMenu_

		onClicked: {
			//
			if (headerMenu.visible) headerMenu.close();
			else { headerMenu.prepare(); headerMenu.open(); }
		}
	}

	QuarkSymbolLabel {
		id: onChainSymbol
		x: menuControl.x
		y: menuControl.y + menuControl.height + spaceItems_
		symbol: !onChain_ ? Fonts.clockSym : Fonts.checkedCircleSym //linkSym
		font.pointSize: 12
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

		property var buzzMediaItem_;
		property var urlInfoItem_;
		property var wrappedItem_;

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

			buzzitem_.calculateHeight();
		}

		onHeightChanged: {
			expand();
			buzzitem_.calculateHeight();
		}

		QuarkLabel {
			id: buzzText
			x: 0
			y: 0
			width: parent.width
			text: buzzBody_
			wrapMode: Text.Wrap
			textFormat: Text.RichText

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

			/*
			TextMetrics {
				id: buzzBodyMetrics
				font.family: buzzText.font.family

				text: buzzBody
			}
			*/
		}

		function expand() {
			//
			var lSource;
			var lComponent;

			// if rebuzz and wapped
			if (wrapped_ && !wrappedItem_) {
				// buzzText
				lSource = "qrc:/qml/buzzitemlight.qml";
				lComponent = Qt.createComponent(lSource);
				wrappedItem_ = lComponent.createObject(bodyControl);
				wrappedItem_.calculatedHeightModified.connect(innerHeightChanged);

				wrappedItem_.x = 0;
				wrappedItem_.y = bodyControl.getY();
				wrappedItem_.width = bodyControl.width;
				wrappedItem_.controller_ = buzzitem_.controller_;

				//console.log("[WRAPPED]: wrapped_.buzzerId = " + wrapped_.buzzerId + ", wrapped_.buzzerInfoId = " + wrapped_.buzzerInfoId);

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
			}

			// expand media
			if (buzzMedia_.length) {
				if (!buzzMediaItem_) {
					lSource = "qrc:/qml/buzzitemmedia.qml";
					lComponent = Qt.createComponent(lSource);
					buzzMediaItem_ = lComponent.createObject(bodyControl);
					buzzMediaItem_.calculatedHeightModified.connect(innerHeightChanged);

					buzzMediaItem_.x = 0;
					buzzMediaItem_.y = bodyControl.getNextY();
					buzzMediaItem_.calculatedWidth = bodyControl.width;
					buzzMediaItem_.width = bodyControl.width;
					buzzMediaItem_.controller_ = buzzitem_.controller_;
					buzzMediaItem_.buzzMedia_ = buzzitem_.buzzMedia_;
					buzzMediaItem_.initialize();

					bodyControl.height = bodyControl.getHeight();
					buzzitem_.calculateHeight();
				}
			} else if (lastUrl_.length) {
				//
				if (!urlInfoItem_) {
					lSource = "qrc:/qml/buzzitemurl.qml";
					lComponent = Qt.createComponent(lSource);
					urlInfoItem_ = lComponent.createObject(bodyControl);
					urlInfoItem_.calculatedHeightModified.connect(innerHeightChanged);

					urlInfoItem_.x = 0;
					urlInfoItem_.y = bodyControl.getNextY();
					urlInfoItem_.calculatedWidth = bodyControl.width;
					urlInfoItem_.width = bodyControl.width;
					urlInfoItem_.controller_ = buzzitem_.controller_;
					urlInfoItem_.lastUrl_ = buzzitem_.lastUrl_;
					urlInfoItem_.initialize();

					bodyControl.height = bodyControl.getHeight();
					buzzitem_.calculateHeight();
				}
			}
		}

		function innerHeightChanged(value) {
			bodyControl.height = (buzzBody_.length > 0 ? buzzText.height : 0) + value +
										(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
										(buzzMedia_.length > 1 ? spaceMediaIndicator_ : spaceSingleMedia_);
			buzzitem_.calculateHeight();
		}

		function getY() {
			return (buzzBody_.length > 0 ? buzzText.height : 0) +
					(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
					(buzzMedia_.length > 1 ? spaceMediaIndicator_ : spaceSingleMedia_);
		}

		function getNextY() {
			return (buzzBody_.length > 0 ? buzzText.height : 0) +
					(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
					(buzzMedia_.length > 1 ? spaceMediaIndicator_ : spaceSingleMedia_) +
					(wrappedItem_ ? wrappedItem_.y + wrappedItem_.calculatedHeight + spaceItems_ : 0);
		}

		function getHeight() {
			return (buzzBody_.length > 0 ? buzzText.height : 0) +
					(buzzMediaItem_ ? buzzMediaItem_.calculatedHeight : 0) +
					(urlInfoItem_ ? urlInfoItem_.calculatedHeight : 0) +
					(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
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
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		Layout.alignment: Qt.AlignHCenter
		font.family: Fonts.icons

		onClicked: {
			//
			var lComponent = null;
			var lPage = null;

			lComponent = Qt.createComponent("qrc:/qml/buzzeditor.qml");
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
	}

	QuarkToolButton	{
		id: rebuzzButton
		x: bodyControl.x - spaceLeft_ + (bodyControl.width + spaceLeft_) / 4 // second group
		y: bodyControl.y + bodyControl.getHeight() + spaceItems_
		symbol: Fonts.rebuzzSym
		Material.background: "transparent"
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		Layout.alignment: Qt.AlignHCenter

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
	}

	QuarkToolButton	{
		id: likeButton
		x: bodyControl.x - spaceLeft_ + ((bodyControl.width + spaceLeft_) / 4) * 2 // third group
		y: bodyControl.y + bodyControl.getHeight() + spaceItems_
		symbol: Fonts.likeSym
		Material.background: "transparent"
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		Layout.alignment: Qt.AlignHCenter

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
	}

	//
	// threaded
	//

	QuarkLabel {
		id: threadedControl
		x: bodyControl.x
		y: replyButton.y + replyButton.height // + getSpacing()
		width: bodyControl.width
		elide: Text.ElideRight
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.buzz.threaded");
		font.pointSize: 14
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		visible: threaded_ || showMore()

		function showMore() {
			//
			if (dynamic_) return false;

			if (replies_ > 0) {
				if (buzzfeedModel_ && buzzfeedModel_.childrenCount(index) === 0)
					return true;
			}

			return false;
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
	}

	//
	// menus
	//

	QuarkPopupMenu {
		id: headerMenu
		x: parent.width - width - spaceRight_
		y: menuControl.y + menuControl.height + spaceItems_
		width: 150
		visible: false

		model: ListModel { id: menuModel }

		Component.onCompleted: prepare()

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
		width: 135
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
		width: 190
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

				lComponent = Qt.createComponent("qrc:/qml/buzzeditor.qml");
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
			endorsed_ = true;
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_ENDORSE"));
			}
		}
	}

	BuzzerCommands.BuzzerMistrustCommand {
		id: buzzerMistrustCommand

		onProcessed: {
			mistrusted_ = true;
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_MISTRUST"));
			}
		}
	}

	BuzzerCommands.BuzzRewardCommand {
		id: buzzRewardCommand
		model: buzzfeedModel_

		onProcessed: {
		}
		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code == "E_TX_NOT_SENT") {
				buzzerClient.resync();
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
			} else {
				controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_REWARD_YOURSELF"));
			}
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
		}
		onError: {
			handleError(code, message);
		}
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			buzzerClient.resync();
			controller_.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			controller_.showError(message);
		}
	}
}
