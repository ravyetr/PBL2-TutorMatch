import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Rectangle {
    id: adminMatchesView
    color: Theme.cardBg
    radius: 10
    border.color: Theme.cardBorder
    border.width: 1

    property int activeFilterIndex: 0
    property var matchesModel: []
    property int pendingTerminateId: 0
    property string pendingTerminateLabel: ""

    function reloadMatches() {
        if (typeof guiBridge !== "undefined") {
            matchesModel = guiBridge.qmlGetAllMatches()
        }
    }

    function filteredMatches() {
        var filterStatuses = ["ACCEPTED", "COMPLETED", "CANCELLED"]
        var targetStatus = filterStatuses[activeFilterIndex] || "ACCEPTED"
        var list = []
        for (var i = 0; i < matchesModel.length; i++) {
            var m = matchesModel[i]
            if (m.status === targetStatus) {
                list.push(m)
            }
        }
        return list
    }

    Component.onCompleted: {
        reloadMatches()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onDataChanged() { reloadMatches() }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 16

            // 1. Title Header & Reload Button
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    spacing: 4
                    Text {
                        text: "Giám sát Kết nối & Lớp học"
                        font.bold: true
                        font.pixelSize: 18
                        color: Theme.textDark
                    }
                    Text {
                        text: "Theo dõi các quan hệ học tập đang diễn ra trên nền tảng"
                        font.pixelSize: 12
                        color: Theme.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "Hiển thị: " + adminMatchesView.filteredMatches().length + " lớp học"
                    font.bold: true
                    font.pixelSize: 13
                    color: Theme.textDark
                }

                Button {
                    text: "Làm mới"
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        font.pixelSize: 12
                        color: Theme.textLight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 100
                        implicitHeight: 34
                        radius: 6
                        color: parent.hovered ? Theme.cSilverLakeBlue : Theme.sidebarBg
                    }
                    onClicked: reloadMatches()
                }
            }

            // Status Banner Notification
            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: 6
                color: Theme.bgApp
                border.color: Theme.accentBlue
                border.width: 1
                visible: (typeof guiBridge !== "undefined" && guiBridge.statusMessage !== "")

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    Text {
                        text: "Thông báo: " + ((typeof guiBridge !== "undefined") ? guiBridge.statusMessage : "")
                        font.pixelSize: 12
                        font.bold: true
                        color: Theme.textDark
                    }
                }
            }

            // 2. Segmented Filter Bar (3 options)
            Rectangle {
                Layout.fillWidth: true
                height: 48
                radius: 8
                color: Theme.bgApp
                border.color: Theme.cardBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 6

                    Repeater {
                        model: ["Đang kết nối", "Đã hoàn thành", "Đã hủy"]

                        SegmentedFilterButton {
                            text: modelData
                            active: index === adminMatchesView.activeFilterIndex
                            onClicked: adminMatchesView.activeFilterIndex = index
                        }
                    }
                }
            }

            // 3. Match Cards List
            ListView {
                id: matchesListView
                Layout.fillWidth: true
                implicitHeight: Math.max(300, count * 86)
                clip: true
                spacing: 10
                model: adminMatchesView.filteredMatches()

                delegate: Rectangle {
                    width: matchesListView.width
                    height: 80
                    color: Theme.cardBg
                    radius: 8
                    border.color: Theme.cardBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 14

                        // Icon Indicator
                        Rectangle {
                            width: 40
                            height: 40
                            radius: 20
                            color: (modelData.status === "ACCEPTED") ? "#e6f7ff" :
                                   (modelData.status === "COMPLETED") ? "#f6ffed" : "#fff1f0"

                            Text {
                                anchors.centerIn: parent
                                text: (modelData.status === "ACCEPTED") ? "🔗" :
                                      (modelData.status === "COMPLETED") ? "✓" : "✕"
                                font.pixelSize: 16
                            }
                        }

                        // Participant Info (Student <-> Tutor & Subject)
                        ColumnLayout {
                            Layout.preferredWidth: 240
                            spacing: 2

                            Text {
                                text: modelData.studentName + " ↔ " + modelData.tutorName
                                font.bold: true
                                font.pixelSize: 13
                                color: Theme.textDark
                                elide: Text.ElideRight
                            }

                            Text {
                                text: "Môn học: " + modelData.subject + " · " + (modelData.hourlyRate ? modelData.hourlyRate.toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ/buổi" : "")
                                font.pixelSize: 11
                                color: Theme.textMuted
                                elide: Text.ElideRight
                            }

                            Text {
                                text: modelData.isCourse ?
                                      ("Ngày bắt đầu: " + modelData.startDate + " (" + modelData.weeksCount + " tuần) · Tạo ngày: " + modelData.createdAt) :
                                      ("Ngày tạo: " + modelData.createdAt)
                                font.pixelSize: 10
                                color: Theme.textMuted
                            }
                        }

                        // Schedule Time Slot
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: "Lịch học cố định:"
                                font.pixelSize: 11
                                color: Theme.textMuted
                            }
                            Text {
                                text: modelData.slotText || modelData.slotLabel || ""
                                font.bold: true
                                font.pixelSize: 12
                                color: Theme.textDark
                                elide: Text.ElideRight
                            }
                        }

                        // Status Badge
                        Rectangle {
                            implicitWidth: statusTextElem.implicitWidth + 16
                            height: 26
                            radius: 4
                            color: (modelData.status === "ACCEPTED") ? "#e6f7ff" :
                                   (modelData.status === "COMPLETED") ? "#f6ffed" : "#fff1f0"
                            border.color: (modelData.status === "ACCEPTED") ? "#91d5ff" :
                                          (modelData.status === "COMPLETED") ? "#b7eb8f" : "#ffa39e"
                            border.width: 1

                            Text {
                                id: statusTextElem
                                anchors.centerIn: parent
                                text: modelData.statusText || modelData.status
                                font.pixelSize: 10
                                font.bold: true
                                color: (modelData.status === "ACCEPTED") ? "#096dd9" :
                                       (modelData.status === "COMPLETED") ? "#389e0d" : "#cf1322"
                            }
                        }

                        // Action Button: [Đóng kết nối] (Only for ACCEPTED)
                        Button {
                            visible: (modelData.status === "ACCEPTED")
                            text: "Đóng kết nối"
                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 11
                                font.bold: true
                                color: "#cf1322"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                implicitWidth: 100
                                implicitHeight: 30
                                radius: 4
                                color: parent.hovered ? "#ffa39e" : "#fff1f0"
                                border.color: "#ffa39e"
                                border.width: 1
                            }
                            onClicked: {
                                adminMatchesView.pendingTerminateId = modelData.id
                                adminMatchesView.pendingTerminateLabel = modelData.studentName + " ↔ " + modelData.tutorName + " (" + modelData.subject + ")"
                                confirmModal.open()
                            }
                        }
                    }
                }

                // Empty State Placeholder
                Rectangle {
                    anchors.centerIn: parent
                    visible: adminMatchesView.filteredMatches().length === 0
                    color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "Không có lớp học nào ở trạng thái này"
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }
                }
            }
        }
    }

    // Confirmation Modal Dialog
    Dialog {
        id: confirmModal
        anchors.centerIn: parent
        modal: true
        focus: true
        title: "Xác nhận đóng kết nối lớp học"
        standardButtons: Dialog.No | Dialog.Yes

        background: Rectangle {
            implicitWidth: 420
            implicitHeight: 180
            color: Theme.cardBg
            radius: 10
            border.color: Theme.cardBorder
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Text {
                text: "XÁC NHẬN ĐÓNG KẾT NỐI LỚP HỌC"
                font.bold: true
                font.pixelSize: 15
                color: Theme.textDark
            }

            Text {
                text: "Bạn có chắc chắn muốn đóng kết nối:\n" + adminMatchesView.pendingTerminateLabel + "?\n\nTrạng thái lớp học sẽ chuyển thành ĐÃ HỦY và thời khóa biểu của cả 2 bên sẽ được giải phóng."
                font.pixelSize: 12
                color: Theme.textDark
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10

                Button {
                    text: "Hủy bỏ"
                    onClicked: confirmModal.close()
                }

                Button {
                    text: "Xác nhận đóng"
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        color: "#FFFFFF"
                    }
                    background: Rectangle {
                        color: "#cf1322"
                        radius: 4
                    }
                    onClicked: {
                        confirmModal.close()
                        if (typeof guiBridge !== "undefined") {
                            guiBridge.qmlAdminTerminateMatch(adminMatchesView.pendingTerminateId)
                            adminMatchesView.reloadMatches()
                        }
                    }
                }
            }
        }
    }
}
