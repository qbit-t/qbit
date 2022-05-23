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
	id: buzzitemlight_

	property int calculatedHeight: calculateHeightInternal()
	property var type_: type
	property var buzzId_//: buzzId
	property var buzzChainId_//: buzzChainId
	property var timestamp_//: timestamp
	property var ago_//: ago
	property var localDateTime_: localDateTime
	property var score_//: score
	property var buzzerId_//: buzzerId
	property var buzzerInfoId_//: buzzerInfoId
	property var buzzerInfoChainId_//: buzzerInfoChainId
	property var buzzBody_//: buzzBody
	property var buzzBodyFlat_//: buzzBodyFlat
	property var buzzMedia_//: buzzMedia
	property bool interactive_: true
	//property var replies_: replies
	//property var rebuzzes_: rebuzzes
	//property var likes_: likes
	//property var rewards_: rewards
	//property var originalBuzzId_: originalBuzzId
	//property var originalBuzzChainId_: originalBuzzChainId
	//property var hasParent_: hasParent
	property var value_: value
	//property var buzzInfos_: buzzInfos
	//property var buzzerName_: buzzerName
	//property var buzzerAlias_: buzzerAlias
	//property var childrenCount_: childrenCount
	//property var hasPrevSibling_: hasPrevSibling
	//property var hasNextSibling_: hasNextSibling
	//property var prevSiblingId_: prevSiblingId
	//property var nextSiblingId_: nextSiblingId
	//property var firstChildId_: firstChildId
	//property var wrapped_: wrapped
	property var childLink_: false
	property var parentLink_: false
	property var lastUrl_//: lastUrl

	property var controller_: controller
	//property var buzzfeedModel_: buzzfeedModel
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

	onBuzzerInfoId_Changed: {
		//console.log("[onBuzzerInfoId_Changed]: " + buzzerClient.getBuzzerAvatarUrl(buzzerInfoId_));
		//avatarDownloadCommand.url = buzzerClient.getBuzzerAvatarUrl(buzzerInfoId_)
		//avatarDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + buzzerClient.getBuzzerAvatarUrlHeader(buzzerInfoId_)
		//avatarDownloadCommand.process();
	}

	onSharedMediaPlayer_Changed: {
		bodyControl.setSharedMediaPlayer();
	}

	function calculateHeightInternal() {
		var lCalculatedInnerHeight = spaceTop_ + spaceHeader_ + buzzerAliasControl.height +
											bodyControl.height + spaceLeft_;
		return lCalculatedInnerHeight > avatarImage.displayHeight ?
										   lCalculatedInnerHeight : avatarImage.displayHeight;
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

	function initialize() {
		//
		console.log("[initialize]: url = " + buzzerClient.getBuzzerAvatarUrl(buzzitemlight_.buzzerInfoId_));
		avatarDownloadCommand.url = buzzerClient.getBuzzerAvatarUrl(buzzitemlight_.buzzerInfoId_);
		avatarDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + buzzerClient.getBuzzerAvatarUrlHeader(buzzitemlight_.buzzerInfoId_);
		avatarDownloadCommand.process();
		bodyControl.expand();
	}

	//
	// avatar download
	//

	BuzzerCommands.DownloadMediaCommand {
		id: avatarDownloadCommand
		preview: true
		skipIfExists: true

		onProcessed: {
			// tx, previewFile, originalFile
			console.log("[buzzitemlight/onProcessed]: previewFile = " + previewFile);
			avatarImage.source = "file://" + previewFile
		}

		onError: {
			console.log("[buzzitemlight/onProcessed/error]: " + code + " - " + message + ", url = " + url);
		}
	}

	//
	// header info
	//

	Rectangle {
		//
		id: buzzItemContainer
		x: 0
		y: 0
		color: "transparent"
		width: parent.width
		height: calculatedHeight // calculateHeight()

		visible: true

		clip: true
		radius: 8

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
					radius: 8
				}
			}
		}
		*/

		/*
		QuarkRoundState {
			id: imageFrame
			x: avatarImage.x - 1
			y: avatarImage.y - 1
			size: avatarImage.displayWidth + 2
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
		*/

		//
		// NOTICE: for mobile versions we should consider to use ImageQx
		//
		Image { //BuzzerComponents.ImageQx
			id: avatarImage

			x: spaceLeft_
			y: spaceTop_
			width: avatarImage.displayWidth
			height: avatarImage.displayHeight
			fillMode: Image.PreserveAspectCrop
			mipmap: true
			//radius: avatarImage.displayWidth

			property bool rounded: true
			property int displayWidth: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 20 : 20
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
					controller_.openBuzzfeedByBuzzer(buzzerClient.getBuzzerName(buzzerInfoId_));
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
		// header
		//

		QuarkLabel {
			id: buzzerAliasControl
			x: avatarImage.x + avatarImage.width + spaceAvatarBuzz_
			y: avatarImage.y
			text: buzzerClient.getBuzzerAlias(buzzerInfoId_)
			font.bold: true
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
		}

		QuarkLabelRegular {
			id: buzzerNameControl
			x: buzzerAliasControl.x + buzzerAliasControl.width + spaceItems_
			y: avatarImage.y
			width: parent.width - x - (agoControl.width + spaceRightMenu_)
			elide: Text.ElideRight
			text: buzzerClient.getBuzzerName(buzzerInfoId_)
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
		}

		QuarkLabel {
			id: agoControl
			x: parent.width - width - spaceRightMenu_
			y: avatarImage.y
			text: ago_
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : defaultFontPointSize
		}

		//
		// body
		//

		Rectangle {
			id: bodyControl
			x: spaceLeft_
			y: avatarImage.y + avatarImage.height + spaceHeader_
			width: parent.width - (spaceLeft_ + spaceRight_)
			//height: getHeight() //buzzText.height

			border.color: "transparent"
			color: "transparent"

			Component.onCompleted: {
				// expand();
				buzzitemlight_.calculateHeight();
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
					buzzMediaItem_.sharedMediaPlayer_ = buzzitemlight_.sharedMediaPlayer_;
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

				buzzitemlight_.calculateHeight();
			}

			onHeightChanged: {
				expand();
				buzzitemlight_.calculateHeight();
			}

			QuarkLabel {
				id: buzzText
				x: 0
				y: 0
				width: parent.width
				text: buzzBody_
				wrapMode: Text.Wrap
				textFormat: Text.RichText
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize * multiplicator_) : defaultFontPointSize * multiplicator_
				lineHeight: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 1.1) : lineHeight

				property real multiplicator_: isEmoji ? 2.5 : 1.0

				MouseArea {
					anchors.fill: parent
					cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
					acceptedButtons: Qt.NoButton
					enabled: interactive_
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

					buzzitemlight_.calculateHeight();
				}
			}

			function expand() {
				//
				var lSource;
				var lComponent;

				// expand media
				if (buzzMedia_ && buzzMedia_.length) {
					if (!buzzMediaItem_) {
						lSource = "qrc:/qml/buzzitemmedia.qml";
						lComponent = Qt.createComponent(lSource);
						buzzMediaItem_ = lComponent.createObject(bodyControl);
						buzzMediaItem_.calculatedHeightModified.connect(innerHeightChanged);

						buzzMediaItem_.x = 0;
						buzzMediaItem_.y = bodyControl.getY();
						buzzMediaItem_.calculatedWidth = bodyControl.width;
						buzzMediaItem_.width = bodyControl.width;
						buzzMediaItem_.controller_ = buzzitemlight_.controller_;
						buzzMediaItem_.buzzId_ = buzzitemlight_.buzzId_;
						buzzMediaItem_.buzzBody_ = buzzitemlight_.buzzBodyFlat_;
						buzzMediaItem_.buzzMedia_ = buzzitemlight_.buzzMedia_;
						buzzMediaItem_.sharedMediaPlayer_ = buzzitemlight_.sharedMediaPlayer_;
						buzzMediaItem_.initialize();

						bodyControl.height = bodyControl.getHeight();
						buzzitemlight_.calculateHeight();
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
						urlInfoItem_.controller_ = buzzitemlight_.controller_;
						urlInfoItem_.lastUrl_ = buzzitemlight_.lastUrl_;
						urlInfoItem_.initialize();

						bodyControl.height = bodyControl.getHeight();
						buzzitemlight_.calculateHeight();
					}
				}
			}

			function innerHeightChanged(value) {				
				//bodyControl.height = (buzzBody_.length > 0 ? buzzText.height : 0) + value +
				//							(buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
				//							(buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0);
				bodyControl.height = bodyControl.getHeight();
				buzzitemlight_.calculateHeight();
			}

			function getY() {
				// var lAdjust = buzzMedia_.length > 0 ? 0 : (buzzerClient.scaleFactor * 12);
				return (buzzBody_ && buzzBody_.length > 0 ? buzzText.height : 0) +
						(buzzBody_ && buzzBody_.length > 0 ? spaceMedia_ : spaceItems_) +
						(buzzMedia_ && buzzMedia_.length > 1 ? spaceMediaIndicator_ : 0);
			}

			function getHeight() {
				var lAdjust =
						buzzMedia_ && buzzMedia_.length > 0 ||
						buzzMediaItem_ ||
						urlInfoItem_ && urlInfoItem_.calculatedHeight > 0 ? 0 : (buzzerClient.scaleFactor * 12);

				return (buzzBody_ && buzzBody_.length > 0 ? buzzText.height - lAdjust : 0) +
						(buzzMediaItem_ ? buzzMediaItem_.calculatedHeight : 0) +
						(urlInfoItem_ ? urlInfoItem_.calculatedHeight : 0) +
						(buzzBody_ && buzzBody_.length > 0 && buzzMedia_ && buzzMedia_.length ? spaceMedia_ : 0 /*spaceItems_*/) +
						(buzzMedia_ && buzzMedia_.length > 1 ? (spaceMediaIndicator_ + (buzzerApp.isDesktop ? 2*spaceItems_ : 0)) : spaceBottom_);
			}
		}
	}

	Rectangle {
		//
		id: frameContainer
		x: buzzItemContainer.x
		y: buzzItemContainer.y
		color: "transparent"
		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		width: buzzItemContainer.width
		height: buzzItemContainer.height
		visible: buzzItemContainer.visible
		radius: 8
		clip: true

		ItemDelegate {
			id: expandItem
			x: parent.x
			y: parent.y
			width: parent.width
			height: parent.height
			enabled: interactive_

			onClicked: {
				//
				controller_.openThread(buzzChainId_, buzzId_, buzzerClient.getBuzzerAlias(buzzerInfoId_), buzzBodyFlat_);
			}
		}
	}
}
