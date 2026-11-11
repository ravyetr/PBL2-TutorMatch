import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: studentSmartMatchView

    property var matchesList: []
    property bool isRefreshing: false
    property var studentProfile: (typeof guiBridge !== "undefined" && guiBridge.currentStudentProfile) ? guiBridge.currentStudentProfile : ({})

    // Dynamic Search Criteria (Independent of persistent Student Profile)
    property string selectedSubject: "Toán"
    property int selectedGrade: (studentProfile && studentProfile.gradeLevel > 0) ? studentProfile.gradeLevel : 10
    property double maxBudgetValue: 200000.0
    property string courseStartDate: "2026-11-16"
    property int selectedWeeksCount: 4
    property bool isStrictSchedule: true
    property double minRatingThreshold: 0.0

    // Selected preferred weekly slots: list of objects { day: 0..6, slot: 0..7 }
    property var selectedSlots: []

    function isSlotSelected(day, slot) {
        for (var i = 0; i < selectedSlots.length; ++i) {
            if (selectedSlots[i].day === day && selectedSlots[i].slot === slot) return true;
        }
        return false;
    }

    function toggleSlot(day, slot) {
        var copy = selectedSlots.slice();
        var foundIdx = -1;
        for (var i = 0; i < copy.length; ++i) {
            if (copy[i].day === day && copy[i].slot === slot) {
                foundIdx = i;
                break;
            }
        }
        if (foundIdx >= 0) {
            copy.splice(foundIdx, 1);
        } else {
            copy.push({ day: day, slot: slot });
        }
        selectedSlots = copy;
    }

    function clearSelectedSlots() {
        selectedSlots = [];
    }

    function applyPresetSlots(presetType) {
        var copy = [];
        if (presetType === "evening_mwf") {
            // Thứ 2, 4, 6 - Ca 7 (18:45 - 20:15)
            copy.push({ day: 0, slot: 6 });
            copy.push({ day: 2, slot: 6 });
            copy.push({ day: 4, slot: 6 });
        } else if (presetType === "evening_tts") {
            // Thứ 3, 5, 7 - Ca 7 (18:45 - 20:15)
            copy.push({ day: 1, slot: 6 });
            copy.push({ day: 3, slot: 6 });
            copy.push({ day: 5, slot: 6 });
        } else if (presetType === "weekend_morning") {
            // Thứ 7, CN - Ca 2 (09:15 - 10:45)
            copy.push({ day: 5, slot: 1 });
            copy.push({ day: 6, slot: 1 });
        }
        selectedSlots = copy;
    }

    function executeSmartMatch() {
        if (typeof guiBridge === "undefined") return;
        isRefreshing = true;
        var budgetVal = parseFloat(budgetField.text) || 200000.0;
        if (budgetVal <= 0.0) budgetVal = 200000.0;
        var criteriaMap = {
            "subject": subjectCombo.currentValue || selectedSubject,
            "gradeLevel": gradeCombo.currentValue,
            "maxBudget": budgetVal,
            "selectedSlots": selectedSlots,
            "startDate": startDateField.text.trim() || courseStartDate,
            "weeksCount": weeksCombo.currentValue || selectedWeeksCount,
            "strictSchedule": strictSwitch.checked,
            "minRating": ratingCombo.currentValue || 0.0
        };

        matchesList = guiBridge.qmlSearchSmartMatches(criteriaMap, 5);
        isRefreshing = false;
    }

    function resetCriteriaToDefault() {
        selectedSubject = "Toán";
        subjectCombo.currentIndex = 0;
        if (studentProfile && studentProfile.gradeLevel > 0) {
            gradeCombo.currentIndex = studentProfile.gradeLevel - 1;
        } else {
            gradeCombo.currentIndex = 9; // Lớp 10
        }
        budgetField.text = "200000";
        startDateField.text = "2026-11-16";
        weeksCombo.currentIndex = 0; // 4 tuần
        strictSwitch.checked = true;
        ratingCombo.currentIndex = 0; // Tất cả
        selectedSlots = [];
        executeSmartMatch();
    }

    Component.onCompleted: {
        // Pre-select evening slot as a convenient initial search preset
        selectedSlots = [{ day: 0, slot: 6 }, { day: 2, slot: 6 }];
        executeSmartMatch();
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 16

            // 1. Header Banner
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    spacing: 4
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0

                    Text {
                        text: "GỢI Ý GIA SƯ THÔNG MINH (SMARTMATCH CRITERIA-BASED ENGINE)"
                        font.bold: true
                        font.pixelSize: 18
                        color: Theme.textDark
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        text: "Thu thập tiêu chí tìm kiếm độc lập theo từng lượt, lọc điều kiện cứng và xếp hạng theo thang điểm 100 minh bạch"
                        font.pixelSize: 12
                        color: Theme.textMuted
                        wrapMode: Text.WordWrap
                    }
                }

                Button {
                    text: "Đặt lại tiêu chí"
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.textDark
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 110
                        implicitHeight: 34
                        radius: 6
                        color: parent.hovered ? Theme.bgApp : Theme.cMistyRose
                        border.color: Theme.cardBorder
                    }
                    onClicked: studentSmartMatchView.resetCriteriaToDefault()
                }

                Button {
                    text: isRefreshing ? "Đang phân tích..." : "Tìm kiếm SmartMatch"
                    enabled: !isRefreshing
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.btnText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 160
                        implicitHeight: 34
                        radius: 6
                        color: parent.enabled ? (parent.hovered ? Theme.btnPrimaryHover : Theme.btnPrimary) : Theme.cardBorder
                    }
                    onClicked: studentSmartMatchView.executeSmartMatch()
                }
            }

            // 2. Collapsible Search Criteria Card (Organized into 3 Groups)
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: criteriaCol.implicitHeight + 28
                color: Theme.cardBg
                radius: 10
                border.color: Theme.cardBorder
                border.width: 1

                ColumnLayout {
                    id: criteriaCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 14
                    spacing: 12

                    // Title
                    Text {
                        text: "TIÊU CHÍ TÌM KIẾM CHO LƯỢT GHÉP NÀY (INDEPENDENT SEARCH CRITERIA)"
                        font.bold: true
                        font.pixelSize: 13
                        color: Theme.textHeading
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.cardBorder
                    }

                    // GROUP 1: TIÊU CHÍ BẮT BUỘC (Môn, Khối lớp, Ngân sách)
                    Text {
                        text: "Nhóm 1: Tiêu chí bắt buộc (Required matching criteria)"
                        font.bold: true
                        font.pixelSize: 12
                        color: Theme.textDark
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        // Target Subject
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text { text: "Môn học mục tiêu (*):"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                            ComboBox {
                                id: subjectCombo
                                Layout.fillWidth: true
                                height: 34
                                model: ["Toán", "Vật lý", "Hóa học", "Sinh học", "Tiếng Anh", "Ngữ văn", "Tin học", "Lịch sử - Địa lý"]
                                currentIndex: 0
                            }
                        }

                        // Target Grade
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text { text: "Khối lớp (*):"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                            ComboBox {
                                id: gradeCombo
                                Layout.fillWidth: true
                                height: 34
                                textRole: "text"
                                valueRole: "value"
                                model: [
                                    { text: "Lớp 1", value: 1 }, { text: "Lớp 2", value: 2 },
                                    { text: "Lớp 3", value: 3 }, { text: "Lớp 4", value: 4 },
                                    { text: "Lớp 5", value: 5 }, { text: "Lớp 6", value: 6 },
                                    { text: "Lớp 7", value: 7 }, { text: "Lớp 8", value: 8 },
                                    { text: "Lớp 9", value: 9 }, { text: "Lớp 10", value: 10 },
                                    { text: "Lớp 11", value: 11 }, { text: "Lớp 12", value: 12 }
                                ]
                                currentIndex: (studentProfile && studentProfile.gradeLevel >= 1 && studentProfile.gradeLevel <= 12) ? (studentProfile.gradeLevel - 1) : 9
                            }
                        }

                        // Max Budget
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text { text: "Học phí tối đa / buổi (VNĐ) (*):"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                            TextField {
                                id: budgetField
                                Layout.fillWidth: true
                                height: 34
                                text: "200000"
                                placeholderText: "Nhập ngân sách tối đa..."
                                validator: DoubleValidator { bottom: 1000; top: 10000000; decimals: 0 }
                                background: Rectangle {
                                    radius: 6
                                    color: Theme.bgApp
                                    border.color: budgetField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.cardBorder
                    }

                    // GROUP 2: YÊU CẦU THỜI GIAN & LỊCH HỌC
                    Text {
                        text: "Nhóm 2: Lịch học & Khóa học (Schedule & Course requirements — 7×8 Timetable)"
                        font.bold: true
                        font.pixelSize: 12
                        color: Theme.textDark
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        // Start Date
                        ColumnLayout {
                            Layout.preferredWidth: 160
                            spacing: 4
                            Text { text: "Ngày bắt đầu (YYYY-MM-DD):"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                            TextField {
                                id: startDateField
                                Layout.fillWidth: true
                                height: 34
                                text: studentSmartMatchView.courseStartDate
                                background: Rectangle {
                                    radius: 6
                                    color: Theme.bgApp
                                    border.color: startDateField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                }
                            }
                        }

                        // Course Duration
                        ColumnLayout {
                            Layout.preferredWidth: 140
                            spacing: 4
                            Text { text: "Thời lượng khóa học:"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                            ComboBox {
                                id: weeksCombo
                                Layout.fillWidth: true
                                height: 34
                                textRole: "text"
                                valueRole: "value"
                                model: [
                                    { text: "Khóa 4 tuần", value: 4 },
                                    { text: "Khóa 8 tuần", value: 8 },
                                    { text: "Khóa 12 tuần", value: 12 }
                                ]
                                currentIndex: 0
                            }
                        }

                        // Schedule Flexibility Mode
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text { text: "Chế độ lịch học:"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                            RowLayout {
                                spacing: 8
                                Switch {
                                    id: strictSwitch
                                    checked: true
                                }
                                Text {
                                    text: strictSwitch.checked ? "Nghiêm ngặt (Bắt buộc thỏa mãn 100% ca đã chọn)" : "Linh hoạt (Ưu tiên trùng khớp cao nhất, tính điểm tỷ lệ)"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: strictSwitch.checked ? "#cf1322" : "#096dd9"
                                }
                            }
                        }
                    }

                    // Interactive 7x8 Weekly Slot Selector
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Chọn các ca học mong muốn trong tuần (Đã chọn: " + studentSmartMatchView.selectedSlots.length + " ca):"
                                font.pixelSize: 11
                                font.bold: true
                                color: Theme.textDark
                            }
                            Item { Layout.fillWidth: true }
                            Button {
                                text: "Tối 2-4-6 (18:45)"
                                height: 26
                                onClicked: studentSmartMatchView.applyPresetSlots("evening_mwf")
                            }
                            Button {
                                text: "Tối 3-5-7 (18:45)"
                                height: 26
                                onClicked: studentSmartMatchView.applyPresetSlots("evening_tts")
                            }
                            Button {
                                text: "Cuối tuần sáng"
                                height: 26
                                onClicked: studentSmartMatchView.applyPresetSlots("weekend_morning")
                            }
                            Button {
                                text: "Xóa chọn ca"
                                height: 26
                                onClicked: studentSmartMatchView.clearSelectedSlots()
                            }
                        }

                        // 7-day compact slot chips
                        GridLayout {
                            columns: 7
                            Layout.fillWidth: true
                            rowSpacing: 4
                            columnSpacing: 4

                            // Headers
                            Repeater {
                                model: ["Thứ 2", "Thứ 3", "Thứ 4", "Thứ 5", "Thứ 6", "Thứ 7", "Chủ Nhật"]
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 22
                                    color: Theme.sidebarBg
                                    radius: 3
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        font.bold: true
                                        font.pixelSize: 10
                                        color: Theme.textLight
                                    }
                                }
                            }

                            // 8 Slots per Day (Rows 0 to 7)
                            Repeater {
                                model: 56 // 7 days * 8 slots
                                Rectangle {
                                    id: slotBtn
                                    property int dayIdx: index % 7
                                    property int slotIdx: Math.floor(index / 7)
                                    property bool isChosen: studentSmartMatchView.isSlotSelected(dayIdx, slotIdx)

                                    Layout.fillWidth: true
                                    height: 24
                                    radius: 3
                                    color: isChosen ? Theme.cBurgundy : (hoverMa.containsMouse ? Theme.cMistyRose : Theme.bgApp)
                                    border.color: isChosen ? Theme.cBurgundy : Theme.cardBorder
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: {
                                            var slotTimes = ["07:30", "09:15", "11:00", "13:30", "15:15", "17:00", "18:45", "20:30"];
                                            return "C" + (slotBtn.slotIdx + 1) + " · " + slotTimes[slotBtn.slotIdx];
                                        }
                                        font.pixelSize: 9
                                        font.bold: slotBtn.isChosen
                                        color: slotBtn.isChosen ? "#FFFFFF" : Theme.textDark
                                    }

                                    MouseArea {
                                        id: hoverMa
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            studentSmartMatchView.toggleSlot(slotBtn.dayIdx, slotBtn.slotIdx);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.cardBorder
                    }

                    // GROUP 3: TÙY CHỌN NÂNG CAO (Optional preferences)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 24

                        Text {
                            text: "Nhóm 3: Tùy chọn nâng cao (Optional preferences):"
                            font.bold: true
                            font.pixelSize: 11
                            color: Theme.textMuted
                        }

                        RowLayout {
                            spacing: 6
                            Text { text: "Đánh giá tối thiểu:"; font.pixelSize: 11; color: Theme.textMuted }
                            ComboBox {
                                id: ratingCombo
                                height: 30
                                textRole: "text"
                                valueRole: "value"
                                model: [
                                    { text: "Tất cả đánh giá", value: 0.0 },
                                    { text: "Từ 4.0 ⭐ trở lên", value: 4.0 },
                                    { text: "Từ 4.5 ⭐ trở lên", value: 4.5 },
                                    { text: "Từ 4.8 ⭐ trở lên", value: 4.8 }
                                ]
                                currentIndex: 0
                            }
                        }
                    }
                }
            }

            // 3. Search Results Header
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "KẾT QUẢ ĐỀ XUẤT XẾP HẠNG (" + studentSmartMatchView.matchesList.length + " GIA SƯ ĐẠT YÊU CẦU)"
                    font.bold: true
                    font.pixelSize: 14
                    color: Theme.textDark
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "Điểm tối đa 100đ: Lịch học (35đ) | Học phí (25đ) | Đánh giá (15đ) | Chuyên môn (15đ) | Uy tín (10đ)"
                    font.pixelSize: 11
                    color: Theme.textMuted
                }
            }

            // 4. Empty Results Alert Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 120
                radius: 8
                color: "#fffbe6"
                border.color: "#ffe58f"
                border.width: 1
                visible: studentSmartMatchView.matchesList.length === 0

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Không tìm thấy gia sư phù hợp với môn học bạn đã chọn."
                        font.bold: true
                        font.pixelSize: 14
                        color: "#d48806"
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Gợi ý: Thử tăng mức học phí tối đa / buổi, hoặc chuyển sang chế độ lịch học 'Linh hoạt', hoặc điều chỉnh ca học yêu cầu."
                        font.pixelSize: 12
                        color: Theme.textDark
                    }
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Đặt lại tiêu chí tìm kiếm"
                        onClicked: studentSmartMatchView.resetCriteriaToDefault()
                    }
                }
            }

            // 5. Results Cards Repeater
            Repeater {
                model: studentSmartMatchView.matchesList

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: cardInnerCol.implicitHeight + 24
                    color: Theme.cardBg
                    radius: 10
                    border.color: (modelData.rank === 1) ? "#52c41a" : Theme.cardBorder
                    border.width: (modelData.rank === 1) ? 2 : 1

                    ColumnLayout {
                        id: cardInnerCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 14
                        spacing: 10

                        // Top Row: Rank & Score Badge + Name & Rate
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            // Rank Badge
                            Rectangle {
                                width: 36
                                height: 36
                                radius: 18
                                color: (modelData.rank === 1) ? "#f6ffed" : ((modelData.rank === 2) ? "#e6f7ff" : Theme.bgApp)
                                border.color: (modelData.rank === 1) ? "#52c41a" : ((modelData.rank === 2) ? "#1890ff" : Theme.cardBorder)
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: "#" + modelData.rank
                                    font.bold: true
                                    font.pixelSize: 14
                                    color: (modelData.rank === 1) ? "#389e0d" : ((modelData.rank === 2) ? "#096dd9" : Theme.textDark)
                                }
                            }

                            // Avatar Initials
                            Rectangle {
                                width: 40
                                height: 40
                                radius: 20
                                color: Theme.sidebarBg
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.initials || "G"
                                    font.bold: true
                                    font.pixelSize: 16
                                    color: Theme.textLight
                                }
                            }

                            // Tutor Identity & Subject
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true

                                RowLayout {
                                    spacing: 8
                                    Text {
                                        text: modelData.fullName || modelData.tutorName
                                        font.bold: true
                                        font.pixelSize: 15
                                        color: Theme.textDark
                                    }
                                }

                                Text {
                                    text: "Chuyên môn: " + modelData.subject + "  |  Đánh giá: " + (modelData.rating ? modelData.rating.toFixed(1) : "5.0") + "⭐ (" + (modelData.totalReviews || 0) + " nhận xét)"
                                    font.pixelSize: 11
                                    color: Theme.textMuted
                                }
                            }

                            // Tuition per Session
                            ColumnLayout {
                                spacing: 2
                                Layout.alignment: Qt.AlignRight
                                Text {
                                    text: (modelData.hourlyRate ? modelData.hourlyRate.toLocaleString(Qt.locale("vi_VN"), 'f', 0) : "0") + " đ/buổi"
                                    font.bold: true
                                    font.pixelSize: 16
                                    color: "#d97706"
                                }
                                Text {
                                    text: "Học phí cố định"
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    Layout.alignment: Qt.AlignRight
                                }
                            }

                            // Total Composite Score Pill
                            Rectangle {
                                width: 115
                                height: 38
                                radius: 6
                                color: (modelData.compositeScore >= 80) ? "#f6ffed" : ((modelData.compositeScore >= 60) ? "#e6f7ff" : "#fffbe6")
                                border.color: (modelData.compositeScore >= 80) ? "#b7eb8f" : ((modelData.compositeScore >= 60) ? "#91d5ff" : "#ffe58f")
                                border.width: 1

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 0
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: modelData.compositeScore + " / 100"
                                        font.bold: true
                                        font.pixelSize: 13
                                        color: (modelData.compositeScore >= 80) ? "#389e0d" : ((modelData.compositeScore >= 60) ? "#096dd9" : "#d48806")
                                    }
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "ĐỘ PHÙ HỢP"
                                        font.pixelSize: 8
                                        font.bold: true
                                        color: Theme.textMuted
                                    }
                                }
                            }
                        }

                        // Detailed Score Breakdown Bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            radius: 4
                            color: Theme.bgApp
                            border.color: Theme.cardBorder
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10

                                Text {
                                    text: "Phân bổ điểm: Lịch học: " + (modelData.scheduleScore || 0) + "/35đ  ·  Học phí: " + (modelData.priceScore || 0) + "/25đ  ·  Đánh giá: " + (modelData.ratingScore || 0) + "/15đ  ·  Chuyên môn: " + (modelData.experienceScore || 0) + "/15đ  ·  Uy tín: " + (modelData.credibilityScore || 0) + "/10đ"
                                    font.pixelSize: 10
                                    color: Theme.textDark
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: "Khớp: " + (modelData.matchedSlotCount || 0) + "/" + (modelData.requestedSlotCount || 0) + " ca"
                                    font.bold: true
                                    font.pixelSize: 10
                                    color: (modelData.matchedSlotCount === modelData.requestedSlotCount && modelData.requestedSlotCount > 0) ? "#389e0d" : "#096dd9"
                                }
                            }
                        }

                        // Explainability Bullets (Phase E)
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            Repeater {
                                model: modelData.explanations || []
                                RowLayout {
                                    spacing: 6
                                    Text {
                                        text: "✓"
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: "#2b6cb0"
                                    }
                                    Text {
                                        text: modelData
                                        font.pixelSize: 11
                                        color: Theme.textDark
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }

                        // Matched Slots Chips
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            visible: modelData.matchedSlots && modelData.matchedSlots.length > 0

                            Text {
                                text: "Các ca rảnh phù hợp:"
                                font.pixelSize: 10
                                font.bold: true
                                color: Theme.textMuted
                            }

                            Repeater {
                                model: modelData.matchedSlots || []
                                Rectangle {
                                    height: 20
                                    implicitWidth: slotLabelTxt.implicitWidth + 12
                                    radius: 3
                                    color: "#e6f7ff"
                                    border.color: "#91d5ff"
                                    border.width: 1

                                    Text {
                                        id: slotLabelTxt
                                        anchors.centerIn: parent
                                        text: modelData.label || ""
                                        font.pixelSize: 9
                                        color: "#096dd9"
                                    }
                                }
                            }
                        }

                        // Action Buttons Row
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Item { Layout.fillWidth: true }

                            Button {
                                text: "Xem hồ sơ gia sư"
                                implicitWidth: 130
                                implicitHeight: 34
                                contentItem: Text {
                                    text: parent.text
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: Theme.textDark
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    radius: 6
                                    color: parent.hovered ? Theme.bgApp : Theme.cMistyRose
                                    border.color: Theme.cardBorder
                                }
                                onClicked: tutorDetailModal.openWithTutorId(modelData.tutorId)
                            }

                            Button {
                                text: "Đăng ký học ngay"
                                implicitWidth: 140
                                implicitHeight: 34
                                contentItem: Text {
                                    text: parent.text
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: Theme.btnText
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    radius: 6
                                    color: parent.hovered ? Theme.btnPrimaryHover : Theme.sidebarBg
                                }
                                onClicked: {
                                    bookingModal.openWithTutor(modelData);
                                }
                            }
                        }
                    }
                }
            }

            // Bottom Spacer
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
            }
        }
    }

    // Modals Integration
    TutorDetailModal {
        id: tutorDetailModal
        onBookCourseRequested: function(tutor) {
            bookingModal.openWithTutor(tutor)
        }
    }

    BookingModal {
        id: bookingModal
        onBookingCompleted: {
            studentSmartMatchView.executeSmartMatch()
        }
    }
}
