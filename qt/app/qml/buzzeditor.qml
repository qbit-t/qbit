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
import "qrc:/lib/dateFunctions.js" as DateFunctions

QuarkPage {
	id: buzzeditor_
	key: "buzzeditor"

	// adjust height by virtual keyboard
	followKeyboard: true

	Component.onCompleted: {
		closePageHandler = closePage;
		activatePageHandler = activatePage;

		startUp.start();
		buzzText.forceActiveFocus();
	}

	function closePage() {
		//
		mediaListEditor.cleanUp();
		stopPage();
		controller.popPage();
		destroy(1000);
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background"));
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

	function stopPage() {
	}

	property bool buzz_: true;
	property bool rebuzz_: false;
	property bool reply_: false;
	property bool message_: false;
	property var buzzfeedModel_;
	property var buzzItem_;
	property var pkey_: "";
	property var conversation_: "";
	property var text_: "";
	property var index_;

	function initializeRebuzz(item, model, index) {
		buzzItem_ = item;
		buzzfeedModel_ = model;
		buzz_ = false;
		rebuzz_ = true;
		message_ = false;
		index_ = index;

		bodyContainer.wrapItem(item);
	}

	function initializeReply(item, model, text) {
		buzzItem_ = item;
		buzzfeedModel_ = model;
		buzz_ = false;
		reply_ = true;
		message_ = false;

		if (text) {
			// buzzText.text = text;
			text_ = text;
			injectText.start();
		}

		bodyContainer.replyItem(item);
	}

	function initializeBuzz(text) {
		buzz_ = true;
		reply_ = false;
		rebuzz_ = false;
		message_ = false;

		if (text) {
			// buzzText.text = text;
			text_ = text;
			injectText.start();
		}
	}

	function initializeMessage(text, pkey, conversation) {
		buzz_ = false;
		reply_ = false;
		rebuzz_ = false;
		message_ = true;
		pkey_ = pkey;
		conversation_ = conversation;

		if (text) buzzText.text = text;
	}

	// spacing
	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceMedia_: 20

	onWidthChanged: {
	}

	// only once to pop-up system keyboard
	Timer {
		id: startUp
		interval: 300
		repeat: false
		running: false

		onTriggered: {
			buzzText.forceActiveFocus();
		}
	}

	// only once to inject outer text
	Timer {
		id: injectText
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			buzzText.text = text_;
		}
	}

	//
	// toolbar
	//
	QuarkToolBar {
		id: buzzEditorToolBar
		height: 45
		width: parent.width

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.cancelSym

			onClicked: {
				controller.popPage();
			}
		}

		QuarkRoundButton {
			id: sendButton
			x: parent.width - width - 12
			y: topOffset + parent.height / 2 - height / 2
			text: buzz_ ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.buzz") :
						  rebuzz_ ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.rebuzz") :
									message_ ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.send") :
											   buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.reply")

			textColor: !sending ?
						 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground") :
						 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")

			onClick: {
				if (!sending) {
					// flagging...
					sending = true;
					// locking from sleep
					buzzerApp.wakeLock();
					//
					buzzerApp.commitCurrentInput();

					if (rebuzz_) buzzeditor_.createRebuzz(buzzItem_.buzzId);
					else if (reply_) buzzeditor_.createReply(buzzItem_.buzzId);
					else if (buzz_) buzzeditor_.createBuzz();
					else if (message_) buzzeditor_.createMessage();
				}
			}
		}

		QuarkHLine {
			id: bottomLine
			x1: 0
			y1: parent.height
			x2: parent.width
			y2: parent.height
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.bottom.separator") //Material.disabledHidden
			visible: true
		}
	}

	property bool sending: false

	//
	// body
	//
	Flickable {
		id: bodyContainer
		x: 0
		y: buzzEditorToolBar.y + buzzEditorToolBar.height
		width: parent.width
		height: parent.height - (buzzEditorToolBar.height + buzzFooterBar.height)
		contentHeight: buzzText.contentHeight + (mediaListEditor.model.count ? mediaBox.height : 0) + spaceMedia_ + spaceBottom_ * 2 +
							wrapContainer.getHeight() + wrapContainer.getSpace() +
							replyContainer.getHeight() + replyContainer.getSpace()
		clip: true

		onHeightChanged: {
			bodyContainer.ensureVisible(buzzText);
		}

		function ensureVisible(item) {
			var lRect = buzzText.positionToRectangle(buzzText.cursorPosition);
			if (height > 0 && item.y + lRect.y + lRect.height + spaceMedia_ > contentY + height) {
				contentY += (item.y + lRect.y + lRect.height + spaceMedia_) - (contentY + height);
			} else if (height > 0 && item.y + lRect.y < contentY) {
				contentY -= (contentY - (item.y + lRect.y));
			}
		}

		BuzzerComponents.BuzzTextHighlighter {
			id: highlighter
			textDocument: buzzText.textDocument

			onMatched: {
				// start, length, match, text
				if (match.length) {
					//
					var lText = buzzerClient.getPlainText(buzzText.textDocument);
					if (!buzzerApp.isDesktop) {
						lText = lText.slice(0, buzzText.cursorPosition) + buzzText.preeditText + lText.slice(buzzText.cursorPosition);
					}
					var lPosition = buzzText.cursorPosition;
					//
					console.log("[onMatched]: match = '" + match + "', start = " + start + ", lPosition = " + lPosition + ", length = " + length);
					console.log("[onMatched]: 1 = " + lText.slice(lPosition - length, lPosition));
					console.log("[onMatched]: 2 = " + lText.slice(lPosition-1, (lPosition-1) + length));
					if (lText.length >= lPosition + length) console.log("[onMatched]: 3 = " + lText.slice(lPosition, lPosition + length));
					if (lText.slice(lPosition - length, lPosition) === match ||
							(!buzzerApp.isDesktop && lText.slice(lPosition-1, (lPosition-1) + length) === match) ||
							(!buzzerApp.isDesktop && lText.length >= lPosition + length &&
													lText.slice(lPosition, lPosition + length) === match)) {
						if (match[0] === '@')
							searchBuzzers.process(match);
						else if (match[0] === '#')
							searchTags.process(match);
					}
				}
			}
		}

		BuzzerCommands.SearchEntityNamesCommand {
			id: searchBuzzers

			onProcessed: {
				// pattern, entities
				var lRect = buzzText.positionToRectangle(buzzText.cursorPosition);
				buzzersList.popup(pattern, buzzText.x + lRect.x + spaceItems_, lRect.y + lRect.height - bodyContainer.contentY, lRect, entities);
			}

			onError: {
				controller.showError(message);
			}
		}

		BuzzerCommands.LoadHashTagsCommand {
			id: searchTags

			onProcessed: {
				// pattern, tags
				var lRect = buzzText.positionToRectangle(buzzText.cursorPosition);
				//console.log("[searchTags/onProcessed]: lRect = " + lRect + ", bodyContainer.contentY = " + bodyContainer.contentY + ", buzzText.y = " + buzzText.y);
				tagsList.popup(pattern, buzzText.x + lRect.x + spaceItems_, lRect.y + lRect.height - bodyContainer.contentY, lRect, tags);
			}

			onError: {
				controller.showError(message);
			}
		}

		Rectangle {
			id: replyContainer
			x: spaceLeft_
			y: spaceTop_
			width: bodyContainer.width - (spaceLeft_ + spaceRight_)
			height: getHeight()
			color: "transparent"

			property var replyItem_;

			function getHeight() {
				if (replyItem_) {
					return replyItem_.calculatedHeight;
				}

				return 0;
			}

			function getSpace() {
				if (replyItem_) return spaceMedia_;
				return 0;
			}
		}

		Rectangle {
			id: quasiCursor

			x: spaceLeft_ + 2
			y: buzzText.y + 2

			width: 1
			height: textMetrics.height

			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

			opacity: 1.0

			TextMetrics	{
				id: textMetrics
				text: "A"
				font.pointSize: buzzText.defaultFontPointSize
			}

			property bool toggleBlink: true

			NumberAnimation on opacity {
				id: blinkOutAnimation
				from: 1.0
				to: 0.0
				duration: 200
				running: quasiCursor.toggleBlink
			}

			NumberAnimation on opacity {
				id: blinkInAnimation
				from: 0.0
				to: 1.0
				duration: 200
				running: !quasiCursor.toggleBlink
			}

			Timer {
				id: blinkTimer
				interval: 500
				repeat: parent.visible
				running: parent.visible

				onTriggered: {
					quasiCursor.toggleBlink = !quasiCursor.toggleBlink
				}
			}
		}

		QuarkTextEdit {
			id: buzzText
			x: spaceLeft_
			y: replyContainer.y + replyContainer.getHeight() + replyContainer.getSpace()
			// height: 1000 //parent.height
			width: parent.width - spaceRight_
			wrapMode: Text.Wrap
			textFormat: Text.RichText

			focus: true
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

			onLengthChanged: {
				// TODO: may by too expensive
				var lText = buzzerClient.getPlainText(buzzText.textDocument);
				countProgress.adjust(buzzerClient.getBuzzBodySize(lText + preeditText.length));
				buzzersList.close();
				tagsList.close();
			}

			property bool buzzerStarted_: false;
			property bool tagStarted_: false;

			onActiveFocusChanged: {
				quasiCursor.visible = !activeFocus;
			}

			onPreeditTextChanged: {
				//
				if (preeditText == " ") {
					buzzerStarted_ = false;
					tagStarted_ = false;
					buzzersList.close();
					tagsList.close();
				} else if (preeditText == "@" || buzzerStarted_) {
					//
					buzzerStarted_ = true;
				} else if (preeditText == "#" || tagStarted_) {
					//
					tagStarted_ = true;
				}

				if (buzzerStarted_ || tagStarted_) {
					var lText = buzzerClient.getPlainText(buzzText.textDocument);
					//highlighter.tryHighlightBlock(lText.slice(0, cursorPosition) + preeditText, 0);
					highlighter.tryHighlightBlock(lText.slice(0, cursorPosition) + preeditText + lText.slice(cursorPosition), 0);
				}
			}

			onContentHeightChanged: {
				bodyContainer.ensureVisible(buzzText);
			}

			onYChanged: {
				bodyContainer.ensureVisible(buzzText);
			}

			onCursorPositionChanged: {
				bodyContainer.ensureVisible(buzzText);
			}
		}

		onWidthChanged: {
			//
			if (replyContainer.replyItem_) {
				replyContainer.replyItem_.width = bodyContainer.width - (spaceLeft_ + spaceRight_);
			}
			//
			if (wrapContainer.wrappedItem_) {
				wrapContainer.wrappedItem_.width = bodyContainer.width - (spaceLeft_ + spaceRight_);
			}
		}

		onFlickingChanged: {
		}

		Rectangle {
			id: wrapContainer
			x: spaceLeft_
			y: buzzText.y + buzzText.contentHeight + spaceMedia_
			width: bodyContainer.width - (spaceLeft_ + spaceRight_)
			height: getHeight()
			color: "transparent"

			property var wrappedItem_;

			function getHeight() {
				if (wrappedItem_) {
					return wrappedItem_.calculatedHeight;
				}

				return 0;
			}

			function getSpace() {
				if (wrappedItem_) return spaceMedia_;
				return 0;
			}
		}

		function wrapItem(item) {
			// buzzText
			var lSource = "qrc:/qml/buzzitemlight.qml";
			var lComponent = Qt.createComponent(lSource);
			wrapContainer.wrappedItem_ = lComponent.createObject(wrapContainer);

			wrapContainer.wrappedItem_.x = 0;
			wrapContainer.wrappedItem_.y = 0;
			wrapContainer.wrappedItem_.width = bodyContainer.width - (spaceLeft_ + spaceRight_);
			wrapContainer.wrappedItem_.controller_ = controller;

			wrapContainer.wrappedItem_.timestamp_ = item.timestamp;
			wrapContainer.wrappedItem_.score_ = item.score;
			wrapContainer.wrappedItem_.buzzId_ = item.buzzId;
			wrapContainer.wrappedItem_.buzzChainId_ = item.buzzChainId;
			wrapContainer.wrappedItem_.buzzerId_ = item.buzzerId;
			wrapContainer.wrappedItem_.buzzerInfoId_ = item.buzzerInfoId;
			wrapContainer.wrappedItem_.buzzerInfoChainId_ = item.buzzerInfoChainId;
			wrapContainer.wrappedItem_.buzzBody_ = buzzerClient.decorateBuzzBody(item.buzzBody);
			wrapContainer.wrappedItem_.buzzMedia_ = item.buzzMedia;
			wrapContainer.wrappedItem_.lastUrl_ = buzzerClient.extractLastUrl(item.buzzBody);
			wrapContainer.wrappedItem_.ago_ = buzzerClient.timestampAgo(item.timestamp);
			wrapContainer.wrappedItem_.initialize();
		}

		function replyItem(item) {
			// buzzText
			var lSource = "qrc:/qml/buzzitemlight.qml";
			var lComponent = Qt.createComponent(lSource);
			replyContainer.replyItem_ = lComponent.createObject(replyContainer);
			replyContainer.replyItem_.calculatedHeightModified.connect(innerHeightChanged);

			replyContainer.replyItem_.x = 0;
			replyContainer.replyItem_.y = 0;
			replyContainer.replyItem_.width = bodyContainer.width - (spaceLeft_ + spaceRight_);
			replyContainer.replyItem_.controller_ = controller;

			replyContainer.replyItem_.timestamp_ = item.timestamp;
			replyContainer.replyItem_.score_ = item.score;
			replyContainer.replyItem_.buzzId_ = item.buzzId;
			replyContainer.replyItem_.buzzChainId_ = item.buzzChainId;
			replyContainer.replyItem_.buzzerId_ = item.buzzerId;
			replyContainer.replyItem_.buzzerInfoId_ = item.buzzerInfoId;
			replyContainer.replyItem_.buzzerInfoChainId_ = item.buzzerInfoChainId;
			replyContainer.replyItem_.buzzBody_ = buzzerClient.decorateBuzzBody(item.buzzBody);
			replyContainer.replyItem_.buzzMedia_ = item.buzzMedia;
			replyContainer.replyItem_.ago_ = buzzerClient.timestampAgo(item.timestamp);
			replyContainer.replyItem_.initialize();

			bodyContainer.ensureVisible(buzzText);
		}

		function innerHeightChanged(value) {
			//console.log("replyItem_.height = " + value);
			bodyContainer.ensureVisible(buzzText);
		}

		//
		// media
		//
		Rectangle {
			id: mediaBox
			color: "transparent"
			x: spaceLeft_ - spaceItems_
			y: getY()
			width: parent.width - (spaceLeft_ + spaceRight_ - (spaceItems_) * 2)
			height: Math.max(mediaListEditor.height, calculatedHeight)
			visible: true

			function getY() {
				var lPos = wrapContainer.getHeight();
				if (lPos) { lPos += wrapContainer.y; lPos += wrapContainer.getSpace(); }

				if (buzzText.contentHeight > 0) {
					return lPos ? lPos : (buzzText.y + buzzText.contentHeight + spaceMedia_);
				}

				return lPos ? lPos : y;
			}

			function orientationChanged() {
				//
				var lOrientation = 0; // vertical
				if (buzzeditor_.width > buzzeditor_.height) lOrientation = 1; // horizontal

				for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
					mediaModel.get(lIdx).mediaItem.adjustOrientation(lOrientation);
				}
			}

			property var calculatedHeight: 400

			QuarkListView {
				id: mediaListEditor
				x: 0
				y: 0
				width: parent.width
				height: parent.height
				clip: true
				orientation: Qt.Horizontal
				layoutDirection:  Qt.LeftToRight
				snapMode: ListView.SnapOneItem

				cacheBuffer: 100000

				function cleanUp() {
					//
					for (var lIdx = 0; lIdx < mediaListEditor.count; lIdx++) {
						var lItem = mediaListEditor.itemAtIndex(lIdx);
						if (lItem && lItem.mediaItem) {
							lItem.mediaItem.terminate();
						}
					}
				}

				onContentXChanged: {
					mediaIndicator.currentIndex = indexAt(contentX, 1);
				}

				add: Transition {
					NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
					//NumberAnimation { property: "height"; from: 0; to: mediaListEditor.width; duration: 400 }
				}

				remove: Transition {
					NumberAnimation { property: "opacity"; from: 1.0; to: 0; duration: 400 }
					NumberAnimation { property: "height"; from: mediaListEditor.width; to: 0; duration: 400 }
				}

				displaced: Transition {
					NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.OutElastic }
				}

				model: ListModel { id: mediaModel }

				delegate: Item {
					id: itemDelegate

					property var mediaItem;

					function adjustHeight(proposed) {
						//
						if (index === mediaIndicator.currentIndex) {
							//
							itemDelegate.height = proposed;
						}
					}

					onWidthChanged: {
						if (mediaItem) {
							mediaItem.width = mediaListEditor.width;
							itemDelegate.height = mediaItem.height;
						}
					}

					Component.onCompleted: {
						var lSource = "qrc:/qml/buzzitemmedia-editor-image.qml";
						if (media === "audio") {
							lSource = "qrc:/qml/buzzitemmedia-editor-audio.qml";
						} else if (media === "video") {
							lSource = "qrc:/qml/buzzitemmedia-editor-video.qml";
						}

						var lComponent = Qt.createComponent(lSource);
						mediaItem = lComponent.createObject(itemDelegate);
						if (media === "video") {
							mediaItem.adjustDuration.connect(adjustDuration);
							mediaItem.adjustHeight.connect(adjustHeight);
						}

						if (media === "image") {
							mediaItem.adjustHeight.connect(adjustHeight);
						}

						mediaItem.width = mediaListEditor.width;
						mediaItem.mediaList = mediaListEditor;
						mediaItem.mediaBox = mediaBox;

						itemDelegate.height = mediaItem.height;
						itemDelegate.width = mediaListEditor.width;
					}

					function adjustDuration(value) {
						//
						for (var lIdx = 0; lIdx < mediaModel.count; lIdx++) {
							if(mediaModel.get(lIdx).key === key) {
								//
								duration = value;
								console.log("[adjustDuration]: vale = " + duration);
							}
						}
					}
				}

				function mediaImagesSize() {
					//
					var lSize = 0;
					for (var lIdx = 0; lIdx < mediaModel.count; lIdx++) {
						//
						var lMedia = mediaModel.get(lIdx);
						if (lMedia && lMedia.media === "image") {
							lSize += lMedia.size;
						}
					}

					return lSize;
				}

				function addMedia(file, info) {
					var lOrientation = 0; // vertical
					if (buzzeditor_.width > buzzeditor_.height) lOrientation = 1; // horizontal

					mediaModel.append({
						key: file,
						path: "file://" + file,
						preview: "none",
						media: "image",
						size: buzzerApp.getFileSize(file),
						duration: 0,
						progress: 0,
						uploaded: 0,
						orientation: lOrientation,
						processing: 0,
						description: info});

					positionViewAtEnd();
				}

				function addAudio(file, duration, info) {
					var lOrientation = 0; // vertical
					if (buzzeditor_.width > buzzeditor_.height) lOrientation = 1; // horizontal

					mediaModel.append({
						key: file,
						path: "file://" + file,
						preview: "none",
						media: "audio",
						size: buzzerApp.getFileSize(file),
						duration: duration,
						progress: 0,
						uploaded: 0,
						orientation: lOrientation,
						processing: 0,
						description: info});

					positionViewAtEnd();
				}

				function addVideo(file, duration, orientation, preview, info) {
					/*
					var lOrientation = 0; // vertical
					if (buzzeditor_.width > buzzeditor_.height) lOrientation = 1; // horizontal
					*/
					mediaModel.append({
						key: file,
						path: "file://" + file,
						preview: preview,
						media: "video",
						size: buzzerApp.getFileSize(file),
						duration: duration,
						progress: 0,
						uploaded: 0,
						orientation: orientation,
						processing: 0,
						description: info});

					positionViewAtEnd();
				}

				function removeMedia(index) {
					mediaModel.remove(index);
				}
			}

			QuarkLabel {
				id: mediaIndicator
				property var defaultFontSize: 11
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : defaultFontSize + 1
				text: "0/0"
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
				visible: count > 1

				anchors.bottom: parent.top
				anchors.horizontalCenter: parent.horizontalCenter

				property var count: mediaModel.count
				property var currentIndex: mediaListEditor.currentIndex

				onCurrentIndexChanged: {
					text = (currentIndex + 1) + "/" + count;
				}
			}
		}
	}

	//
	// footer bar
	//
	QuarkToolBar {
		id: buzzFooterBar
		height: 45
		width: parent.width
		y: parent.height - 45

		property int totalHeight: height

		backgroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabBackground")

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: addFromGalleryButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: !sending ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
								   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.imagesSym

			focusPolicy: Qt.NoFocus

			y: parent.height / 2 - height / 2

			onClicked: {
				if (!sending)
					//imageListing.listImages();
					buzzerApp.pickImageFromGallery();
			}
		}

		QuarkToolButton {
			id: addPhotoButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.cameraSym

			x: addFromGalleryButton.x + addFromGalleryButton.width // + spaceItems_
			y: parent.height / 2 - height / 2

			focusPolicy: Qt.NoFocus

			onClicked: {
				//
				if (!buzzerApp.isDesktop && mediaListEditor.mediaImagesSize() > 50*1024*1024) {
					handleError("E_IMAGES_MEDIA_SIZE_EXCEEDED", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_IMAGES_MEDIA_SIZE_EXCEEDED"));
					return;
				}

				//
				if (sending) return;

				var lComponent = null;
				var lPage = null;

				lComponent = Qt.createComponent("qrc:/qml/camera.qml");
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = controller;
					lPage.dataReady.connect(photoTaken);

					addPage(lPage);

					lPage.focusPolicy = Qt.StrongFocus;
					lPage.forceActiveFocus(Qt.MouseFocusReason);
				}
			}

			function photoTaken(path) {
				mediaListEditor.addMedia(path, "none");
			}
		}

		QuarkToolButton {
			id: addVideoButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.videoSym

			x: addPhotoButton.x + addPhotoButton.width // + spaceItems_
			y: parent.height / 2 - height / 2

			focusPolicy: Qt.NoFocus

			onClicked: {
				if (sending) return;

				var lComponent = null;
				var lPage = null;

				lComponent = Qt.createComponent("qrc:/qml/camera-video.qml");
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = controller;
					lPage.videoReady.connect(videoTaken);

					addPage(lPage);

					lPage.focusPolicy = Qt.StrongFocus;
					lPage.forceActiveFocus(Qt.MouseFocusReason);
				}
			}

			function videoTaken(path, duration, orientation, preview) {
				var lPreviewSize = buzzerApp.getFileSize(preview);
				if (lPreviewSize)
					mediaListEditor.addVideo(path, duration, orientation, preview, "none");
				else
					handleError("E_PREVIEW_FILE_WAS_NOT_FOUND", "Preview file was not found.");
			}
		}

		QuarkRoundState {
			id: addAudioFrame
			x: addAudioButton.x + 6
			y: addAudioButton.y + 6
			size: addAudioButton.width - 12
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hiddenLight")
			background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
			visible: audioRecorder.isRecording
		}

		QuarkToolButton {
			id: addAudioButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor:
				audioRecorder.isRecording ?
					buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.event.like") :
					(!sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled"))
			Layout.alignment: Qt.AlignHCenter
			symbol: audioRecorder.isRecording ? Fonts.dotCircle2Sym : Fonts.microphoneSym

			x: addVideoButton.x + addVideoButton.width // + spaceItems_
			y: parent.height / 2 - height / 2

			focusPolicy: Qt.NoFocus

			onClicked: {
				if (audioRecorder.isRecording) {
					symbol = Fonts.microphoneSym;
					audioRecorder.stop();
					recordingAudio.stop();
				} else {
					symbol = Fonts.dotCircle2Sym;
					audioRecorder.record();
					recordingAudio.start();
				}
			}

			function adjust() {
				symbol = audioRecorder.isRecording ? Fonts.dotCircle3Sym : Fonts.microphoneSym;
			}

			Rectangle {
				id: fill
				x: addAudioButton.contentItem.x
				y: addAudioButton.contentItem.y
				width: addAudioButton.contentItem.width
				height: width
				radius: width / 2
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.banned")
				visible: audioRecorder.isRecording
			}
		}

		QuarkLabel {
			id: elapsedAudioTime
			x: addAudioButton.x + addAudioButton.width + spaceItems_
			y: addAudioButton.y + addAudioButton.height / 2 - height / 2
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
			text: "00:00"
			visible: audioRecorder.isRecording

			function setTime(ms) {
				text = DateFunctions.msToTimeString(ms);
			}
		}

		ProgressBar {
			id: createProgressBar

			x: addAudioButton.x + addAudioButton.width + spaceItems_
			y: parent.height / 2 - height / 2
			width: countProgress.x - x - spaceRight_
			visible: false
			value: 0.0

			Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
			Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
			Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
			Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
		}

		QuarkRoundProgress {
			id: addAudioProgress
			x: addAudioButton.x + 6
			y: addAudioButton.y + 6
			size: addAudioButton.width - 12
			colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
			colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
			arcBegin: 0
			arcEnd: progress
			lineWidth: 2
			visible: audioRecorder.isRecording

			property var progress: 0;
		}

		QuarkRoundState {
			id: hiddenCountFrame
			x: parent.width - (size + spaceRight_)
			y: parent.height / 2 - size / 2
			size: 28
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hiddenLight")
			background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabBackground") //Page.background
		}

		QuarkRoundProgress {
			id: countProgress
			x: hiddenCountFrame.x
			y: hiddenCountFrame.y
			size: hiddenCountFrame.size
			colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
			colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
			arcBegin: 0
			arcEnd: 0
			lineWidth: 3

			function adjust(length) {
				arcEnd = (length * 360) / buzzerClient.getBuzzBodyMaxSize();
			}
		}

		QuarkHLine {
			id: topLine
			x1: 0
			y1: 0
			x2: parent.width
			y2: 0
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.top.separator") //Market.tabBackground
			visible: true
		}
	}

	//
	// menus and dropdowns
	//

	QuarkPopupMenu {
		id: buzzersList
		width: 170
		visible: false

		model: ListModel { id: buzzersModel }

		property var matched;

		onClick: {
			//
			var lPos = buzzText.cursorPosition; // current
			var lText = buzzerClient.getPlainText(buzzText.textDocument) + "";
			lText = lText.slice(0, lPos) + buzzText.preeditText + lText.slice(lPos);

			for (var lIdx = lPos; lIdx >= 0; lIdx--) {
				if (lText[lIdx] === '@') {
					break;
				}
			}

			var lNewText = lText.slice(0, lIdx);
			if (lNewText.length && lNewText[lNewText.length-1] !== ' ') lNewText += " ";
			lNewText += key + lText.slice(lPos + buzzText.preeditText.length);

			buzzText.clear();

			var lParagraphs = lNewText.split("\n");
			for (var lChunk = 0; lChunk < lParagraphs.length; lChunk++) {
				buzzText.append(lParagraphs[lChunk]);
			}

			buzzText.cursorPosition = lPos + key.length;
		}

		function popup(match, nx, ny, rect, buzzers) {
			//
			if (buzzers.length === 0) return;
			if (buzzers.length === 1 && match === buzzers[0]) return;
			//
			matched = match;
			//
			if (nx + width > parent.width) x = parent.width - width;
			else x = nx;

			buzzersModel.clear();

			for (var lIdx = 0; lIdx < buzzers.length; lIdx++) {
				buzzersModel.append({
					key: buzzers[lIdx],
					keySymbol: "",
					name: buzzers[lIdx]});
			}

			var lNewY = ny + buzzEditorToolBar.y + buzzEditorToolBar.height + buzzText.y + 5;
			var lAbove = (ny + buzzText.y) - bodyContainer.contentY;
			var lBelow = ((bodyContainer.contentY + bodyContainer.height) - lAbove);
			//console.info("[popup]: lAbove = " + lAbove + ", lBelow = " + lBelow + ", bodyContainer.contentY = " + bodyContainer.contentY + ", bodyContainer.height = " + bodyContainer.height);
			//if (lNewY + (buzzers.length * itemHeight/*height*/) > bodyContainer.height && lNewY - (((buzzers.length) * itemHeight) + 5 + rect.height) > bodyContainer.contentY)
			if (lAbove > lBelow)
				y = lNewY - (((buzzers.length) * itemHeight) + 5 + rect.height);
			else
				y = lNewY;

			open();
		}
	}

	QuarkPopupMenu {
		id: tagsList
		width: 170
		visible: false

		model: ListModel { id: tagsModel }

		property var matched;

		onClick: {
			//
			var lPos = buzzText.cursorPosition; // current
			var lText = buzzerClient.getPlainText(buzzText.textDocument) + "";
			lText = lText.slice(0, lPos) + buzzText.preeditText + lText.slice(lPos);

			for (var lIdx = lPos; lIdx >= 0; lIdx--) {
				if (lText[lIdx] === '#') {
					break;
				}
			}

			var lNewText = lText.slice(0, lIdx);
			if (lNewText.length && lNewText[lNewText.length-1] !== ' ') lNewText += " ";
			lNewText += key + lText.slice(lPos + buzzText.preeditText.length);

			buzzText.clear();

			var lParagraphs = lNewText.split("\n");
			for (var lChunk = 0; lChunk < lParagraphs.length; lChunk++) {
				buzzText.append(lParagraphs[lChunk]);
			}

			buzzText.cursorPosition = lPos + key.length;
		}

		function popup(match, nx, ny, rect, tags) {
			//
			if (tags.length === 0) return;
			if (tags.length === 1 && match === tags[0]) return;
			//
			matched = match;
			//
			if (nx + width > parent.width) x = parent.width - width;
			else x = nx;

			tagsModel.clear();

			for (var lIdx = 0; lIdx < tags.length; lIdx++) {
				tagsModel.append({
					key: tags[lIdx],
					keySymbol: "",
					name: tags[lIdx]});
			}

			var lNewY = ny + buzzEditorToolBar.y + buzzEditorToolBar.height + buzzText.y + 5;
			var lAbove = (ny + buzzText.y) - bodyContainer.contentY;
			var lBelow = ((bodyContainer.contentY + bodyContainer.height) - lAbove);
			//console.info("[popup]: lAbove = " + lAbove + ", lBelow = " + lBelow + ", bodyContainer.contentY = " + bodyContainer.contentY + ", bodyContainer.height = " + bodyContainer.height);
			//if (lNewY + (tags.length * itemHeight/*height*/) > bodyContainer.height && lNewY - (((tags.length) * itemHeight) + 5 + rect.height) > bodyContainer.contentY)
			if (lAbove > lBelow)
				y = lNewY - (((tags.length) * itemHeight) + 5 + rect.height);
			else
				y = lNewY;

			open();
		}
	}

	//
	// Image listing - gallery
	//

	BuzzerComponents.ImageListing {
		id: imageListing

		onImageFound:  {
			mediaListEditor.addMedia(file);
		}
	}

	Connections {
		id: selectImage
		target: buzzerApp

		function onFileSelected(file, preview, description) {
			//
			if (mediaModel.count < 31) {
				//
				var lSize = buzzerApp.getFileSize(file);
				var lPreviewSize = buzzerApp.getFileSize(preview);
				console.log("[onFileSelected]: file = " + file + "/" + lSize + ", preview = " + preview + "/" + lPreviewSize);
				//
				if ((file.includes(".mp4") || file.includes(".mp3") || file.includes(".m4a")) && preview !== "") {
					mediaListEditor.addVideo(file, 0, 0, preview, description);
				} else if (file.includes(".mp3") || file.includes(".m4a")) {
					mediaListEditor.addAudio(file, 0, description);
				} else if (file.includes(".mp4")) {
					handleError("E_MEDIA_PREVIEW_ABSENT", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_MEDIA_PREVIEW_ABSENT"));
				} else {
					//
					if (!buzzerApp.isDesktop && mediaListEditor.mediaImagesSize() > 50*1024*1024) {
						handleError("E_IMAGES_MEDIA_SIZE_EXCEEDED", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_IMAGES_MEDIA_SIZE_EXCEEDED"));
						return;
					}

					mediaListEditor.addMedia(file, description);
				}
			} else {
				handleError("E_MAX_ATTACHMENTS", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_MAX_ATTACHMENTS"));
			}
		}
	}

	//
	// Backend
	//

	// recording control
	Timer {
		id: recordingAudio
		interval: 1000
		repeat: true
		running: false

		onTriggered: {
			//
			if (audioRecorder.duration >= 5 * 60 * 1000) {
				// we are done
				addAudioButton.symbol = Fonts.microphoneSym;
				audioRecorder.stop();
				stop();
			}
		}
	}

	BuzzerComponents.AudioRecorder {
		id: audioRecorder
		localPath: buzzerClient.getTempFilesPath()

		onDurationChanged: {
			// max 5 min
			var lPart = (duration * 100.0) / (5 * 60 * 1000);
			addAudioProgress.progress = lPart * 360.0 / 100.0;
			elapsedAudioTime.setTime(duration);
		}

		onStopped: {
			//
			if (actualFileLocation !== "") {
				// we have location and content saved
				mediaListEditor.addAudio(audioRecorder.actualFileLocation, duration, "none");
			}
		}

		onActualFileLocationChanged: {
			//
			if (isStopped) {
				// we have location and content saved
				mediaListEditor.addAudio(audioRecorder.actualFileLocation, duration, "none");
			}
		}

		onIsRecordingChanged: {
			// console.log("[onIsRecordingChanged]: " + isRecording);
		}
	}

	BuzzerCommands.UploadMediaCommand {
		id: uploadBuzzMedia
		pkey: pkey_

		onProgress: {
			//
			var lPart = 100 / mediaModel.count;

			// file, pos, size
			for (var lIdx = 0; lIdx < mediaModel.count; lIdx++) {
				if(mediaModel.get(lIdx).key === file) {
					//
					if (pos > 0) createProgressBar.indeterminate = false;

					// update media progress
					var lPercent = (pos * 100) / size;
					var lArc = (360 * lPercent) / 100;
					mediaModel.get(lIdx).progress = lArc;

					if (lPercent === 100.0) mediaModel.get(lIdx).uploaded = 1;

					// update global
					var lCurrent = (lIdx * lPart);
					lCurrent += (lPercent * lPart) / 100.0;
					createProgressBar.value = lCurrent / 100.0;
					break;
				}
			}
		}
	}

	BuzzerCommands.BuzzCommand {
		id: buzzCommand
		uploadCommand: uploadBuzzMedia

		onProcessed: {
			//
			buzzerApp.wakeRelease();
			//
			mediaListEditor.cleanUp();
			createProgressBar.value = 1.0;
			controller.popPage();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryBuzzCommand.start();
		}

		onError: {
			createProgressBar.visible = false;
			sending = false;
			handleError(code, message);
		}

		onMediaUploaded: {
			createProgressBar.indeterminate = true;
		}
	}

	Timer {
		id: retryBuzzCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			buzzCommand.reprocess();
		}
	}

	BuzzerCommands.ReBuzzCommand {
		id: rebuzzCommand
		uploadCommand: uploadBuzzMedia
		model: buzzfeedModel_

		onProcessed: {
			//
			buzzerApp.wakeRelease();
			//
			if (buzzfeedModel_ && index_ !== undefined)
				buzzfeedModel_.setHasRebuzz(index_);
			//
			mediaListEditor.cleanUp();
			createProgressBar.value = 1.0;
			controller.popPage();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryReBuzzCommand.start();
		}

		onError: {
			createProgressBar.visible = false;
			sending = false;
			handleError(code, message);
		}

		onMediaUploaded: {
			createProgressBar.indeterminate = true;
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

	BuzzerCommands.ReplyCommand {
		id: replyCommand
		uploadCommand: uploadBuzzMedia
		model: buzzfeedModel_ // TODO: implement property buzzChainId (with buzzfeedModel in parallel)

		onProcessed: {
			//
			buzzerApp.wakeRelease();
			//
			mediaListEditor.cleanUp();
			createProgressBar.value = 1.0;
			controller.popPage();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryReplyCommand.start();
		}

		onError: {
			createProgressBar.visible = false;
			sending = false;
			handleError(code, message);
		}

		onMediaUploaded: {
			createProgressBar.indeterminate = true;
		}
	}

	Timer {
		id: retryReplyCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			replyCommand.reprocess();
		}
	}

	BuzzerCommands.ConversationMessageCommand {
		id: messageCommand
		uploadCommand: uploadBuzzMedia

		onProcessed: {
			//
			buzzerApp.wakeRelease();
			//
			mediaListEditor.cleanUp();
			createProgressBar.value = 1.0;
			controller.popPage();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryConversationMessageCommand.start();
		}

		onError: {
			createProgressBar.visible = false;
			sending = false;
			handleError(code, message);
		}

		onMediaUploaded: {
			createProgressBar.indeterminate = true;
		}
	}

	Timer {
		id: retryConversationMessageCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			messageCommand.reprocess();
		}
	}

	function createBuzz() {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;
		if (lText.includes("/>") || lText.includes("</")) { // TODO: implement more common approach
			handleError("E_BUZZ_UNSUPPORTED_HTML", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_UNSUPPORTED_HTML"));
			sending = false;
			return;
		}

		//
		mediaListEditor.cleanUp();

		if (lText.length === 0 && mediaModel.count === 0) {
			handleError("E_BUZZ_IS_EMPTY", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_EMPTY"));
			sending = false;
			return;
		}

		if (buzzerClient.getBuzzBodySize(lText) >= buzzerClient.getBuzzBodyMaxSize()) {
			handleError("E_BUZZ_IS_TOO_BIG", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_TOO_BIG"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;
		//
		buzzCommand.clear();
		//
		buzzCommand.buzzBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(buzzCommand.buzzBody);
		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			buzzCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			console.log("[createBuzz/media]: key = " + mediaModel.get(lIdx).key + ", preview = " + mediaModel.get(lIdx).preview);
			buzzCommand.addMedia(mediaModel.get(lIdx).key + "," +
								 mediaModel.get(lIdx).duration + "," +
								 mediaModel.get(lIdx).preview + "," +
								 mediaModel.get(lIdx).orientation + "," +
								 mediaModel.get(lIdx).description);
		}

		buzzCommand.process();
	}

	function createRebuzz(buzzId) {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;
		if (lText.includes("/>") || lText.includes("</")) { // TODO: implement more common approach
			handleError("E_BUZZ_UNSUPPORTED_HTML", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_UNSUPPORTED_HTML"));
			sending = false;
			return;
		}

		//
		mediaListEditor.cleanUp();

		if (buzzerClient.getBuzzBodySize(lText) >= buzzerClient.getBuzzBodyMaxSize()) {
			handleError("E_BUZZ_IS_TOO_BIG", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_TOO_BIG"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;
		//
		rebuzzCommand.clear();
		//
		rebuzzCommand.buzzBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(rebuzzCommand.buzzBody);
		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			rebuzzCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			rebuzzCommand.addMedia(mediaModel.get(lIdx).key + "," +
								   mediaModel.get(lIdx).duration + "," +
								   mediaModel.get(lIdx).preview + "," +
								   mediaModel.get(lIdx).orientation + "," +
								   mediaModel.get(lIdx).description);
		}

		if (buzzId) rebuzzCommand.buzzId = buzzId;

		rebuzzCommand.process();
	}

	function createReply(buzzId) {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;
		if (lText.includes("/>") || lText.includes("</")) { // TODO: implement more common approach
			handleError("E_BUZZ_UNSUPPORTED_HTML", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_UNSUPPORTED_HTML"));
			sending = false;
			return;
		}

		//
		mediaListEditor.cleanUp();

		if (lText.length === 0 && mediaModel.count === 0) {
			handleError("E_BUZZ_IS_EMPTY", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_EMPTY"));
			sending = false;
			return;
		}

		if (buzzerClient.getBuzzBodySize(lText) >= buzzerClient.getBuzzBodyMaxSize()) {
			handleError("E_BUZZ_IS_TOO_BIG", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_TOO_BIG"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;
		//
		replyCommand.clear();
		//
		replyCommand.buzzBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(replyCommand.buzzBody);
		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			replyCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			replyCommand.addMedia(mediaModel.get(lIdx).key + "," +
								  mediaModel.get(lIdx).duration + "," +
								  mediaModel.get(lIdx).preview + "," +
								  mediaModel.get(lIdx).orientation + "," +
								  mediaModel.get(lIdx).description);
		}

		if (buzzId) replyCommand.buzzId = buzzId;

		replyCommand.process();
	}

	function createMessage() {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;
		if (lText.includes("/>") || lText.includes("</")) { // TODO: implement more common approach
			handleError("E_BUZZ_UNSUPPORTED_HTML", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_UNSUPPORTED_HTML"));
			sending = false;
			return;
		}

		//
		mediaListEditor.cleanUp();

		if (lText.length === 0 && mediaModel.count === 0) {
			handleError("E_BUZZ_IS_EMPTY", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_EMPTY"));
			sending = false;
			return;
		}

		if (buzzerClient.getBuzzBodySize(lText) >= buzzerClient.getBuzzBodyMaxSize()) {
			handleError("E_BUZZ_IS_TOO_BIG", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_TOO_BIG"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;
		//
		messageCommand.clear();
		//
		if (conversation_ !== "") messageCommand.conversation = conversation_;
		messageCommand.messageBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(messageCommand.messageBody);
		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			messageCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			messageCommand.addMedia(mediaModel.get(lIdx).key + "," +
									mediaModel.get(lIdx).duration + "," +
									mediaModel.get(lIdx).preview + "," +
									mediaModel.get(lIdx).orientation + "," +
									mediaModel.get(lIdx).description);
		}

		messageCommand.process();
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT" ||
				((code === "E_SENT_TX" || code === "E_UPLOAD_DATA") && message.includes("UNKNOWN_REFTX"))) {
			// a little tricky, but for now
			// we need to RE-process failed upload, but now there is no uploads chain recovery present
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
		} else {
			controller.showError(message, true);
		}
	}
}
