import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import Qt.labs.folderlistmodel 2.12
import QtMultimedia 5.8

//
// Picture choose dialog
//
Dialog
{
    id: pictureDialog
    modal: true
    focus: true
    title: "Choose picture"

    x: 5
    y: 10
    width: window.width - 30
    height: window.height - 100

    standardButtons: Dialog.Ok | Dialog.Cancel

    property string fileURL;
    property variant caller;

    function selectPicture(parent)
    {
        pictureView.currentIndex = -1;

        caller = parent;
        pictureDialog.open();
    }

    onAccepted:
    {
        caller.acceptSelection(fileURL);
		if (pictureView.prevCheckImage !== null) pictureView.prevCheckImage.checked = false;
        pictureDialog.close();
    }
    onRejected:
    {
		if (pictureView.prevCheckImage !== null) pictureView.prevCheckImage.checked = false;
        pictureDialog.close();
    }

    FolderListModel
    {
        id: pictureListModel
        nameFilters: [ "*.jpg", "*.png" ]
        showDirs: false

        Component.onCompleted:
        {
			console.log(buzzerApp.picturesLocation);
			pictureListModel.folder = buzzerApp.picturesLocation[0]; // but there are some directories may be...

        }
    }

    Rectangle
    {
        clip: true
        anchors.fill: parent

        ListView
        {
            id: pictureView
            anchors.fill: parent

            focus: true
            currentIndex: -1

            model: pictureListModel
            property variant prevCheckImage;

            delegate: ItemDelegate
            {
                id: pictureDelegate

                contentItem: Row
                {
                    Rectangle
                    {
                        id: pictureRow
                        width: pictureView.width - 10
                        height: 150
                        color: "transparent"

                        Image
                        {
                            id: pictureImage
                            height: 150
                            width: 150
                            fillMode: Image.PreserveAspectFit
                            source: fileURL
                        }
                        CheckBox
                        {
                            id: checkImage
                            x: pictureImage.width + 5
                            y: pictureImage.height/2 - checkImage.height / 2

                            onClicked:
                            {
                                if(checkImage.checked)
                                {
									if (pictureView.prevCheckImage !== null) pictureView.prevCheckImage.checked = false;
                                    pictureView.prevCheckImage = checkImage;
                                    checkImage.checked = true; // recheck

                                    //console.log(fileURL);
                                    pictureView.currentIndex = index;
                                    pictureDialog.fileURL = fileURL;
                                }
                            }
                        }
                    }
                }
            }

            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }
}
