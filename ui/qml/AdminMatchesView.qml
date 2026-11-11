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
            if (activeFilterIndex === 2) {
                if (m.status === "CANCELLED" || m.status === "REJECTED" || m.status === "EARLY_TERMINATED") {
                    list.push(m)
                }
            } else if (m.status === targetStatus) {
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
            ColumnLayout {
                id: matchesRowsCol
                Layout.fillWidth: true
                spacing: 10

                Repeater {
                    id: matchesRepeater
                    model: adminMatchesView.filteredMatches()

                    Rectangle {
                        id: rowCard
                        Layout.fillWidth: true
                        implicitHeight: 84
                        color: Theme.cardBg
                        radius: 8
                        border.color: Theme.cardBorder
                        border.width: 1
                        clip: true

                        RowLayout {
                            id: rowLayout
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 16

                            // 1. Icon Indicator
                            Rectangle {
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                radius: 20
                                color: (modelData.status === "ACCEPTED") ? "#e6f7ff" :
                                       (modelData.status === "COMPLETED") ? "#f6ffed" :
                                       (modelData.status === "PENDING") ? "#fffbe6" :
                                       (modelData.status === "EARLY_TERMINATED") ? "#fff7e6" : "#fff1f0"
                                border.color: (modelData.status === "ACCEPTED") ? "#91d5ff" :
                                              (modelData.status === "COMPLETED") ? "#b7eb8f" :
                                              (modelData.status === "PENDING") ? "#ffe58f" :
                                              (modelData.status === "EARLY_TERMINATED") ? "#ffa940" : "#ffa39e"
                                border.width: 1
                                Layout.alignment: Qt.AlignVCenter

                                Text {
                                    anchors.centerIn: parent
                                    text: (modelData.status === "ACCEPTED") ? "🔗" :
                                          (modelData.status === "COMPLETED") ? "✓" :
                                          (modelData.status === "PENDING") ? "⏳" :
                                          (modelData.status === "EARLY_TERMINATED") ? "⛔" : "✕"
                                    font.pixelSize: 16
                                }
                            }

                            // 2. Region A: Student ↔ Tutor Identity & Course Subject
                            ColumnLayout {
                                id: regionA
                                Layout.fillWidth: true
                                Layout.minimumWidth: 230
                                Layout.preferredWidth: 320
                                Layout.maximumWidth: 420
                                spacing: 3
                                Layout.alignment: Qt.AlignVCenter

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: (modelData.studentName || "Học viên") + " ↔ " + (modelData.tutorName || "Gia sư")
                                    font.bold: true
                                    font.pixelSize: 13
                                    color: Theme.textDark
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: "📚 " + (modelData.subject || "Khóa học") + (modelData.hourlyRate ? (" · " + modelData.hourlyRate.toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " đ/buổi") : "")
                                    font.pixelSize: 11
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: "📅 Tạo ngày: " + (modelData.createdAt || "N/A")
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }

                            // 3. Region B: Schedule Information
                            ColumnLayout {
                                id: regionB
                                Layout.fillWidth: true
                                Layout.minimumWidth: 200
                                Layout.preferredWidth: 260
                                spacing: 3
                                Layout.alignment: Qt.AlignVCenter

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    spacing: 5

                                    Text {
                                        text: "⏰"
                                        font.pixelSize: 11
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: modelData.slotText || modelData.slotLabel || "Chưa cập nhật ca học"
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: Theme.textDark
                                        elide: Text.ElideRight
                                        maximumLineCount: 1
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    spacing: 5

                                    Text {
                                        text: "🗓️"
                                        font.pixelSize: 10
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: modelData.isCourse ?
                                              ("Khóa " + (modelData.weeksCount || 4) + " tuần · Bắt đầu: " + (modelData.startDate || "N/A")) :
                                              ("Bắt đầu: " + (modelData.startDate || modelData.createdAt || "N/A"))
                                        font.pixelSize: 11
                                        color: Theme.textMuted
                                        elide: Text.ElideRight
                                        maximumLineCount: 1
                                    }
                                }
                            }

                            // 4. Region C: Status Badge & Action Controls
                            RowLayout {
                                id: regionC
                                Layout.preferredWidth: 220
                                Layout.minimumWidth: 120
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                spacing: 8

                                Item { Layout.fillWidth: true } // Pushes status and button to the right

                                Rectangle {
                                    id: statusBadge
                                    implicitWidth: Math.max(92, statusLabel.implicitWidth + 18)
                                    height: 26
                                    radius: 4
                                    color: (modelData.status === "ACCEPTED") ? "#e6f7ff" :
                                           (modelData.status === "COMPLETED") ? "#f6ffed" :
                                           (modelData.status === "PENDING") ? "#fffbe6" :
                                           (modelData.status === "EARLY_TERMINATED") ? "#fff7e6" : "#fff1f0"
                                    border.color: (modelData.status === "ACCEPTED") ? "#91d5ff" :
                                                  (modelData.status === "COMPLETED") ? "#b7eb8f" :
                                                  (modelData.status === "PENDING") ? "#ffe58f" :
                                                  (modelData.status === "EARLY_TERMINATED") ? "#ffa940" : "#ffa39e"
                                    border.width: 1

                                    Text {
                                        id: statusLabel
                                        anchors.centerIn: parent
                                        text: (modelData.status === "EARLY_TERMINATED") ? "KẾT THÚC SỚM" :
                                              (modelData.status === "ACCEPTED") ? "ĐANG HỌC" :
                                              (modelData.statusText || modelData.status)
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: (modelData.status === "ACCEPTED") ? "#096dd9" :
                                               (modelData.status === "COMPLETED") ? "#389e0d" :
                                               (modelData.status === "PENDING") ? "#d48806" :
                                               (modelData.status === "EARLY_TERMINATED") ? "#d46b08" : "#cf1322"
                                    }
                                }

                                Button {
                                    id: terminateBtn
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
                                        implicitWidth: 95
                                        implicitHeight: 28
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
                    }
                }

                // Empty State Placeholder
                Rectangle {
                    Layout.fillWidth: true
                    height: 180
                    visible: adminMatchesView.filteredMatches().length === 0
                    radius: 8
                    color: Theme.bgApp
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        Text {
                            text: "🔗 Chưa có lớp học nào ở trạng thái này"
                            font.bold: true
                            font.pixelSize: 15
                            color: Theme.textDark
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: "Các kết nối lớp học sẽ tự động hiển thị tại đây khi người dùng đặt lịch và được duyệt."
                            font.pixelSize: 12
                            color: Theme.textMuted
                            Layout.alignment: Qt.AlignHCenter
                        }
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
