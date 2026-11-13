import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: bookingModal
    anchors.fill: parent
    visible: false
    z: 999

    property var tutorData: ({})
    property int selectedDayIndex: -1
    property int selectedSlotIndex: -1
    property string selectedSlotLabel: ""
    property string errorMessage: ""
    property var availableSlotsList: []
    property var groupedDaysList: []
    property var selectedSlots: []

    // Recurring Course Booking Specific Properties
    property int selectedWeeksCount: 4
    property string selectedStartDate: ""
    property var candidateStartDates: []
    property var sessionPreviewDates: []
    property bool isSubmitting: false
    property bool commitmentAcknowledged: false

    signal bookingCompleted()

    function isSlotSelected(day, slot) {
        for (var i = 0; i < selectedSlots.length; i++) {
            if (selectedSlots[i].dayIndex === day && selectedSlots[i].slotIndex === slot) return true;
        }
        return false;
    }

    function toggleSlot(slotData) {
        var idx = -1;
        for (var i = 0; i < selectedSlots.length; i++) {
            if (selectedSlots[i].dayIndex === slotData.dayIndex && selectedSlots[i].slotIndex === slotData.slotIndex) {
                idx = i;
                break;
            }
        }
        var arr = selectedSlots.slice();
        if (idx >= 0) {
            arr.splice(idx, 1);
        } else {
            arr.push({
                dayIndex: slotData.dayIndex,
                slotIndex: slotData.slotIndex,
                timeRange: slotData.timeRange,
                dayName: slotData.dayName,
                slotLabel: slotData.slotLabel
            });
        }
        selectedSlots = arr;
        errorMessage = "";

        if (selectedSlots.length > 0) {
            selectedDayIndex = selectedSlots[0].dayIndex;
            selectedSlotIndex = selectedSlots[0].slotIndex;
            var labels = [];
            for (var k = 0; k < selectedSlots.length; k++) {
                labels.push(selectedSlots[k].dayName + " " + selectedSlots[k].timeRange);
            }
            selectedSlotLabel = labels.join(", ");

            if (typeof guiBridge !== "undefined") {
                var upcoming = guiBridge.getUpcomingStartDatesForSlots(selectedSlots, 4) || [];
                candidateStartDates = upcoming;
                if (upcoming.length > 0) {
                    selectedStartDate = upcoming[0].date;
                } else {
                    selectedStartDate = "";
                }
            }
        } else {
            selectedDayIndex = -1;
            selectedSlotIndex = -1;
            selectedSlotLabel = "";
            selectedStartDate = "";
            candidateStartDates = [];
        }
        updatePreview();
    }

    function updatePreview() {
        if (typeof guiBridge !== "undefined" && selectedStartDate !== "" && selectedWeeksCount > 0 && selectedSlots.length > 0) {
            var raw = guiBridge.calculateMultiSlotSessionDates(selectedStartDate, selectedWeeksCount, selectedSlots) || [];
            var formatted = [];
            for (var i = 0; i < raw.length; i++) {
                formatted.push(raw[i].label || raw[i].date);
            }
            sessionPreviewDates = formatted;
        } else if (typeof guiBridge !== "undefined" && selectedStartDate !== "" && selectedWeeksCount > 0 && selectedDayIndex >= 0) {
            sessionPreviewDates = guiBridge.calculateCourseSessionDates(selectedStartDate, selectedWeeksCount) || [];
        } else {
            sessionPreviewDates = [];
        }
    }

    function updateAvailableSlots() {
        if (typeof guiBridge === "undefined" || !tutorData.id) {
            availableSlotsList = []
            groupedDaysList = []
            return
        }
        var rawSlots = guiBridge.getTutorAvailableBookingSlots(tutorData.id)
        availableSlotsList = rawSlots || []

        // Group slots by dayIndex (0..6)
        var daysMap = {}
        var dayOrder = []
        for (var i = 0; i < availableSlotsList.length; i++) {
            var s = availableSlotsList[i]
            var dIdx = s.dayIndex
            if (!daysMap[dIdx]) {
                daysMap[dIdx] = {
                    dayIndex: dIdx,
                    dayName: s.dayName,
                    slots: []
                }
                dayOrder.push(dIdx)
            }
            daysMap[dIdx].slots.push(s)
        }

        var result = []
        for (var j = 0; j < dayOrder.length; j++) {
            result.push(daysMap[dayOrder[j]])
        }
        groupedDaysList = result
    }

    function openWithTutor(tutor) {
        tutorData = tutor || {}
        selectedSlots = []
        selectedDayIndex = -1
        selectedSlotIndex = -1
        selectedSlotLabel = ""
        selectedWeeksCount = 4
        selectedStartDate = ""
        candidateStartDates = []
        sessionPreviewDates = []
        errorMessage = ""
        isSubmitting = false
        commitmentAcknowledged = false
        subjectInput.text = tutor.subject || ""

        updateAvailableSlots()
        bookingModal.visible = true
    }

    // Modal Background Overlay Dimming
    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        MouseArea {
            anchors.fill: parent
            onClicked: {} // Prevent click-through
        }
    }

    // Centered Modal Window Card
    Rectangle {
        id: modalCard
        width: Math.min(720, parent.width - 32)
        height: Math.min(720, parent.height - 32)
        anchors.centerIn: parent
        radius: 12
        color: Theme.cardBg
        border.color: Theme.cardBorder
        border.width: 1
        clip: true

        MouseArea {
            anchors.fill: parent
            onClicked: {} // Prevent click-through
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 10

            // =========================================================
            // 1. HEADER (Fixed at top)
            // =========================================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "ĐĂNG KÝ KHÓA HỌC ĐỊNH KỲ"
                        font.bold: true
                        font.pixelSize: 18
                        color: Theme.textDark
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "✕"
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 16
                            color: Theme.textMuted
                        }
                        background: Rectangle { color: "transparent" }
                        onClicked: {
                            if (!bookingModal.isSubmitting) bookingModal.visible = false
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.cardBorder
                }

                // Tutor Identity Header Card
                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: Theme.bgApp
                    radius: 6
                    border.color: Theme.cardBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 12

                        Text {
                            text: "Gia sư: " + (tutorData.fullName || "")
                            font.bold: true
                            font.pixelSize: 13
                            color: Theme.textDark
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: "Học phí: " + ((tutorData.hourlyRate || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ / buổi")
                            font.pixelSize: 13
                            font.bold: true
                            color: "#d97706"
                        }
                    }
                }

                // Error Banner
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    radius: 6
                    color: "#fff1f0"
                    border.color: "#ff4d4f"
                    border.width: 1
                    visible: bookingModal.errorMessage !== ""

                    Text {
                        anchors.centerIn: parent
                        text: bookingModal.errorMessage
                        font.pixelSize: 11
                        font.bold: true
                        color: "#cf1322"
                    }
                }
            }

            // =========================================================
            // 2. BODY (Scrollable area)
            // =========================================================
            ScrollView {
                id: bodyScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                ColumnLayout {
                    width: bodyScrollView.availableWidth
                    spacing: 12

                    // Subject Selection Input
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text { text: "Môn đăng ký (*):"; font.bold: true; color: Theme.textDark; font.pixelSize: 12 }
                        TextField {
                            id: subjectInput
                            Layout.fillWidth: true
                            color: Theme.textDark
                            placeholderText: "Nhập tên môn học..."
                            background: Rectangle {
                                color: Theme.bgApp
                                radius: 6
                                border.color: subjectInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                            }
                        }
                    }

                    // 1. Slot Selection Section
                    Text {
                        text: "1. CHỌN THỜI KHÓA BIỂU ĐỊNH KỲ (Thứ & Ca học):"
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.textDark
                    }

                    // Scrollable Day-Grouped Available Slots Area
                    ScrollView {
                        id: slotsScrollView
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(140, Math.max(60, slotsCol.implicitHeight))
                        clip: true
                        contentWidth: availableWidth
                        visible: bookingModal.groupedDaysList.length > 0

                        ColumnLayout {
                            id: slotsCol
                            width: slotsScrollView.availableWidth
                            spacing: 8

                            Repeater {
                                model: bookingModal.groupedDaysList

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Rectangle {
                                            width: 75
                                            height: 22
                                            radius: 4
                                            color: Theme.sidebarBg

                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData.dayName
                                                font.bold: true
                                                font.pixelSize: 10
                                                color: Theme.sidebarActive
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 1
                                            color: Theme.cardBorder
                                        }
                                    }

                                    Flow {
                                        Layout.fillWidth: true
                                        width: parent.width
                                        spacing: 6

                                        Repeater {
                                            model: modelData.slots

                                            Rectangle {
                                                width: 110
                                                height: 28
                                                radius: 4

                                                readonly property bool isSelected: bookingModal.isSlotSelected(modelData.dayIndex, modelData.slotIndex)

                                                color: isSelected ? Theme.sidebarBg : (btnMouse.containsMouse ? Theme.sidebarActive : Theme.bgApp)
                                                border.color: isSelected ? Theme.sidebarBg : Theme.cardBorder
                                                border.width: isSelected ? 2 : 1

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: (isSelected ? "✓ " : "") + modelData.timeRange
                                                    font.bold: isSelected
                                                    font.pixelSize: 10
                                                    color: isSelected ? Theme.textLight : Theme.textDark
                                                }

                                                MouseArea {
                                                    id: btnMouse
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: {
                                                        bookingModal.toggleSlot(modelData)
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Empty State Box
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        color: Theme.bgApp
                        radius: 6
                        border.color: Theme.cardBorder
                        border.width: 1
                        visible: bookingModal.groupedDaysList.length === 0

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 4

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "⚠️ Gia sư hiện chưa có khung giờ khả dụng"
                                font.bold: true
                                font.pixelSize: 12
                                color: Theme.textDark
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "Tất cả các ca học đã có người đăng ký hoặc chưa mở lịch rảnh."
                                font.pixelSize: 10
                                color: Theme.textMuted
                            }
                        }
                    }

                    // 2. Weeks Count & Start Date Section (Shown when a slot is chosen)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: bookingModal.selectedDayIndex >= 0

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Text {
                                text: "2. THỜI LƯỢNG KHÓA HỌC:"
                                font.bold: true
                                font.pixelSize: 11
                                color: Theme.textDark
                            }

                            Repeater {
                                model: [4, 8, 12]

                                Rectangle {
                                    width: 100
                                    height: 28
                                    radius: 6
                                    readonly property bool isSel: bookingModal.selectedWeeksCount === modelData
                                    color: isSel ? Theme.sidebarBg : (wkMouse.containsMouse ? Theme.sidebarActive : Theme.bgApp)
                                    border.color: isSel ? Theme.sidebarBg : Theme.cardBorder
                                    border.width: isSel ? 2 : 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData + " tuần (" + modelData + " buổi)"
                                        font.bold: parent.isSel
                                        font.pixelSize: 10
                                        color: parent.isSel ? Theme.textLight : Theme.textDark
                                    }

                                    MouseArea {
                                        id: wkMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            bookingModal.selectedWeeksCount = modelData
                                            bookingModal.updatePreview()
                                        }
                                    }
                                }
                            }
                        }

                        // Start Date Selection Chips
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: "3. NGÀY BẮT ĐẦU:"
                                font.bold: true
                                font.pixelSize: 11
                                color: Theme.textDark
                            }

                            Repeater {
                                model: bookingModal.candidateStartDates

                                Rectangle {
                                    id: dtChipRect
                                    implicitWidth: Math.max(120, dtChipText.implicitWidth + 16)
                                    width: implicitWidth
                                    height: 28
                                    radius: 6
                                    readonly property bool isSel: bookingModal.selectedStartDate === modelData.date
                                    color: isSel ? "#2563eb" : (dtMouse.containsMouse ? "#e6f7ff" : Theme.bgApp)
                                    border.color: isSel ? "#1d4ed8" : Theme.cardBorder
                                    border.width: isSel ? 2 : 1

                                    Text {
                                        id: dtChipText
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.bold: parent.isSel
                                        font.pixelSize: 10
                                        color: parent.isSel ? "#ffffff" : Theme.textDark
                                    }

                                    MouseArea {
                                        id: dtMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            bookingModal.selectedStartDate = modelData.date
                                            bookingModal.updatePreview()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 3. Course Summary & Session Preview Card
                    Rectangle {
                        id: summaryCard
                        Layout.fillWidth: true
                        implicitHeight: summaryCol.implicitHeight + 20
                        color: Theme.bgApp
                        radius: 8
                        border.color: Theme.cardBorder
                        border.width: 1
                        visible: bookingModal.selectedDayIndex >= 0 && bookingModal.selectedStartDate !== ""

                        ColumnLayout {
                            id: summaryCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 10
                            spacing: 6

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "📋 THÔNG TIN KHÓA HỌC DỰ KIẾN:"
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: Theme.textDark
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: "Tổng học phí: " + (((tutorData.hourlyRate || 0) * (bookingModal.selectedSlots.length > 0 ? bookingModal.selectedSlots.length : 1) * bookingModal.selectedWeeksCount).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ")
                                    font.bold: true
                                    font.pixelSize: 12
                                    color: "#d97706"
                                }
                            }

                            Text {
                                text: "• Lịch học: " + bookingModal.selectedSlotLabel + " | Thời lượng: " + bookingModal.selectedWeeksCount + " tuần (" + ((bookingModal.selectedSlots.length > 0 ? bookingModal.selectedSlots.length : 1) * bookingModal.selectedWeeksCount) + " buổi)"
                                font.pixelSize: 11
                                color: Theme.textDark
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Text {
                                text: "• Khoảng thời gian: Từ " + bookingModal.selectedStartDate + " đến " +
                                      (bookingModal.sessionPreviewDates.length > 0 ? bookingModal.sessionPreviewDates[bookingModal.sessionPreviewDates.length - 1] : "")
                                font.pixelSize: 11
                                color: Theme.textDark
                            }

                            Text {
                                text: "• Danh sách các ngày học thực tế:"
                                font.bold: true
                                font.pixelSize: 10
                                color: Theme.textMuted
                            }

                            Flow {
                                id: sessionFlow
                                Layout.fillWidth: true
                                width: summaryCol.width
                                Layout.preferredHeight: childrenRect.height
                                spacing: 6

                                Repeater {
                                    model: bookingModal.sessionPreviewDates

                                    Rectangle {
                                        id: tagRect
                                        width: Math.min(sessionFlow.width > 0 ? sessionFlow.width : 500, tagText.implicitWidth + 20)
                                        height: Math.max(26, tagText.implicitHeight + 8)
                                        radius: 6
                                        color: "#e6f7ff"
                                        border.color: "#91d5ff"
                                        border.width: 1

                                        Text {
                                            id: tagText
                                            anchors.centerIn: parent
                                            width: Math.min(implicitWidth, parent.width - 12)
                                            text: "B" + (index + 1) + ": " + modelData
                                            font.pixelSize: 10
                                            color: "#096dd9"
                                            font.bold: true
                                            elide: Text.ElideRight
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 4. Commitment Acknowledgment Card (COMMITMENT_V1)
                    Rectangle {
                        id: commitmentCard
                        Layout.fillWidth: true
                        implicitHeight: commitmentCol.implicitHeight + 20
                        color: "#f8fafc"
                        radius: 8
                        border.color: bookingModal.commitmentAcknowledged ? "#93c5fd" : Theme.cardBorder
                        border.width: 1

                        ColumnLayout {
                            id: commitmentCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 10
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "📜 ĐIỀU KHOẢN CAM KẾT KHÓA HỌC (" + (typeof guiBridge !== "undefined" ? guiBridge.getCommitmentVersion() : "COMMITMENT_V1") + "):"
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: Theme.textDark
                                }
                            }

                            Text {
                                text: (typeof guiBridge !== "undefined" ? guiBridge.getCommitmentPolicyText() : "Học viên cam kết tham gia đầy đủ các buổi học theo lịch đã đăng ký. Việc kết thúc sớm cần sự đồng thuận của cả hai bên theo quy trình cam kết.")
                                font.pixelSize: 10
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                CheckBox {
                                    id: studentAckCheckbox
                                    checked: bookingModal.commitmentAcknowledged
                                    onToggled: bookingModal.commitmentAcknowledged = checked
                                }

                                Text {
                                    text: "Tôi đã đọc, hiểu rõ và đồng ý với chính sách cam kết khóa học và điều khoản kết thúc sớm."
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: bookingModal.commitmentAcknowledged ? "#1d4ed8" : Theme.textDark
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            studentAckCheckbox.checked = !studentAckCheckbox.checked;
                                            bookingModal.commitmentAcknowledged = studentAckCheckbox.checked;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // =========================================================
            // 3. FOOTER (Fixed at bottom)
            // =========================================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.cardBorder
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: "Hủy bỏ"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        enabled: !bookingModal.isSubmitting
                        contentItem: Text {
                            text: parent.text
                            font.bold: true
                            color: Theme.textDark
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                            radius: 6
                            border.color: Theme.cardBorder
                        }
                        onClicked: bookingModal.visible = false
                    }

                    Button {
                        text: bookingModal.isSubmitting ? "Đang gửi yêu cầu..." : "Xác nhận gửi yêu cầu đăng ký khóa học"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        enabled: (bookingModal.selectedSlots.length > 0 &&
                                  bookingModal.selectedStartDate !== "" &&
                                  bookingModal.commitmentAcknowledged &&
                                  !bookingModal.isSubmitting)
                        contentItem: Text {
                            text: parent.text
                            font.bold: true
                            color: parent.enabled ? Theme.btnText : Theme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.enabled ? (parent.hovered ? Theme.sidebarBg : Theme.btnPrimary) : "#e0e0e0"
                            radius: 6
                        }
                        onClicked: {
                            if (bookingModal.selectedSlots.length === 0 || bookingModal.selectedStartDate === "") {
                                bookingModal.errorMessage = "Vui lòng chọn ít nhất một ca học và ngày bắt đầu!"
                                return;
                            }
                            if (!bookingModal.commitmentAcknowledged) {
                                bookingModal.errorMessage = "Bạn phải đồng ý với chính sách cam kết khóa học trước khi gửi yêu cầu!"
                                return;
                            }

                            if (typeof guiBridge !== "undefined") {
                                bookingModal.isSubmitting = true;
                                bookingModal.errorMessage = "";
                                var ok = false;
                                if (bookingModal.selectedSlots.length > 1) {
                                    ok = guiBridge.requestCourseBookingMulti(
                                        bookingModal.tutorData.id,
                                        subjectInput.text,
                                        bookingModal.selectedSlots,
                                        bookingModal.selectedStartDate,
                                        bookingModal.selectedWeeksCount,
                                        bookingModal.commitmentAcknowledged
                                    );
                                } else {
                                    ok = guiBridge.requestRecurringCourseBooking(
                                        bookingModal.tutorData.id,
                                        subjectInput.text,
                                        bookingModal.selectedSlots[0].dayIndex,
                                        bookingModal.selectedSlots[0].slotIndex,
                                        bookingModal.selectedStartDate,
                                        bookingModal.selectedWeeksCount,
                                        bookingModal.commitmentAcknowledged
                                    );
                                }
                                bookingModal.isSubmitting = false;
                                if (ok) {
                                    bookingModal.visible = false;
                                    bookingModal.bookingCompleted();
                                } else {
                                    bookingModal.errorMessage = guiBridge.statusMessage || "Lỗi tạo yêu cầu đăng ký khóa học!";
                                    bookingModal.updateAvailableSlots();
                                }
                            } else {
                                bookingModal.visible = false;
                                bookingModal.bookingCompleted();
                            }
                        }
                    }
                }
            }
        }
    }
}
