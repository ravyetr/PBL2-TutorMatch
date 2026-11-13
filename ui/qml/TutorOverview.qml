import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: tutorOverviewRoot

    signal navigateToTabRequested(int tabIndex)

    property var overviewData: ({})
    property var upcomingLessons: []
    property var pendingBookings: []

    function reloadData() {
        if (typeof guiBridge !== "undefined") {
            overviewData = guiBridge.qmlGetTutorOverview() || {}
            upcomingLessons = guiBridge.qmlGetTutorScheduleLessons() || []
            
            var allBookings = guiBridge.qmlGetTutorBookings() || []
            var pending = []
            for (var i = 0; i < allBookings.length; i++) {
                if (allBookings[i].status === "PENDING") {
                    pending.push(allBookings[i])
                }
            }
            pendingBookings = pending
        }
    }

    Component.onCompleted: reloadData()

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onDataChanged() { reloadData() }
        function onTutorScheduleChanged() { reloadData() }
        function onTutorProfileChanged() { reloadData() }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 20

            // 1. Welcome Header Card
            Rectangle {
                Layout.fillWidth: true
                height: 100
                radius: 12
                color: Theme.sidebarBg

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 16

                    Rectangle {
                        width: 56
                        height: 56
                        radius: 28
                        color: Theme.sidebarActive
                        Text {
                            anchors.centerIn: parent
                            text: (tutorOverviewRoot.overviewData.fullName || "G").substring(0, 1).toUpperCase()
                            font.bold: true
                            font.pixelSize: 24
                            color: Theme.textDark
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Text {
                            text: "Chào mừng, " + (tutorOverviewRoot.overviewData.fullName || "Gia sư")
                            font.bold: true
                            font.pixelSize: 20
                            color: Theme.textLight
                        }
                        Text {
                            text: "Chuyên môn: " + (tutorOverviewRoot.overviewData.subjectSpecialty || "Chưa cập nhật") + " · Học phí: " + ((tutorOverviewRoot.overviewData.hourlyRate || 150000).toLocaleString(Qt.locale("vi_VN"), 'f', 0)) + " VNĐ / buổi"
                            font.pixelSize: 13
                            color: Theme.sidebarActive
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        id: editProfileBtn
                        text: "Chỉnh sửa hồ sơ"
                        contentItem: Text {
                            text: parent.text
                            font.bold: true
                            font.pixelSize: 12
                            color: Theme.btnText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            implicitWidth: 140
                            implicitHeight: 36
                            radius: 6
                            color: parent.hovered ? Theme.sidebarActive : Theme.btnPrimary
                        }
                        onClicked: {
                            editModal.openWithProfile(tutorOverviewRoot.overviewData)
                        }
                    }
                }
            }

            // 2. Metric Cards Grid (5 Summary Cards)
            GridLayout {
                Layout.fillWidth: true
                columns: (width > 1050) ? 5 : (width > 650 ? 3 : 2)
                rowSpacing: 12
                columnSpacing: 12

                // Card 1: Students
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 10
                    color: Theme.cardBg
                    border.color: Theme.cardBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 4
                        Text { text: "HỌC VIÊN ĐANG THEO HỌC"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                        Text { text: (tutorOverviewRoot.overviewData.totalStudents || 0).toString() + " học viên"; font.bold: true; font.pixelSize: 22; color: Theme.textDark }
                    }
                }

                // Card 2: Upcoming Lessons
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 10
                    color: Theme.cardBg
                    border.color: Theme.cardBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 4
                        Text { text: "CA HỌC CỐ ĐỊNH"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                        Text { text: (tutorOverviewRoot.overviewData.upcomingLessonsCount || 0).toString() + " ca / tuần"; font.bold: true; font.pixelSize: 22; color: "#2563eb" }
                    }
                }

                // Card 3: Pending Requests
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 10
                    color: (tutorOverviewRoot.overviewData.pendingBookingsCount > 0) ? "#fffbe6" : Theme.cardBg
                    border.color: (tutorOverviewRoot.overviewData.pendingBookingsCount > 0) ? "#ffe58f" : Theme.cardBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 4
                        Text { text: "YÊU CẦU ĐANG CHỜ DUYỆT"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                        Text { text: (tutorOverviewRoot.overviewData.pendingBookingsCount || 0).toString() + " đơn mới"; font.bold: true; font.pixelSize: 22; color: "#d97706" }
                    }
                }

                // Card 4: Rating
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 10
                    color: Theme.cardBg
                    border.color: Theme.cardBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 4
                        Text { text: "ĐÁNH GIÁ TRUNG BÌNH"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                        Text { text: (tutorOverviewRoot.overviewData.rating || 5.0).toFixed(1) + " ★ (" + (tutorOverviewRoot.overviewData.totalReviews || 0) + ")"; font.bold: true; font.pixelSize: 22; color: "#d97706" }
                    }
                }

                // Card 5: Accumulated Income
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 10
                    color: Theme.cardBg
                    border.color: Theme.cardBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 4
                        Text { text: "TỔNG HỌC PHÍ TÍCH LŨY"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                        Text {
                            text: ((tutorOverviewRoot.overviewData.totalEarnings || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0)) + " VNĐ"
                            font.bold: true
                            font.pixelSize: 20
                            color: "#16a34a"
                        }
                    }

                    ToolTip.visible: mouseAreaIncome.containsMouse
                    ToolTip.text: "Giá trị học phí của các khóa học đã hoàn thành theo thỏa thuận; không đồng nghĩa với số tiền đã thực nhận."

                    MouseArea {
                        id: mouseAreaIncome
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }
            }

            Text {
                text: "💡 Lưu ý: Tổng học phí tích lũy được tính từ các khóa học đã hoàn thành (COMPLETED) theo thỏa thuận, không đồng nghĩa với số tiền đã thực nhận."
                font.pixelSize: 11
                color: Theme.textMuted
            }

            // 3. Quick Action Toolbar
            Rectangle {
                Layout.fillWidth: true
                height: 60
                radius: 10
                color: Theme.cardBg
                border.color: Theme.cardBorder

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Text { text: "Thao tác nhanh:"; font.bold: true; font.pixelSize: 13; color: Theme.textDark }

                    Button {
                        text: "📩 Duyệt yêu cầu (" + (tutorOverviewRoot.overviewData.pendingBookingsCount || 0) + ")"
                        onClicked: tutorOverviewRoot.navigateToTabRequested(1)
                    }

                    Button {
                        text: "🎓 Quản lý Học viên"
                        onClicked: tutorOverviewRoot.navigateToTabRequested(2)
                    }

                    Button {
                        text: "⭐ Đánh giá từ Học viên"
                        onClicked: tutorOverviewRoot.navigateToTabRequested(3)
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // 4. Pending Booking Alert Section
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10
                visible: tutorOverviewRoot.pendingBookings.length > 0

                Text { text: "YÊU CẦU ĐĂNG KÝ MỚI CHỜ XỬ LÝ"; font.bold: true; font.pixelSize: 15; color: Theme.textDark }

                Repeater {
                    model: tutorOverviewRoot.pendingBookings

                    Rectangle {
                        Layout.fillWidth: true
                        height: 70
                        radius: 8
                        color: "#fffbe6"
                        border.color: "#ffe58f"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 16

                            Text {
                                text: "👤 " + modelData.studentName
                                font.bold: true
                                font.pixelSize: 14
                                color: Theme.textDark
                            }

                            Text {
                                text: "📚 " + modelData.subject + " · ⏰ " + modelData.slotLabel
                                font.pixelSize: 13
                                color: Theme.textDark
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                text: "Chấp nhận"
                                onClicked: {
                                    if (typeof guiBridge !== "undefined") {
                                        guiBridge.qmlAcceptBooking(modelData.id)
                                    }
                                }
                            }

                            Button {
                                text: "Từ chối"
                                onClicked: {
                                    if (typeof guiBridge !== "undefined") {
                                        guiBridge.qmlRejectBooking(modelData.id)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 5. Weekly Teaching Timetable Grid
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                Text { text: "THỜI KHÓA BIỂU GIẢNG DẠY TRONG TUẦN"; font.bold: true; font.pixelSize: 15; color: Theme.textDark }

                TimetableGrid {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 560
                    Layout.minimumHeight: 480
                }
            }
        }
    }

    ProfileEditModal {
        id: editModal
        isTutor: true
    }
}
