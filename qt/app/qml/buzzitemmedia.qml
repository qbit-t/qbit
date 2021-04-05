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
	id: buzzitemmedia_

	property int calculatedHeight: 400
	property int calculatedWidth: 500
	property int calculatedWidthInternal: 500
	property var buzzId_: buzzId
	property var buzzMedia_: buzzMedia
	property var controller_: controller

	readonly property int maxCalculatedWidth_: 600
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
	readonly property real defaultFontSize: 11

	property var pkey_: ""

	signal calculatedHeightModified(var value);
	
	onCalculatedHeightChanged: calculatedHeightModified(calculatedHeight);

	onCalculatedWidthChanged: {
		//
		if (calculatedWidth > maxCalculatedWidth_) {
			calculatedWidth = maxCalculatedWidth_;
		}

		calculatedWidthInternal = calculatedWidth;
	}

	Component.onCompleted: {
	}

	function initialize(key) {
		if (key !== undefined) pkey_ = key;
		mediaList.prepare();
	}

	//
	// media list
	//

	QuarkListView {
		id: mediaList
		x: 0
		y: 0
		width: calculatedWidthInternal
		height: calculatedHeight
		clip: true
		orientation: Qt.Horizontal
		layoutDirection:  Qt.LeftToRight
		snapMode: ListView.SnapOneItem

		onContentXChanged: {
			mediaIndicator.currentIndex = indexAt(contentX, 1);
		}

		add: Transition {
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
		}

		model: ListModel { id: mediaModel }

		delegate: Rectangle {
			//
			id: imageFrame
			color: "transparent"
			width: mediaImage.width + 2 * spaceItems_
			height: mediaImage.height //calculatedHeight

			Image {
				id: mediaImage
				autoTransform: true
				asynchronous: true

				x: spaceItems_
				y: 0

				width: mediaList.width - spaceItems_
				height: calculatedHeight
				fillMode: Image.PreserveAspectCrop
				mipmap: true

				source: path_

				Component.onCompleted: {
					mediaList.height = Math.max(buzzitemmedia_.calculatedHeight, height);
					buzzitemmedia_.calculatedHeight = mediaList.height;
				}

				onStatusChanged: {
					//
					if (status == Image.Error) {
						// force to reload
						console.log("[onStatusChanged]: forcing reload of " + path_);
						path_ = "";
						downloadCommand.process(true);
					}
				}

				MouseArea {
					id: linkClick
					x: 0
					y: 0
					width: mediaImage.width
					height: mediaImage.height
					enabled: true
					cursorShape: Qt.PointingHandCursor

					ItemDelegate {
						id: linkClicked
						x: 0
						y: 0
						width: mediaImage.width
						height: mediaImage.height
						enabled: true

						onClicked: {
							// expand
							var lSource;
							var lComponent;

							// gallery
							lSource = "qrc:/qml/buzzmedia.qml";
							lComponent = Qt.createComponent(lSource);

							if (lComponent.status === Component.Error) {
								controller_.showError(lComponent.errorString());
							} else {
								var lMedia = lComponent.createObject(controller_);
								lMedia.controller = controller_;
								lMedia.buzzMedia_ = buzzitemmedia_.buzzMedia_;
								lMedia.initialize(pkey_);
								controller_.addPage(lMedia);
							}
						}
					}
				}

				layer.enabled: true
				layer.effect: OpacityMask {
					id: roundEffect
					maskSource: Item {
						width: roundEffect.getWidth()
						height: roundEffect.getHeight()

						Rectangle {
							//anchors.centerIn: parent
							x: roundEffect.getX()
							y: roundEffect.getY()
							width: roundEffect.getWidth()
							height: roundEffect.getHeight()
							radius: 8
						}
					}

					function getX() {
						return 0;
					}

					function getY() {
						return 0;
					}

					function getWidth() {
						return mediaImage.width;
					}

					function getHeight() {
						return mediaImage.height;
					}
				}
			}

			QuarkRoundProgress {
				id: imageLoading
				x: mediaList.width / 2 - width / 2
				y: mediaList.height / 2 - height / 2
				size: buzzerClient.scaleFactor * 50
				colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
				colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
				arcBegin: 0
				arcEnd: 0
				lineWidth: buzzerClient.scaleFactor * 3
				visible: false

				QuarkSymbolLabel {
					id: waitSymbol
					anchors.fill: parent
					symbol: Fonts.clockSym
					font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (imageLoading.size-10)) : (imageLoading.size-10)
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
					visible: imageLoading.visible
				}

				function progress(pos, size) {
					//
					waitSymbol.visible = false;
					//
					var lPercent = (pos * 100) / size;
					arcEnd = (360 * lPercent) / 100;
				}
			}

			BuzzerCommands.DownloadMediaCommand {
				id: downloadCommand
				preview: true
				skipIfExists: true // (pkey_ !== "" || pkey_ !== undefined) ? false : true
				url: url_
				localFile: key_
				pkey: pkey_

				property int tryCount_: 0;

				onProgress: {
					//
					imageLoading.progress(pos, size);
				}

				onProcessed: {
					// tx, previewFile, originalFile, orientation
					// stop timer
					downloadWaitTimer.stop();
					// set file
					if (preview) path_ = "file://" + previewFile;
					else path_ = "file://" + originalFile;
					// set original orientation
					orientation_ = orientation;
					// stop spinning
					imageLoading.visible = false;
					// reset height
					mediaList.height = Math.max(buzzitemmedia_.calculatedHeight, mediaImage.height);
					buzzitemmedia_.calculatedHeight = mediaList.height;
				}

				onError: {
					if (code == "E_MEDIA_HEADER_NOT_FOUND") {
						//
						tryCount_++;

						if (tryCount_ < 15) {
							downloadTimer.start();
						} else {
							imageLoading.visible = false;
							mediaImage.source = "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "blur.one")
							// reset height
							mediaList.height = Math.max(buzzitemmedia_.calculatedHeight, mediaImage.height);
							buzzitemmedia_.calculatedHeight = mediaList.height;
						}
					}
				}
			}

			Timer {
				id: downloadTimer
				interval: 2000
				repeat: false
				running: false

				onTriggered: {
					downloadCommand.process();
				}
			}

			Timer {
				id: downloadWaitTimer
				interval: 1000
				repeat: false
				running: false

				onTriggered: {
					imageLoading.visible = true;
				}
			}

			Component.onCompleted: {
				downloadWaitTimer.start();
				downloadCommand.process();
			}
		}

		function addMedia(url, file) {
			mediaModel.append({
				key_: file,
				url_: url,
				path_: "",
				orientation_: 0,
				loaded_: false });
		}

		function prepare() {
			for (var lIdx = 0; lIdx < buzzMedia_.length; lIdx++) {
				var lMedia = buzzMedia_[lIdx];
				addMedia(lMedia.url, buzzerClient.getTempFilesPath() + "/" + lMedia.tx);
			}
		}
	}

	PageIndicator {
		id: mediaIndicator
		count: buzzMedia_ ? buzzMedia_.length : 0
		currentIndex: mediaList.currentIndex

		x: calculatedWidthInternal / 2 - width / 2
		y: spaceStats_ - height
		visible: buzzMedia_ ? buzzMedia_.length > 1 : false

		Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
		Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
		Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
		Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
	}
}
