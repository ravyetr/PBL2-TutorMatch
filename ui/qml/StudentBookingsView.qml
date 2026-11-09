import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Item {
    id: studentBookingsView

    property var bookingsList: []
    property int activeFilterIndex: 0 // 0: Tất cả, 1: Đang chờ, 2: Đã kết nối, 3: Lịch sử
    property bool isLoading: false
    property var selectedBookingToCancel: null
    property var selectedBookingForEarlyTerm: null

    property var backendMetrics: ({
        acceptedCount: 0,
        pendingCount: 0,
        historyCount: 0,
        weeklyCost: 0.0
    })

    signal navigateToTabRequested(int tabIndex)

    function reloadBookings() {
        isLoading = true
        if (typeof guiBridge !== "undefined") {
            bookingsList = guiBridge.qmlGetStudentBookings() || []
            if (typeof guiBridge.qmlGetStudentBookingMetrics === "function") {
                var m = guiBridge.qmlGetStudentBookingMetrics(0)
                if (m && typeof m.acceptedCount !== "undefined") {
                    backendMetrics = m
                }
            }
        }
        isLoading = false
    }

    Component.onCompleted: {
        console.log("[FORENSIC] StudentBookingsView loaded | activeFilterIndex=" + activeFilterIndex)
        reloadBookings()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onStateChanged() { reloadBookings() }
        function onDataChanged() { reloadBookings() }
    }

    function getFilteredBookings() {
        if (!bookingsList || bookingsList.length === 0) return []
        if (activeFilterIndex === 1) { // 1: Đang học (ACCEPTED)
            return bookingsList.filter(function(b) { return b.status === "ACCEPTED" })
        } else if (activeFilterIndex === 2) { // 2: Chờ duyệt (PENDING)
            return bookingsList.filter(function(b) { return b.status === "PENDING" })
        } else if (activeFilterIndex === 3) { // 3: Đã kết thúc (COMPLETED, CANCELLED, REJECTED, EARLY_TERMINATED)
            return bookingsList.filter(function(b) {
                return b.status === "COMPLETED" || b.status === "CANCELLED" || b.status === "REJECTED" || b.status === "EARLY_TERMINATED"
            })
        }
        return bookingsList // 0: Tất cả
    }

    readonly property var metrics: backendMetrics

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        // Page Header Banner
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Layout.minimumWidth: 0

                Text {
                    text: "LỊCH HỌC & KẾT NỐI (BOOKING LIFECYCLE MANAGEMENT)"
                    font.bold: true
                    font.pixelSize: 18
                    color: Theme.textDark
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Text {
                    text: "Theo dõi toàn bộ tiến trình yêu cầu đăng ký, lớp học đang diễn ra và lịch sử kết nối của bạn"
                    font.pixelSize: 12
                    color: Theme.textMuted
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
            }

            Button {
                text: "Làm mới dữ liệu"
                focusPolicy: Qt.NoFocus
                contentItem: Text {
                    text: parent.text
                    font.bold: true
                    font.pixelSize: 11
                    color: parent.down ? Theme.textLight : (parent.hovered ? Theme.textDark : Theme.btnText)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 120
                    implicitHeight: 36
                    radius: 6
                    color: parent.down ? Theme.sidebarBg : (parent.hovered ? Theme.sidebarActive : Theme.btnPrimary)
                    border.color: Theme.cardBorder
                    border.width: 1
                }
                onClicked: studentBookingsView.reloadBookings()
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
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    elide: Text.ElideRight
                }
            }
        }

        // Top 3 Metric Cards Grid
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            // Metric Card 1: Accepted Classes
            Rectangle {
                Layout.fillWidth: true
                height: 72
                radius: 10
                color: Theme.cardBg
                border.color: Theme.cardBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    Rectangle {
                        width: 44
                        height: 44
                        radius: 22
                        color: "#132719"
                        border.color: "#52c41a"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "🎓"
                            font.pixelSize: 20
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Text {
                            text: "LỚP ĐANG HỌC"
                            font.bold: true
                            font.pixelSize: 11
                            color: Theme.textMuted
                        }
                        Text {
                            text: studentBookingsView.metrics.acceptedCount + " lớp kết nối"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#52c41a"
                        }
                    }
                }
            }

            // Metric Card 2: Pending Requests
            Rectangle {
                Layout.fillWidth: true
                height: 72
                radius: 10
                color: Theme.cardBg
                border.color: Theme.cardBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    Rectangle {
                        width: 44
                        height: 44
                        radius: 22
                        color: "#fffbe6"
                        border.color: "#f59e0b"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "⏳"
                            font.pixelSize: 20
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Text {
                            text: "YÊU CẦU CHỜ DUYỆT"
                            font.bold: true
                            font.pixelSize: 11
                            color: Theme.textMuted
                        }
                        Text {
                            text: studentBookingsView.metrics.pendingCount + " đơn đăng ký"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#d97706"
                        }
                    }
                }
            }

            // Metric Card 3: Weekly Budget / Cost
            Rectangle {
                Layout.fillWidth: true
                height: 72
                radius: 10
                color: Theme.cardBg
                border.color: Theme.cardBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    Rectangle {
                        width: 44
                        height: 44
                        radius: 22
                        color: Theme.sidebarBg
                        Text {
                            anchors.centerIn: parent
                            text: "💳"
                            font.pixelSize: 20
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Text {
                            text: "CHI PHÍ DỰ KIẾN / TUẦN"
                            font.bold: true
                            font.pixelSize: 11
                            color: Theme.textMuted
                        }
                        Text {
                            text: (studentBookingsView.metrics.weeklyCost || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ"
                            font.bold: true
                            font.pixelSize: 16
                            color: Theme.textDark
                        }
                    }
                }
            }
        }

        // Segmented Filter Control Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Repeater {
                model: ["Tất cả (" + (studentBookingsView.bookingsList ? studentBookingsView.bookingsList.length : 0) + ")",
                        "Đang học (" + (studentBookingsView.metrics ? studentBookingsView.metrics.acceptedCount : 0) + ")",
                        "Chờ duyệt (" + (studentBookingsView.metrics ? studentBookingsView.metrics.pendingCount : 0) + ")",
                        "Đã kết thúc (" + (studentBookingsView.metrics ? studentBookingsView.metrics.historyCount : 0) + ")"]

                SegmentedFilterButton {
                    text: modelData
                    active: index === studentBookingsView.activeFilterIndex
                    implicitHeight: 34
                    onClicked: studentBookingsView.activeFilterIndex = index
                }
            }

            Item { Layout.fillWidth: true }
        }

        // Main Bookings List Scroll Area
        ScrollView {
            id: bookingsScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: bookingsScrollView.availableWidth
                spacing: 14

                Repeater {
                    model: studentBookingsView.getFilteredBookings()

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: cardCol.implicitHeight + 32
                        color: Theme.cardBg
                        radius: 10
                        border.color: Theme.cardBorder
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            id: cardCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 16
                            spacing: 12

                            // Card Top Header Row
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                Rectangle {
                                    width: 48
                                    height: 48
                                    radius: 24
                                    color: Theme.sidebarBg
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.tutorInitials || "G"
                                        font.bold: true
                                        font.pixelSize: 18
                                        color: Theme.textLight
                                    }
                                }

                                ColumnLayout {
                                    spacing: 3
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0

                                    Text {
                                        text: modelData.tutorName || "Gia sư"
                                        font.bold: true
                                        font.pixelSize: 16
                                        color: Theme.textDark
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        text: "Môn giảng dạy: " + (modelData.subject || "Chưa xác định")
                                        font.pixelSize: 12
                                        color: Theme.textMuted
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        elide: Text.ElideRight
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                // Status Badge Pill
                                Rectangle {
                                    height: 28
                                    implicitWidth: statusText.implicitWidth + 20
                                    radius: 14
                                    color: (modelData.status === "ACCEPTED") ? "#132719" :
                                           (modelData.status === "PENDING") ? "#fffbe6" :
                                           (modelData.status === "COMPLETED") ? "#e6f7ff" :
                                           (modelData.status === "EARLY_TERMINATED") ? "#fff7e6" :
                                           (modelData.status === "REJECTED") ? "#fff1f0" : "#f5f5f5"
                                    border.color: (modelData.status === "ACCEPTED") ? "#52c41a" :
                                                  (modelData.status === "PENDING") ? "#f59e0b" :
                                                  (modelData.status === "COMPLETED") ? "#1890ff" :
                                                  (modelData.status === "EARLY_TERMINATED") ? "#ffa940" :
                                                  (modelData.status === "REJECTED") ? "#cf1322" : "#8c8c8c"
                                    border.width: 1

                                    Text {
                                        id: statusText
                                        anchors.centerIn: parent
                                        text: (modelData.status === "ACCEPTED") ? "✓ Đã kết nối" :
                                              (modelData.status === "PENDING") ? "⏳ Chờ duyệt" :
                                              (modelData.status === "COMPLETED") ? "★ Đã hoàn thành" :
                                              (modelData.status === "EARLY_TERMINATED") ? "⛔ Kết thúc sớm" :
                                              (modelData.status === "REJECTED") ? "✕ Từ chối" : "Đã hủy"
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: (modelData.status === "ACCEPTED") ? "#52c41a" :
                                               (modelData.status === "PENDING") ? "#d97706" :
                                               (modelData.status === "COMPLETED") ? "#1890ff" :
                                               (modelData.status === "EARLY_TERMINATED") ? "#d46b08" :
                                               (modelData.status === "REJECTED") ? "#cf1322" : "#8c8c8c"
                                    }
                                }
                            }

                            // Schedule & Tuition Container Card
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: schedInnerCol.implicitHeight + 20
                                color: Theme.bgApp
                                radius: 8
                                border.color: Theme.cardBorder
                                border.width: 1

                                ColumnLayout {
                                    id: schedInnerCol
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 10
                                    spacing: 4

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: "📅 Khung giờ học: " + (modelData.slotLabel || "N/A")
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: Theme.textDark
                                        wrapMode: Text.WordWrap
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        visible: modelData.isCourse === true
                                        text: "🗓️ Thời gian khóa học: " + (modelData.dateRangeLabel || "")
                                        font.pixelSize: 11
                                        color: Theme.textDark
                                        wrapMode: Text.WordWrap
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: modelData.isCourse ?
                                              ("💰 Học phí: " + ((modelData.hourlyRate || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ/buổi · Tổng khóa: " + ((modelData.totalCost || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ (" + modelData.totalSessions + " buổi)"))) :
                                              ("💰 Học phí thỏa thuận: " + ((modelData.hourlyRate || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ / buổi"))
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: "#d97706"
                                    }

                                    Flow {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        visible: modelData.isCourse === true && modelData.sessionDates && modelData.sessionDates.length > 0
                                        Repeater {
                                            model: modelData.sessionDates
                                            Rectangle {
                                                width: 82
                                                height: 18
                                                radius: 3
                                                color: "#f0f5ff"
                                                border.color: "#adc6ff"
                                                border.width: 1
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: "B" + (index + 1) + ": " + modelData
                                                    font.pixelSize: 9
                                                    color: "#1d39c4"
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Review Display Snippet (If Completed and Reviewed)
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: revCol.implicitHeight + 16
                                color: Theme.cMistyRose
                                radius: 6
                                border.color: Theme.cardBorder
                                border.width: 1
                                visible: modelData.isReviewed === true

                                ColumnLayout {
                                    id: revCol
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 8
                                    spacing: 3

                                    Text {
                                        text: "★ Đánh giá của bạn: " + modelData.reviewRating + "/5 sao"
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: "#d97706"
                                    }
                                    Text {
                                        text: "\"" + modelData.reviewComment + "\""
                                        font.italic: true
                                        font.pixelSize: 11
                                        color: Theme.textDark
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }

                            // Card Action Buttons Row
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Item { Layout.fillWidth: true }

                                // PENDING ACTIONS
                                Button {
                                    visible: modelData.status === "PENDING"
                                    text: "Hủy yêu cầu"
                                    focusPolicy: Qt.NoFocus
                                    implicitWidth: 120
                                    implicitHeight: 36
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: "#cf1322"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 8
                                        color: parent.down ? "#fff1f0" : (parent.hovered ? "#fff1f0" : Theme.bgApp)
                                        border.color: "#ff4d4f"
                                        border.width: 1
                                    }
                                    onClicked: {
                                        studentBookingsView.selectedBookingToCancel = modelData
                                        cancelConfirmModal.visible = true
                                    }
                                }

                                // ACCEPTED ACTIONS
                                Button {
                                    visible: modelData.status === "ACCEPTED"
                                    text: "Hoàn thành khóa học"
                                    focusPolicy: Qt.NoFocus
                                    implicitWidth: 150
                                    implicitHeight: 36
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: "#ffffff"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 8
                                        color: parent.down ? "#1b5299" : (parent.hovered ? "#3b82f6" : "#2563eb")
                                    }
                                    onClicked: {
                                        if (typeof guiBridge !== "undefined") {
                                            guiBridge.terminateCourse(modelData.id)
                                        }
                                    }
                                }

                                Button {
                                    visible: modelData.status === "ACCEPTED"
                                    text: "Xem vị trí trên TKB"
                                    focusPolicy: Qt.NoFocus
                                    implicitWidth: 140
                                    implicitHeight: 36
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: Theme.textDark
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 8
                                        color: parent.down ? Theme.sidebarActive : (parent.hovered ? Theme.bgApp : Theme.cMistyRose)
                                        border.color: Theme.cardBorder
                                        border.width: 1
                                    }
                                    onClicked: {
                                        // Switch to Tab 0 (Hồ sơ & Lịch học)
                                        studentBookingsView.navigateToTabRequested(0)
                                    }
                                }

                                Button {
                                    visible: modelData.status === "PENDING"
                                    text: "Hủy yêu cầu"
                                    focusPolicy: Qt.NoFocus
                                    implicitWidth: 110
                                    implicitHeight: 36
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: "#cf1322"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 8
                                        color: parent.down ? "#fff1f0" : (parent.hovered ? "#fff1f0" : Theme.bgApp)
                                        border.color: "#ff4d4f"
                                        border.width: 1
                                    }
                                    onClicked: {
                                        studentBookingsView.selectedBookingToCancel = modelData
                                        cancelConfirmModal.visible = true
                                    }
                                }

                                // Consensual Early Termination Actions
                                Button {
                                    visible: modelData.status === "ACCEPTED" && modelData.canRequestEarlyTermination === true
                                    text: "Đề nghị kết thúc sớm"
                                    focusPolicy: Qt.NoFocus
                                    implicitWidth: 150
                                    implicitHeight: 36
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: "#d46b08"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 8
                                        color: parent.down ? "#fff7e6" : (parent.hovered ? "#fff7e6" : Theme.bgApp)
                                        border.color: "#ffa940"
                                        border.width: 1
                                    }
                                    onClicked: {
                                        studentBookingsView.selectedBookingForEarlyTerm = modelData
                                        earlyTermModal.visible = true
                                    }
                                }

                                Rectangle {
                                    visible: modelData.status === "ACCEPTED" && modelData.isPendingEarlyTerminationFromMe === true
                                    height: 36
                                    implicitWidth: 230
                                    radius: 8
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
                                    visible: modelData.status === "ACCEPTED" && modelData.canRespondEarlyTermination === true
                                    spacing: 6

                                    Button {
                                        text: "Đồng ý kết thúc"
                                        focusPolicy: Qt.NoFocus
                                        implicitWidth: 120
                                        implicitHeight: 36
                                        contentItem: Text { text: parent.text; font.bold: true; font.pixelSize: 11; color: "#FFFFFF"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        background: Rectangle { radius: 8; color: "#d46b08" }
                                        onClicked: {
                                            if (typeof guiBridge !== "undefined") {
                                                guiBridge.respondEarlyTermination(modelData.id, true)
                                                studentBookingsView.reloadBookings()
                                            }
                                        }
                                    }

                                    Button {
                                        text: "Từ chối"
                                        focusPolicy: Qt.NoFocus
                                        implicitWidth: 80
                                        implicitHeight: 36
                                        contentItem: Text { text: parent.text; font.bold: true; font.pixelSize: 11; color: Theme.textDark; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        background: Rectangle { radius: 8; color: Theme.bgApp; border.color: Theme.cardBorder; border.width: 1 }
                                        onClicked: {
                                            if (typeof guiBridge !== "undefined") {
                                                guiBridge.respondEarlyTermination(modelData.id, false)
                                                studentBookingsView.reloadBookings()
                                            }
                                        }
                                    }
                                }

                                // COMPLETED / EARLY_TERMINATED ACTIONS
                                Button {
                                    visible: (modelData.status === "COMPLETED" || modelData.status === "EARLY_TERMINATED") && modelData.canReview === true
                                    text: "Viết đánh giá"
                                    focusPolicy: Qt.NoFocus
                                    implicitWidth: 130
                                    implicitHeight: 36
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: Theme.btnText
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 8
                                        color: parent.down ? Theme.btnPrimaryHover : (parent.hovered ? Theme.btnPrimaryHover : Theme.sidebarBg)
                                    }
                                    onClicked: {
                                        reviewModal.openWithBooking(modelData)
                                    }
                                }

                                Rectangle {
                                    visible: (modelData.status === "COMPLETED" || modelData.status === "EARLY_TERMINATED") && modelData.isReviewed === true
                                    height: 32
                                    implicitWidth: 120
                                    radius: 6
                                    color: Theme.cMistyRose
                                    border.color: "#d97706"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✓ Đã đánh giá"
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: "#d97706"
                                    }
                                }
                            }
                        }
                    }
                }

                // Empty State View Card
                Rectangle {
                    Layout.fillWidth: true
                    height: 220
                    color: Theme.cardBg
                    radius: 10
                    border.color: Theme.cardBorder
                    border.width: 1
                    visible: studentBookingsView.getFilteredBookings().length === 0

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 12

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "📑 Chưa có lịch học hoặc yêu cầu kết nối nào"
                            font.bold: true
                            font.pixelSize: 16
                            color: Theme.textDark
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Hãy chủ động tìm kiếm gia sư hoặc xem danh sách gợi ý thông minh để bắt đầu khóa học."
                            font.pixelSize: 12
                            color: Theme.textMuted
                        }

                        Button {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Tìm kiếm Gia sư ngay"
                            focusPolicy: Qt.NoFocus
                            contentItem: Text {
                                text: parent.text
                                font.bold: true
                                font.pixelSize: 12
                                color: Theme.btnText
                            }
                            background: Rectangle {
                                implicitWidth: 160
                                implicitHeight: 38
                                radius: 6
                                color: Theme.sidebarBg
                            }
                            onClicked: {
                                // Switch to Tab 1 (Tìm kiếm Gia sư)
                                studentBookingsView.navigateToTabRequested(1)
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                }
            }
        }
    }

    // Cancellation Confirmation Modal Dialog
    Item {
        id: cancelConfirmModal
        anchors.fill: parent
        visible: false
        z: 999

        Rectangle {
            anchors.fill: parent
            color: "#80000000"
            MouseArea { anchors.fill: parent; onClicked: {} }
        }

        Rectangle {
            width: 460
            height: 240
            anchors.centerIn: parent
            radius: 10
            color: Theme.cardBg
            border.color: Theme.cardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                Text {
                    text: "XÁC NHẬN HỦY LỊCH HỌC / YÊU CẦU DĂNG KÝ"
                    font.bold: true
                    font.pixelSize: 15
                    color: Theme.textDark
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.cardBorder }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: "Bạn có chắc chắn muốn hủy đơn đăng ký khóa học này không?\n" +
                          "Gia sư: " + (studentBookingsView.selectedBookingToCancel ? studentBookingsView.selectedBookingToCancel.tutorName : "") +
                          "\nKhung giờ: " + (studentBookingsView.selectedBookingToCancel ? studentBookingsView.selectedBookingToCancel.slotLabel : "")
                    font.pixelSize: 12
                    color: Theme.textDark
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
                        onClicked: cancelConfirmModal.visible = false
                    }

                    Button {
                        text: "Xác nhận hủy"
                        Layout.fillWidth: true
                        height: 38
                        contentItem: Text { text: parent.text; font.bold: true; color: "#FFFFFF"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: "#cf1322"; radius: 6 }
                        onClicked: {
                            if (studentBookingsView.selectedBookingToCancel && typeof guiBridge !== "undefined") {
                                var ok = guiBridge.qmlCancelBooking(studentBookingsView.selectedBookingToCancel.id)
                                cancelConfirmModal.visible = false
                                studentBookingsView.reloadBookings()
                            } else {
                                cancelConfirmModal.visible = false
                            }
                        }
                    }
                }
            }
        }
    }

    // Consensual Early Termination Request Modal
    Item {
        id: earlyTermModal
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
                    text: "Khóa học sẽ kết thúc sau khi gia sư đồng thuận. Các buổi học trong tương lai sẽ được giải phóng."
                    font.pixelSize: 11
                    color: Theme.textMuted
                }

                TextField {
                    id: earlyTermReasonInput
                    Layout.fillWidth: true
                    placeholderText: "Nhập lý do kết thúc sớm (ví dụ: đã nắm vững kiến thức)..."
                    color: Theme.textDark
                    background: Rectangle {
                        color: Theme.bgApp
                        radius: 6
                        border.color: earlyTermReasonInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder
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
                            earlyTermModal.visible = false
                            earlyTermReasonInput.text = ""
                        }
                    }

                    Button {
                        text: "Gửi đề nghị"
                        Layout.fillWidth: true
                        height: 38
                        contentItem: Text { text: parent.text; font.bold: true; color: "#FFFFFF"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: "#d46b08"; radius: 6 }
                        onClicked: {
                            if (studentBookingsView.selectedBookingForEarlyTerm && typeof guiBridge !== "undefined") {
                                guiBridge.requestEarlyTermination(studentBookingsView.selectedBookingForEarlyTerm.id, earlyTermReasonInput.text)
                                earlyTermModal.visible = false
                                earlyTermReasonInput.text = ""
                                studentBookingsView.reloadBookings()
                            } else {
                                earlyTermModal.visible = false
                            }
                        }
                    }
                }
            }
        }
    }

    // Review Modal Integration
    ReviewModal {
        id: reviewModal
        onReviewSubmitted: {
            studentBookingsView.reloadBookings()
        }
    }
}
