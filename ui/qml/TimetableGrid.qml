import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: timetableGrid
    color: Theme.cardBg
    radius: 12
    border.color: Theme.cardBorder
    border.width: 1
    implicitWidth: 800
    implicitHeight: 560

    property var scheduleData: []
    property var scheduleCells: []
    property bool readOnly: false
    property string currentMonday: ""
    property var weekDaysInfo: []
    property string weekRangeText: ""
    property string fullTodayText: ""

    signal cellClicked(int day, int slot)

    function reloadSchedule() {
        if (typeof guiBridge !== "undefined") {
            if (currentMonday === "") {
                currentMonday = guiBridge.getWeekMonday()
            }
            weekDaysInfo = guiBridge.getWeekDaysInfo(currentMonday) || []
            weekRangeText = guiBridge.formatWeekRangeDisplay(currentMonday) || ""
            fullTodayText = guiBridge.getCurrentFullDateString() || ""

            if (scheduleData && scheduleData.length > 0) {
                scheduleCells = scheduleData
                return
            }

            if (guiBridge.currentUserRole === "Gia sư" || guiBridge.currentUserRole === "Tutor") {
                scheduleCells = guiBridge.getTutorWeeklySchedule(currentMonday)
            } else if (guiBridge.currentUserRole === "Học viên" || guiBridge.currentUserRole === "Student") {
                scheduleCells = guiBridge.getStudentWeeklySchedule(currentMonday)
            } else if (typeof guiBridge.getWeeklyScheduleMatrix === "function") {
                scheduleCells = guiBridge.getWeeklyScheduleMatrix(currentMonday)
            } else {
                scheduleCells = guiBridge.getStudentWeeklySchedule(currentMonday)
            }
        }
    }

    onScheduleDataChanged: {
        reloadSchedule()
    }

    Component.onCompleted: {
        reloadSchedule()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onStudentWeeklyScheduleChanged() {
            reloadSchedule()
        }
        function onTutorScheduleChanged() {
            reloadSchedule()
        }
        function onDataChanged() {
            reloadSchedule()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // Row 1: Header Title & Current Date (Local Time)
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "THỜI KHÓA BIỂU"
                    font.bold: true
                    font.pixelSize: 15
                    color: Theme.textDark
                }
                Text {
                    text: timetableGrid.readOnly ? "Chế độ xem thời khóa biểu (Read-only)" : "Nhấp vào ô để thay đổi RẢNH ↔ BẬN cá nhân"
                    font.pixelSize: 11
                    color: Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            // Current Date Pill
            Rectangle {
                height: 28
                implicitWidth: todayRow.implicitWidth + 16
                radius: 6
                color: Theme.bgApp
                border.color: Theme.cardBorder
                border.width: 1

                RowLayout {
                    id: todayRow
                    anchors.centerIn: parent
                    spacing: 6
                    Text {
                        text: "📅 Hôm nay: " + (timetableGrid.fullTodayText || "")
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.textHeading
                    }
                }
            }
        }

        // Row 2: Week Navigation Controls, Range Label & Status Legends
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Navigation buttons (Previous, Today, Next)
            RowLayout {
                spacing: 6

                Button {
                    id: prevWeekBtn
                    text: "◀ Tuần trước"
                    contentItem: Text {
                        text: prevWeekBtn.text
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.textDark
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 92
                        implicitHeight: 30
                        radius: 6
                        color: prevWeekBtn.hovered ? Theme.cMistyRose : Theme.bgApp
                        border.color: Theme.cardBorder
                        border.width: 1
                    }
                    onClicked: {
                        if (typeof guiBridge !== "undefined") {
                            timetableGrid.currentMonday = guiBridge.shiftWeek(timetableGrid.currentMonday, -1)
                            timetableGrid.reloadSchedule()
                        }
                    }
                }

                Button {
                    id: todayWeekBtn
                    text: "Tuần hiện tại"
                    contentItem: Text {
                        text: todayWeekBtn.text
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.cBurgundy
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 92
                        implicitHeight: 30
                        radius: 6
                        color: todayWeekBtn.hovered ? Theme.bgApp : Theme.cCherryBlossom
                        border.color: Theme.cBurgundy
                        border.width: 1
                    }
                    onClicked: {
                        if (typeof guiBridge !== "undefined") {
                            timetableGrid.currentMonday = guiBridge.getWeekMonday()
                            timetableGrid.reloadSchedule()
                        }
                    }
                }

                Button {
                    id: nextWeekBtn
                    text: "Tuần sau ▶"
                    contentItem: Text {
                        text: nextWeekBtn.text
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.textDark
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 92
                        implicitHeight: 30
                        radius: 6
                        color: nextWeekBtn.hovered ? Theme.cMistyRose : Theme.bgApp
                        border.color: Theme.cardBorder
                        border.width: 1
                    }
                    onClicked: {
                        if (typeof guiBridge !== "undefined") {
                            timetableGrid.currentMonday = guiBridge.shiftWeek(timetableGrid.currentMonday, 1)
                            timetableGrid.reloadSchedule()
                        }
                    }
                }

                Rectangle {
                    height: 30
                    implicitWidth: weekRangeTxt.implicitWidth + 16
                    radius: 6
                    color: Theme.cardBg
                    border.color: Theme.accentBlue
                    border.width: 1

                    Text {
                        id: weekRangeTxt
                        anchors.centerIn: parent
                        text: "Tuần: " + (timetableGrid.weekRangeText || "")
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.cLapisLazuli
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // 3-item Legend indicators (BOOKED, FREE, BUSY)
            RowLayout {
                spacing: 12

                // BOOKED Legend - Cherry Blossom
                RowLayout {
                    spacing: 5
                    Rectangle {
                        width: 14
                        height: 14
                        radius: 3
                        color: Theme.slotBookedBg
                        border.color: Theme.slotBookedText
                        border.width: 1
                    }
                    Text {
                        text: "Đã chốt lịch"
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.slotBookedText
                    }
                }

                // FREE Legend - White sample square with Silver Lake Blue border
                RowLayout {
                    spacing: 5
                    Rectangle {
                        width: 14
                        height: 14
                        radius: 3
                        color: Theme.cardBg
                        border.color: Theme.slotFreeBorder
                        border.width: 1
                    }
                    Text {
                        text: "Rảnh"
                        font.pixelSize: 11
                        color: Theme.textDark
                    }
                }

                // BUSY Legend - Misty Rose sample square
                RowLayout {
                    spacing: 5
                    Rectangle {
                        width: 14
                        height: 14
                        radius: 3
                        color: Theme.slotBusyBg
                        border.color: Theme.cardBorder
                        border.width: 1
                    }
                    Text {
                        text: "Bận cá nhân"
                        font.pixelSize: 11
                        color: Theme.textDark
                    }
                }
            }

            // Styled ActionButton to Clear All Personal Busy Slots
            Button {
                id: clearBtn
                visible: !timetableGrid.readOnly
                text: "Xóa tất cả ca bận"
                contentItem: Text {
                    text: clearBtn.text
                    font.bold: true
                    font.pixelSize: 11
                    color: Theme.textDark
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 120
                    implicitHeight: 30
                    radius: 6
                    color: clearBtn.hovered ? Theme.bgApp : Theme.cMistyRose
                    border.color: Theme.cardBorder
                    border.width: 1
                }
                onClicked: {
                    if (typeof guiBridge !== "undefined") {
                        if (typeof guiBridge.clearAllBusySlots === "function") {
                            guiBridge.clearAllBusySlots()
                        } else {
                            guiBridge.clearStudentBusySchedule()
                        }
                        timetableGrid.reloadSchedule()
                    }
                }
            }
        }

        // 8 Columns × 9 Rows Timetable Grid Layout
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            // Row 0: Header Row (Col 0 = Time Header, Col 1-7 = 7 Day Headers with Date & Today indicator)
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                // Col 0 Header: Time Column Header
                Rectangle {
                    Layout.preferredWidth: 110
                    height: 38
                    color: Theme.sidebarBg
                    radius: 6

                    Text {
                        anchors.centerIn: parent
                        text: "Khung giờ"
                        font.bold: true
                        font.pixelSize: 12
                        color: Theme.textLight
                    }
                }

                // Col 1-7 Headers: Days Monday -> Sunday with actual calendar dates
                Repeater {
                    model: (timetableGrid.weekDaysInfo && timetableGrid.weekDaysInfo.length === 7) ? timetableGrid.weekDaysInfo : [
                        { dayName: "Thứ 2", displayDate: "", isToday: false },
                        { dayName: "Thứ 3", displayDate: "", isToday: false },
                        { dayName: "Thứ 4", displayDate: "", isToday: false },
                        { dayName: "Thứ 5", displayDate: "", isToday: false },
                        { dayName: "Thứ 6", displayDate: "", isToday: false },
                        { dayName: "Thứ 7", displayDate: "", isToday: false },
                        { dayName: "Chủ Nhật", displayDate: "", isToday: false }
                    ]

                    Rectangle {
                        Layout.fillWidth: true
                        height: 38
                        color: modelData.isToday ? Theme.cLapisLazuli : Theme.sidebarBg
                        radius: 6
                        border.color: modelData.isToday ? Theme.cCherryBlossom : "transparent"
                        border.width: modelData.isToday ? 2 : 0

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 1

                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 4
                                Text {
                                    text: modelData.dayName
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: Theme.textLight
                                }
                                Rectangle {
                                    visible: modelData.isToday
                                    implicitWidth: 32
                                    implicitHeight: 14
                                    radius: 3
                                    color: Theme.cCherryBlossom
                                    Text {
                                        anchors.centerIn: parent
                                        text: "Hôm nay"
                                        font.bold: true
                                        font.pixelSize: 8
                                        color: Theme.cBurgundy
                                    }
                                }
                            }

                            Text {
                                visible: modelData.displayDate !== ""
                                Layout.alignment: Qt.AlignHCenter
                                text: modelData.displayDate
                                font.pixelSize: 10
                                font.bold: modelData.isToday
                                color: modelData.isToday ? Theme.cCherryBlossom : "#d1d5db"
                            }
                        }
                    }
                }
            }

            // Rows 1-8: 8 Time Slot Rows
            Repeater {
                model: [
                    "07:30 - 09:00", "09:15 - 10:45", "13:30 - 15:00", "15:15 - 16:45",
                    "17:00 - 18:30", "18:45 - 20:15", "20:30 - 22:00", "22:00 - 23:30"
                ]

                RowLayout {
                    id: slotRow
                    property int slotIdx: index
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 38
                    Layout.preferredHeight: 46
                    spacing: 6

                    // Time Column Cell (Col 0)
                    Rectangle {
                        Layout.preferredWidth: 110
                        Layout.fillHeight: true
                        Layout.minimumHeight: 38
                        Layout.preferredHeight: 46
                        color: Theme.bgApp
                        radius: 6
                        border.color: Theme.cardBorder
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            font.bold: true
                            font.pixelSize: 11
                            color: Theme.textDark
                        }
                    }

                    // 7 Day Cells (Col 1-7)
                    Repeater {
                        model: 7

                        TimetableCell {
                            property int dayIdx: index
                            property int flatIndex: slotRow.slotIdx * 7 + dayIdx
                            property var cellData: (timetableGrid.scheduleCells && timetableGrid.scheduleCells.length > flatIndex) ? timetableGrid.scheduleCells[flatIndex] : ({})

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 38
                            Layout.preferredHeight: 46

                            dayIndex: dayIdx
                            slotIndex: slotRow.slotIdx
                            status: cellData.status || "FREE"
                            displayName: cellData.displayName || cellData.primaryText || cellData.tutorName || cellData.studentName || ""
                            subjectName: cellData.subjectName || cellData.secondaryText || cellData.label || ""
                            primaryText: cellData.primaryText || cellData.displayName || ""
                            secondaryText: cellData.secondaryText || cellData.subjectName || ""
                            tutorName: cellData.tutorName || ""
                            studentName: cellData.studentName || ""
                            label: cellData.label || ""
                            subLabel: cellData.subLabel || ""
                            timeRange: cellData.timeRange || ""
                            sessionLabel: cellData.sessionLabel || ""
                            date: cellData.date || ""
                            isCourse: cellData.isCourse || false
                            bookingId: cellData.bookingId || 0
                            editable: cellData.editable !== undefined ? cellData.editable : (cellData.status !== "BOOKED" && cellData.status !== "ACCEPTED")
                            locked: cellData.locked !== undefined ? cellData.locked : (cellData.status === "BOOKED" || cellData.status === "ACCEPTED")

                            onToggleRequested: function(d, s) {
                                timetableGrid.cellClicked(d, s)
                                if (typeof guiBridge !== "undefined") {
                                    if (guiBridge.currentUserRole === "Gia sư" || guiBridge.currentUserRole === "Tutor") {
                                        guiBridge.qmlToggleTutorScheduleSlot(d, s)
                                    } else {
                                        guiBridge.qmlToggleStudentScheduleSlot(d, s)
                                    }
                                    timetableGrid.reloadSchedule()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
