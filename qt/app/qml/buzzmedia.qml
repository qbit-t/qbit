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

QuarkPage {
	id: buzzmedia_
	key: "buzzmedia"
	stacked: false

	//
	property string mediaViewTheme: "Darkmatter"
	property string mediaViewSelector: "dark"

	// mandatory
	property var buzzMedia_;
	property var mediaPlayerControler;
	property bool initialized_: false;
	property var pkey_: ""
	property var mediaIndex_: 0
	property var mediaPlayer_: null
	property var buzzBody_: ""

	statusBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.statusBar")
	navigationBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.navigationBar")
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground")
	pageTheme: "dark"

	Component.onCompleted: {
		closePageHandler = closePage;
		activatePageHandler = activatePage;

		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground"));
	}

	onMediaPlayerControlerChanged: {
		//
		if (mediaContainer.buzzMediaItem_)
			mediaContainer.buzzMediaItem_.sharedMediaPlayer_ = mediaPlayerControler;
	}

	onControllerChanged: {
	}

	function closePage() {
		stopPage();
		controller.popPage();
		destroy(1000);

		// set back
		statusBarColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.statusBar")
		navigationBarColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.navigationBar")
		pageTheme = buzzerClient.themeSelector;
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

	function stopPage() {
		//
		mediaContainer.checkPlaying();
		//
		playerControl.unlink();
		//
		if (mediaPlayerControler)
			mediaPlayerControler.popVideoInstance();
	}

	function initialize(pkey, mediaIndex, player, buzzBody) {
		//
		if (key !== undefined) pkey_ = pkey;
		mediaIndex_ = mediaIndex;
		mediaPlayer_ = player;
		buzzBody_ = buzzBody;

		console.log("[buzzmedia/initialize]: mediaIndex = " + mediaIndex + ", player = " + player);

		mediaContainer.initialize();
		//
		initialized_ = true;
	}

	function adjustToolBar() {
		if (initialized_ && !buzzerApp.isDesktop) {
			//console.log("w = " + width + ", h = " + height);
			if (width > height && height > 0) {
				if (buzzMedia_.length === 1) {
					buzzMediaToolBar.height = 0;
					bottomLine.visible = false;
					cancelButton.visible = false;
				} else {
					buzzMediaToolBar.height = 20;
					cancelButton.visible = false;
				}
			} else {
				buzzMediaToolBar.height = 45;
				cancelButton.visible = true;
				bottomLine.visible = true;
			}
		}
	}

	onWidthChanged: {
		//
		adjustToolBar();
		orientationChangedTimer.start();
	}

	onHeightChanged: {
		adjustToolBar();
	}

	// to adjust model
	Timer {
		id: orientationChangedTimer
		interval: 100
		repeat: false
		running: false

		onTriggered: {
			mediaContainer.adjustOrientation();
		}
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

	//
	// toolbar
	//
	QuarkToolBar {
		id: buzzMediaToolBar
		height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 45
		width: parent.width
		backgroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground")

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
			y: parent.height / 2 - height / 2
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.cancelSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

			onClicked: {
				closePage();
			}
		}

		QuarkHLine {
			id: bottomLine
			x1: 0
			y1: parent.height
			x2: parent.width
			y2: parent.height
			penWidth: 1
			color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.disabledHidden")
			visible: true
		}
	}

	//
	// buzz gallery
	//
	Rectangle {
		id: mediaContainer

		x: 0
		y: buzzMediaToolBar.y + buzzMediaToolBar.height // + spaceTop_
		width: parent.width
		height: parent.height - buzzMediaToolBar.height // - spaceTop_
		color: "transparent"

		property var buzzMediaItem_;

		function adjustOrientation() {
			if (buzzMediaItem_) {
				buzzMediaItem_.calculatedWidth = mediaContainer.width;
				buzzMediaItem_.calculatedHeight = mediaContainer.height;
				buzzMediaItem_.adjust();
			}
		}

		function checkPlaying() {
			//
			if (buzzMediaItem_) {
				buzzMediaItem_.checkPlaying();
			}
		}

		function initialize() {
			//
			var lSource = "qrc:/qml/buzzitemmediaview.qml";
			var lComponent = Qt.createComponent(lSource);

			if (lComponent.status === Component.Error) {
				controller.showError(lComponent.errorString());
			} else {
				buzzMediaItem_ = lComponent.createObject(mediaContainer);

				buzzMediaItem_.x = 0;
				buzzMediaItem_.y = 0;
				buzzMediaItem_.calculatedWidth = mediaContainer.width;
				buzzMediaItem_.calculatedHeight = mediaContainer.height;
				//buzzMediaItem_.width = mediaContainer.width;
				//buzzMediaItem_.height = mediaContainer.height;
				buzzMediaItem_.controller_ = buzzmedia_.controller;
				buzzMediaItem_.buzzMedia_ = buzzmedia_.buzzMedia_;
				buzzMediaItem_.sharedMediaPlayer_ = buzzmedia_.mediaPlayerControler;
				buzzMediaItem_.initialize(pkey_, mediaIndex_, mediaPlayer_, buzzBody_);
			}
		}

		onWidthChanged: {
			if (buzzMediaItem_) {
				buzzMediaItem_.calculatedWidth = mediaContainer.width;
				buzzMediaItem_.adjust();
			}
		}

		onHeightChanged: {
			if (buzzMediaItem_) {
				buzzMediaItem_.calculatedHeight = mediaContainer.height;
				buzzMediaItem_.adjust();
			}
		}
	}

	//
	BuzzItemMediaPlayer {
		id: playerControl
		x: 0
		y: buzzMediaToolBar.y + buzzMediaToolBar.height
		width: parent.width
		mediaPlayerControler: buzzmedia_.mediaPlayerControler
	}
}
