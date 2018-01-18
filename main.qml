import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import QtQuick.Dialogs 1.1
import Qt.labs.settings 1.0
import org.qtproject.example 1.0
import QtQml 2.2

ApplicationWindow {
    id: window
    visible: true
    width: 640
    height: 480
    title: qsTr("Elf Explorer by:hanrai@gmail.com")
    font.family: "Arial"

    header: ToolBar {
        RowLayout {
            spacing: 20
            anchors.fill: parent

            ToolButton {
                contentItem: Image {
                    fillMode: Image.Pad
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    source: "qrc:/img/drawer.png"
                }
                onClicked: drawer.open()
            }

            Label {
                id: titleLabel
                font.pixelSize: 20
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

            ToolButton {
                contentItem: Image {
                    fillMode: Image.Pad
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    source: "qrc:/img/menu.png"
                }
                onClicked: optionsMenu.open()

                Menu {
                    id: optionsMenu
                    x: parent.width - width
                    transformOrigin: Menu.TopRight

                    MenuItem {
                        text: qsTr("打开文件...")
                        onTriggered: fileDialog.open()
                    }
                    MenuItem {
                        text: qsTr("保存文件...")
                        onTriggered: saveDialog.open()
                    }
                }
            }
        }
    }

    MessageDialog {
        id: messageDialog
        title: "文件系统错"
        text: "It's so cool that you are using Qt Quick."
    }
    Connections {
        target: fileManager
        onFileSystemError: {
            messageDialog.text = reason
            messageDialog.open()
        }
    }

    Flickable {
        id: flick

        anchors.fill: parent
        contentWidth: edit.paintedWidth
        contentHeight: edit.paintedHeight
        clip: true

        TextArea.flickable: TextArea {
            id: edit
            width: flick.width
            height: flick.height
            focus: true
            text: fileManager.mesScript
            font.family: "微软雅黑"
            font.pointSize: 16
            wrapMode: TextEdit.Wrap
        }
        ScrollBar.vertical: ScrollBar { }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("打开Arc文件")
        nameFilters: [ qsTr("Arc文件 (*.arc)")]
        onAccepted: {
            fileManager.openArc(fileDialog.fileUrl)
        }
    }
    FileDialog {
        id: saveDialog
        selectExisting: false
        title: qsTr("保存mes文件")
        nameFilters: [ qsTr("mes文件 (*.txt)")]
        onAccepted: {
            fileManager.saveMes(saveDialog.fileUrl)
        }
    }

    Drawer {
        id: drawer
        width: Math.min(window.width, window.height) / 3 * 2
        height: window.height

        ColumnLayout {
            spacing: 20
            anchors.fill: parent
            RowLayout {
                spacing: 20
                Layout.fillWidth: true
                height: 40

                Button {
                    text: "A"
                    onClicked: proxyModel.sortOrder = 0
                }

                Label {
                    id: arcFilenameLabel
                    font.pixelSize: 20
                    text: fileManager.arcFilename
                    elide: Label.ElideRight
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                }

                Button {
                    text: "V"
                    onClicked: proxyModel.sortOrder = 1
                }

            }

            TextArea {
                id: searchBox

                placeholderText: "Search..."
                inputMethodHints: Qt.ImhNoPredictiveText

                Layout.fillWidth: true
            }

            ListView {
                id: filenames
                currentIndex: -1
                Layout.fillHeight: true
                Layout.fillWidth: true

                delegate: ItemDelegate {
                    width: parent.width
                    text: model.name
                    highlighted: ListView.isCurrentItem
                    onClicked: {
                        filenames.currentIndex = index
                        titleLabel.text = model.name
                        fileManager.decodeFile(model.name, model.offset, model.size)
                        drawer.close()
                    }
                }
                ScrollBar.vertical: ScrollBar { }

                model: SortFilterProxyModel {
                    id: proxyModel
                    source: arcModel
                    sortCaseSensitivity: Qt.CaseInsensitive

                    filterString: "*" + searchBox.text + "*"
                    filterSyntax: SortFilterProxyModel.Wildcard
                    filterCaseSensitivity: Qt.CaseInsensitive
                }

                ScrollIndicator.vertical: ScrollIndicator { }
            }
        }
    }

    Settings {
        property alias x: window.x
        property alias y: window.y
        property alias width: window.width
        property alias height: window.height
        property alias folder: fileDialog.folder
        property alias saveFolder: saveDialog.folder
    }
}
