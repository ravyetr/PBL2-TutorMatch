import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Item {
    id: tutorBookingsRoot

    property var allBookingsList: []
    property int activeFilterIndex: 0 // 0: All, 1: Pending, 2: Accepted, 3: Rejected/Cancelled/Terminated
    property var selectedBookingForEarlyTerm: null
    property var selectedBookingForAccept: null
    property bool tutorAckChecked: false

    function reloadData() {
        if (typeof guiBridge !== "undefined") {
            allBookingsList = guiBridge.qmlGetTutorBookings() || []
        }
    }

    Component.onCompleted: {
        console.log("[FORENSIC] TutorBookings loaded | activeFilterIndex=" + activeFilterIndex)
        reloadData()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onDataChanged() { reloadData() }
        function onTutorScheduleChanged() { reloadData() }
    }

    function filteredBookings() {
        if (activeFilterIndex === 0) return allBookingsList
        var res = []
        for (var i = 0; i < allBookingsList.length; i++) {
            var st = allBookingsList[i].status
            if (activeFilterIndex === 1 && st === "PENDING") res.push(allBookingsList[i])
            else if (activeFilterIndex === 2 && (st === "ACCEPTED" || st === "COMPLETED")) res.push(allBookingsList[i])
            else if (activeFilterIndex === 3 && (st === "REJECTED" || st === "CANCELLED" || st === "EARLY_TERMINATED")) res.push(allBookingsList[i])
        }
        return res
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 16

            // Header Title
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 4
                    Text { text: "YÊU CẦU ĐĂNG KÝ CỦA HỌC VIÊN"; font.bold: true; font.pixelSize: 18; color: Theme.textDark }
                    Text { text: "Quản lý các yêu cầu đăng ký lịch học trực tuyến từ học viên"; font.pixelSize: 12; color: Theme.textMuted }
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "Tổng số: " + tutorBookingsRoot.filteredBookings().length + " yêu cầu"
                    font.bold: true
                    font.pixelSize: 13
                    color: "#d97706"
                }
            }

            // Notification Banner
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
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    Text {
                        text: "Thông báo: " + ((typeof guiBridge !== "undefined") ? guiBridge.statusMessage : "")
                        font.pixelSize: 12
                        font.bold: true
                        color: Theme.textDark
                    }
                }
            }

            // Filter Tabs Bar
            Rectangle {
                Layout.fillWidth: true
                height: 50
                radius: 10
                color: Theme.cardBg
                border.color: Theme.cardBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 8

                    Repeater {
                        model: ["Tất cả đơn", "Chờ duyệt", "Đã nhận", "Đã từ chối / Hủy"]

                        SegmentedFilterButton {
                            text: modelData
                            active: index === tutorBookingsRoot.activeFilterIndex
                            inactiveBgColor: "#FFFFFF"
                            inactiveTextColor: Theme.textDark
                            inactiveBorderColor: Theme.cardBorder
                            hoverBgColor: Theme.accentBlue
                            hoverTextColor: Theme.textDark
                            activeBgColor: Theme.sidebarBg
                            activeTextColor: Theme.textLight
                            activeBorderColor: Theme.sidebarBg
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            onClicked: tutorBookingsRoot.activeFilterIndex = index
                        }
                    }
                }
            }

            // Empty State
            Rectangle {
                Layout.fillWidth: true
                height: 180
                radius: 10
                color: Theme.cardBg
                border.color: Theme.cardBorder
                visible: tutorBookingsRoot.filteredBookings().length === 0

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12
                    Text { text: "📩 Chưa có yêu cầu học viên mới"; font.bold: true; font.pixelSize: 18; color: Theme.textDark; Layout.alignment: Qt.AlignHCenter }
                    Text { text: "Không tìm thấy yêu cầu đăng ký nào trong mục lọc này."; font.pixelSize: 13; color: Theme.textMuted; Layout.alignment: Qt.AlignHCenter }
                }
            }

            // Bookings Repeater List
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12
                visible: tutorBookingsRoot.filteredBookings().length > 0

                Repeater {
                    model: tutorBookingsRoot.filteredBookings()

                    Rectangle {
                        Layout.fillWidth: true
                        height: 95
                        radius: 10
                        color: Theme.cardBg
                        border.color: Theme.cardBorder

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 16

                            // Student Avatar Initials
                            Rectangle {
                                width: 44
                                height: 44
                                radius: 22
                                color: Theme.sidebarBg
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.studentInitials || "H"
                                    font.bold: true
                                    font.pixelSize: 18
                                    color: Theme.textLight
                                }
                            }

                            ColumnLayout {
                                spacing: 4
                                RowLayout {
                                    spacing: 8
                                    Text { text: modelData.studentName; font.bold: true; font.pixelSize: 15; color: Theme.textDark }

                                    // Status Badge
                                    Rectangle {
                                        height: 22
                                        implicitWidth: statusText.implicitWidth + 12
                                        radius: 4
                                        color: (modelData.status === "PENDING") ? "#fffbe6" :
                                               (modelData.status === "ACCEPTED" || modelData.status === "COMPLETED") ? "#f6ffed" :
                                               (modelData.status === "EARLY_TERMINATED") ? "#fff7e6" : "#fff1f0"
                                        border.color: (modelData.status === "PENDING") ? "#ffe58f" :
                                                      (modelData.status === "ACCEPTED" || modelData.status === "COMPLETED") ? "#b7eb8f" :
                                                      (modelData.status === "EARLY_TERMINATED") ? "#ffa940" : "#ffa39e"

                                        Text {
                                            id: statusText
                                            anchors.centerIn: parent
                                            text: (modelData.status === "PENDING") ? "CHỜ DUYỆT" :
                                                  (modelData.status === "ACCEPTED") ? "ĐÃ CHẤP NHẬN" :
                                                  (modelData.status === "COMPLETED") ? "HOÀN THÀNH" :
                                                  (modelData.status === "EARLY_TERMINATED") ? "KẾT THÚC SỚM" :
                                                  (modelData.status === "REJECTED") ? "ĐÃ TỪ CHỐI" : "ĐÃ HỦY"
                                            font.bold: true
                                            font.pixelSize: 10
                                            color: (modelData.status === "PENDING") ? "#d97706" :
                                                   (modelData.status === "ACCEPTED" || modelData.status === "COMPLETED") ? "#52c41a" :
                                                   (modelData.status === "EARLY_TERMINATED") ? "#d46b08" : "#f5222d"
                                        }
                                    }
                                }

                                Text {
                                    text: "Môn: " + modelData.subject + " · Thời gian: " + modelData.slotLabel
                                    font.pixelSize: 12
                                    color: Theme.textMuted
                                }

                                Text {
                                    visible: modelData.isCourse === true
                                    text: "🗓️ Thời gian khóa học: " + (modelData.dateRangeLabel || "") + " · Tổng: " + ((modelData.totalCost || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ")
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }

                                Text {
                                    text: modelData.isCourse ?
                                          ("Học phí: " + modelData.hourlyRate.toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ/buổi · Ngày gửi đơn: " + (modelData.createdAt || "N/A")) :
                                          ("Học phí: " + modelData.hourlyRate.toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ/buổi · Ngày đăng ký: " + (modelData.createdAt || "N/A"))
                                    font.pixelSize: 11
                                    color: "#d97706"
                                }
                            }

                            Item { Layout.fillWidth: true }

                            // Actions
                            RowLayout {
                                spacing: 8
                                visible: modelData.status === "PENDING"

                                Button {
                                    text: "Chấp nhận"
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: Theme.btnText
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        implicitWidth: 100
                                        implicitHeight: 34
                                        radius: 6
                                        color: parent.hovered ? "#52c41a" : "#389e0d"
                                    }
                                    onClicked: {
                                        tutorBookingsRoot.selectedBookingForAccept = modelData
                                        tutorBookingsRoot.tutorAckChecked = false
                                        tutorAcceptCommitmentModal.visible = true
                                    }
                                }

                                Button {
                                    text: "Từ chối"
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: "#ffffff"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        implicitWidth: 90
                                        implicitHeight: 34
                                        radius: 6
                                        color: parent.hovered ? "#ff4d4f" : "#cf1322"
                                    }
                                    onClicked: {
                                        if (typeof guiBridge !== "undefined") {
                                            guiBridge.qmlRejectBooking(modelData.id)
                                        }
                                    }
                                }
                            }

                            // Consensual Early Termination Actions for Tutor
                            RowLayout {
                                spacing: 8
                                visible: modelData.status === "ACCEPTED"

                                Button {
                                    visible: modelData.canRequestEarlyTermination === true
                                    text: "Đề nghị kết thúc sớm"
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: "#d46b08"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        implicitWidth: 140
                                        implicitHeight: 34
                                        radius: 6
                                        color: parent.hovered ? "#fff7e6" : Theme.bgApp
                                        border.color: "#ffa940"
                                        border.width: 1
                                    }
                                    onClicked: {
                                        tutorBookingsRoot.selectedBookingForEarlyTerm = modelData
                                        tutorEarlyTermModal.visible = true
                                    }
                                }

                                Rectangle {
                                    visible: modelData.isPendingEarlyTerminationFromMe === true
                                    implicitWidth: 200
                                    implicitHeight: 34
                                    radius: 6
                                    color: "#e6f7ff"
                                    border.color: "#91d5ff"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: "⏳ Đã gửi đề nghị kết thúc sớm"
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: "#096dd9"
                                    }
                                }

                                RowLayout {
                                    visible: modelData.canRespondEarlyTermination === true
                                    spacing: 6

                                    Button {
                                        text: "Đồng ý kết thúc"
                                        contentItem: Text { text: parent.text; font.bold: true; font.pixelSize: 11; color: "#FFFFFF"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        background: Rectangle { implicitWidth: 110; implicitHeight: 34; radius: 6; color: "#d46b08" }
                                        onClicked: {
                                            if (typeof guiBridge !== "undefined") {
                                                guiBridge.respondEarlyTermination(modelData.id, true)
                                                tutorBookingsRoot.reloadData()
                                            }
                                        }
                                    }

                                    Button {
                                        text: "Từ chối"
                                        contentItem: Text { text: parent.text; font.bold: true; font.pixelSize: 11; color: Theme.textDark; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        background: Rectangle { implicitWidth: 80; implicitHeight: 34; radius: 6; color: Theme.bgApp; border.color: Theme.cardBorder; border.width: 1 }
                                        onClicked: {
                                            if (typeof guiBridge !== "undefined") {
                                                guiBridge.respondEarlyTermination(modelData.id, false)
                                                tutorBookingsRoot.reloadData()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Tutor Early Termination Request Modal
    Item {
        id: tutorEarlyTermModal
        anchors.fill: parent
        visible: false
        z: 999

        Rectangle {
            anchors.fill: parent
            color: "#80000000"
            MouseArea { anchors.fill: parent; onClicked: {} }
        }

        Rectangle {
            width: 480
            height: 280
            anchors.centerIn: parent
            radius: 10
            color: Theme.cardBg
            border.color: Theme.cardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12

                Text {
                    text: "ĐỀ NGHỊ KẾT THÚC KHÓA HỌC SỚM"
                    font.bold: true
                    font.pixelSize: 15
                    color: Theme.textDark
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: "Khóa học sẽ kết thúc sau khi học viên đồng thuận. Các buổi học trong tương lai sẽ được giải phóng."
                    font.pixelSize: 11
                    color: Theme.textMuted
                }

                TextField {
                    id: tutorReasonInput
                    Layout.fillWidth: true
                    placeholderText: "Nhập lý do kết thúc sớm (ví dụ: học viên đã hoàn thành mục tiêu học tập)..."
                    color: Theme.textDark
                    background: Rectangle {
                        color: Theme.bgApp
                        radius: 6
                        border.color: tutorReasonInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: "Quay lại"
                        Layout.fillWidth: true
                        height: 38
                        contentItem: Text { text: parent.text; font.bold: true; color: Theme.textDark; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: Theme.accentBlue; radius: 6; border.color: Theme.cardBorder }
                        onClicked: {
                            tutorEarlyTermModal.visible = false
                            tutorReasonInput.text = ""
                        }
                    }

                    Button {
                        text: "Gửi đề nghị"
                        Layout.fillWidth: true
                        height: 38
                        contentItem: Text { text: parent.text; font.bold: true; color: "#FFFFFF"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: "#d46b08"; radius: 6 }
                        onClicked: {
                            if (tutorBookingsRoot.selectedBookingForEarlyTerm && typeof guiBridge !== "undefined") {
                                guiBridge.requestEarlyTermination(tutorBookingsRoot.selectedBookingForEarlyTerm.id, tutorReasonInput.text)
                                tutorEarlyTermModal.visible = false
                                tutorReasonInput.text = ""
                                tutorBookingsRoot.reloadData()
                            } else {
                                tutorEarlyTermModal.visible = false
                            }
                        }
                    }
                }
            }
        }
    }

    // Tutor Commitment Acknowledgment Modal Before Acceptance
    Item {
        id: tutorAcceptCommitmentModal
        anchors.fill: parent
        visible: false
        z: 999

        Rectangle {
            anchors.fill: parent
            color: "#80000000"
            MouseArea { anchors.fill: parent; onClicked: {} }
        }

        Rectangle {
            width: 520
            implicitHeight: contentCol.implicitHeight + 40
            anchors.centerIn: parent
            radius: 10
            color: Theme.cardBg
            border.color: Theme.cardBorder
            border.width: 1

            ColumnLayout {
                id: contentCol
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12

                Text {
                    text: "XÁC NHẬN CHẤP NHẬN YÊU CẦU KHÓA HỌC"
                    font.bold: true
                    font.pixelSize: 15
                    color: Theme.textDark
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: infoCol.implicitHeight + 16
                    color: Theme.bgApp
                    radius: 6
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        id: infoCol
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 4

                        Text {
                            text: "Học viên: " + (tutorBookingsRoot.selectedBookingForAccept ? tutorBookingsRoot.selectedBookingForAccept.studentName : "")
                            font.bold: true
                            font.pixelSize: 12
                            color: Theme.textDark
                        }
                        Text {
                            text: "Môn: " + (tutorBookingsRoot.selectedBookingForAccept ? tutorBookingsRoot.selectedBookingForAccept.subject : "") + " · " + (tutorBookingsRoot.selectedBookingForAccept ? tutorBookingsRoot.selectedBookingForAccept.slotLabel : "")
                            font.pixelSize: 11
                            color: Theme.textMuted
                        }
                        Text {
                            text: "Thời gian: " + (tutorBookingsRoot.selectedBookingForAccept ? (tutorBookingsRoot.selectedBookingForAccept.dateRangeLabel || "") : "") + " · Tổng: " + ((tutorBookingsRoot.selectedBookingForAccept ? (tutorBookingsRoot.selectedBookingForAccept.totalCost || 0) : 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ")
                            font.pixelSize: 11
                            font.bold: true
                            color: "#d97706"
                        }
                    }
                }

                Text {
                    text: "📜 ĐIỀU KHOẢN CAM KẾT KHÓA HỌC (" + (typeof guiBridge !== "undefined" ? guiBridge.getCommitmentVersion() : "COMMITMENT_V1") + "):"
                    font.bold: true
                    font.pixelSize: 11
                    color: Theme.textDark
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: policyText.implicitHeight + 16
                    color: "#f8fafc"
                    radius: 6
                    border.color: Theme.cardBorder
                    border.width: 1

                    Text {
                        id: policyText
                        anchors.fill: parent
                        anchors.margins: 8
                        text: (typeof guiBridge !== "undefined" ? guiBridge.getCommitmentPolicyText() : "Gia sư cam kết thực hiện đầy đủ các buổi dạy...")
                        font.pixelSize: 10
                        color: Theme.textMuted
                        wrapMode: Text.WordWrap
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    CheckBox {
                        id: tutorAckCheckbox
                        checked: tutorBookingsRoot.tutorAckChecked
                        onToggled: tutorBookingsRoot.tutorAckChecked = checked
                    }

                    Text {
                        text: "Tôi đã đọc, hiểu rõ và đồng ý với chính sách cam kết khóa học và điều khoản kết thúc sớm."
                        font.bold: true
                        font.pixelSize: 11
                        color: tutorBookingsRoot.tutorAckChecked ? "#1d4ed8" : Theme.textDark
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                tutorAckCheckbox.checked = !tutorAckCheckbox.checked;
                                tutorBookingsRoot.tutorAckChecked = tutorAckCheckbox.checked;
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: "Hủy bỏ"
                        Layout.fillWidth: true
                        height: 38
                        contentItem: Text { text: parent.text; font.bold: true; color: Theme.textDark; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: Theme.accentBlue; radius: 6; border.color: Theme.cardBorder }
                        onClicked: {
                            tutorAcceptCommitmentModal.visible = false
                            tutorBookingsRoot.selectedBookingForAccept = null
                            tutorBookingsRoot.tutorAckChecked = false
                        }
                    }

                    Button {
                        text: "Xác nhận chấp nhận"
                        Layout.fillWidth: true
                        height: 38
                        enabled: tutorBookingsRoot.tutorAckChecked
                        contentItem: Text {
                            text: parent.text
                            font.bold: true
                            color: parent.enabled ? "#FFFFFF" : Theme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.enabled ? (parent.hovered ? "#52c41a" : "#389e0d") : "#e0e0e0"
                            radius: 6
                        }
                        onClicked: {
                            if (tutorBookingsRoot.selectedBookingForAccept && typeof guiBridge !== "undefined") {
                                var ok = guiBridge.qmlAcceptBooking(tutorBookingsRoot.selectedBookingForAccept.id, true)
                                tutorAcceptCommitmentModal.visible = false
                                tutorBookingsRoot.selectedBookingForAccept = null
                                tutorBookingsRoot.tutorAckChecked = false
                                tutorBookingsRoot.reloadData()
                            } else {
                                tutorAcceptCommitmentModal.visible = false
                            }
                        }
                    }
                }
            }
        }
    }
}
