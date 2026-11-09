import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: profileEditModal
    anchors.fill: parent
    visible: false
    z: 999

    property bool isTutor: false
    property bool isSaving: false
    property var profileData: ({})
    property string errorMessage: ""

    function openWithProfile(data) {
        profileData = data || {}
        errorMessage = ""
        isSaving = false

        if (isTutor) {
            var rawFullName = data.fullName || ""
            var recognizedTitles = ["ThS.", "TS.", "ThS", "TS", "Thầy", "Cô", "Ms.", "Mr.", "Cử nhân", "Kỹ sư", "PGS.TS.", "GS.TS."]
            var foundTitle = ""
            var cleanName = rawFullName
            for (var i = 0; i < recognizedTitles.length; i++) {
                var prefix = recognizedTitles[i]
                if (rawFullName.indexOf(prefix) === 0) {
                    foundTitle = prefix
                    cleanName = rawFullName.substring(prefix.length).trim()
                    break
                }
            }
            titleInput.text = foundTitle
            nameInput.text = cleanName
            emailInput.text = data.email || ""
            subjectInput.text = data.subjectSpecialty || ""
            rateInput.text = (data.hourlyRate || 150000).toString()
            bioInput.text = data.bio || ""
        } else {
            nameInput.text = data.fullName || ""
            emailInput.text = data.email || ""
            gradeInput.text = (data.gradeLevel || 10).toString()
            subjectInput.text = data.preferredSubject || ""
            budgetInput.text = (data.maxBudget || 200000).toString()
        }

        profileEditModal.visible = true
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
        width: 520
        height: isTutor ? 590 : 540
        anchors.centerIn: parent
        radius: 12
        color: Theme.cardBg
        border.color: Theme.cardBorder
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 12

            // Header
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: isTutor ? "CHỈNH SỬA HỒ SƠ GIA SƯ" : "CHỈNH SỬA HỒ SƠ HỌC VIÊN"
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
                    onClicked: profileEditModal.visible = false
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.cardBorder
            }

            // Validation Error Banner
            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: 6
                color: "#fff1f0"
                border.color: "#ff4d4f"
                border.width: 1
                visible: profileEditModal.errorMessage !== ""

                Text {
                    anchors.centerIn: parent
                    text: profileEditModal.errorMessage
                    font.pixelSize: 12
                    font.bold: true
                    color: "#cf1322"
                }
            }

            // Form Inputs Grid
            GridLayout {
                columns: 2
                rowSpacing: 9
                columnSpacing: 16
                Layout.fillWidth: true

                // Read-only User ID
                Text {
                    text: isTutor ? "Mã ID Gia sư:" : "Mã ID Học viên:"
                    font.bold: true
                    color: Theme.textMuted
                }
                TextField {
                    text: (profileData.id || 0).toString()
                    readOnly: true
                    enabled: false
                    Layout.fillWidth: true
                    color: Theme.textMuted
                    background: Rectangle { color: "#f0f0f0"; radius: 6; border.color: Theme.cardBorder }
                }

                // Read-only Username
                Text { text: "Tên đăng nhập:"; font.bold: true; color: Theme.textMuted }
                TextField {
                    text: profileData.username || ""
                    readOnly: true
                    enabled: false
                    Layout.fillWidth: true
                    color: Theme.textMuted
                    background: Rectangle { color: "#f0f0f0"; radius: 6; border.color: Theme.cardBorder }
                }

                // Field: Academic Title / Degree (Tutor only)
                Text {
                    visible: isTutor
                    text: "Học vị / Danh xưng:"
                    font.bold: true
                    color: Theme.textDark
                }
                TextField {
                    id: titleInput
                    visible: isTutor
                    Layout.fillWidth: true
                    color: Theme.textDark
                    placeholderText: "Ví dụ: ThS., TS., Thầy, Cô, Ms., Mr...."
                    background: Rectangle { color: Theme.bgApp; radius: 6; border.color: titleInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder }
                }

                // Editable Full Name
                Text { text: "Họ và Tên (*):"; font.bold: true; color: Theme.textDark }
                TextField {
                    id: nameInput
                    Layout.fillWidth: true
                    color: Theme.textDark
                    placeholderText: isTutor ? "Nhập họ và tên gia sư..." : "Nhập họ và tên học viên..."
                    background: Rectangle { color: Theme.bgApp; radius: 6; border.color: nameInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder }
                }

                // Editable Email
                Text { text: "Email liên hệ (*):"; font.bold: true; color: Theme.textDark }
                TextField {
                    id: emailInput
                    Layout.fillWidth: true
                    color: Theme.textDark
                    placeholderText: "Nhập địa chỉ email..."
                    background: Rectangle { color: Theme.bgApp; radius: 6; border.color: emailInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder }
                }

                // Student: Grade Level
                Text {
                    visible: !isTutor
                    text: "Khối Lớp (1-12) (*):"
                    font.bold: true
                    color: Theme.textDark
                }
                TextField {
                    id: gradeInput
                    visible: !isTutor
                    Layout.fillWidth: true
                    color: Theme.textDark
                    placeholderText: "Nhập khối lớp (ví dụ: 10)..."
                    validator: IntValidator { bottom: 1; top: 12 }
                    background: Rectangle { color: Theme.bgApp; radius: 6; border.color: gradeInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder }
                }

                // Subject: Tutor Specialty or Student Target
                Text {
                    text: isTutor ? "Môn chuyên dạy (*):" : "Môn Học Mục Tiêu (*):"
                    font.bold: true
                    color: Theme.textDark
                }
                TextField {
                    id: subjectInput
                    Layout.fillWidth: true
                    color: Theme.textDark
                    placeholderText: isTutor ? "Nhập môn giảng dạy (vd: Toán 12 - Luyện thi)..." : "Nhập môn học mục tiêu..."
                    background: Rectangle { color: Theme.bgApp; radius: 6; border.color: subjectInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder }
                }

                // Student: Max Budget
                Text {
                    visible: !isTutor
                    text: "Học Phí Tối Đa (VNĐ) (*):"
                    font.bold: true
                    color: Theme.textDark
                }
                TextField {
                    id: budgetInput
                    visible: !isTutor
                    Layout.fillWidth: true
                    color: Theme.textDark
                    placeholderText: "Nhập mức ngân sách..."
                    validator: DoubleValidator { bottom: 1000; top: 10000000; decimals: 0 }
                    background: Rectangle { color: Theme.bgApp; radius: 6; border.color: budgetInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder }
                }

                // Tutor: Hourly Rate
                Text {
                    visible: isTutor
                    text: "Học phí (VNĐ / buổi) (*):"
                    font.bold: true
                    color: Theme.textDark
                }
                TextField {
                    id: rateInput
                    visible: isTutor
                    Layout.fillWidth: true
                    color: Theme.textDark
                    placeholderText: "Nhập học phí mỗi buổi (vd: 150000)..."
                    validator: DoubleValidator { bottom: 1000; top: 50000000; decimals: 0 }
                    background: Rectangle { color: Theme.bgApp; radius: 6; border.color: rateInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder }
                }

                // Tutor: Short Bio
                Text {
                    visible: isTutor
                    text: "Giới thiệu ngắn:"
                    font.bold: true
                    color: Theme.textDark
                }
                TextField {
                    id: bioInput
                    visible: isTutor
                    Layout.fillWidth: true
                    color: Theme.textDark
                    placeholderText: "Kinh nghiệm, thành tích giảng dạy..."
                    background: Rectangle { color: Theme.bgApp; radius: 6; border.color: bioInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder }
                }
            }

            Item { Layout.fillHeight: true }

            // Action Buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Button {
                    text: "Hủy bỏ"
                    Layout.fillWidth: true
                    height: 40
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
                    onClicked: profileEditModal.visible = false
                }

                Button {
                    id: saveBtn
                    text: isSaving ? "Đang lưu..." : "Lưu thay đổi"
                    enabled: !isSaving
                    Layout.fillWidth: true
                    height: 40
                    contentItem: Text {
                        text: saveBtn.text
                        font.bold: true
                        color: Theme.btnText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? Theme.sidebarBg : Theme.btnPrimary) : Theme.cardBorder
                        radius: 6
                    }
                    onClicked: {
                        if (isSaving) return;
                        isSaving = true;

                        if (typeof guiBridge !== "undefined") {
                            if (isTutor) {
                                var rateVal = parseFloat(rateInput.text) || 0.0;
                                var ok = guiBridge.updateTutorProfile(titleInput.text, nameInput.text, emailInput.text, bioInput.text, subjectInput.text, rateVal);
                                if (ok) {
                                    profileEditModal.visible = false;
                                } else {
                                    profileEditModal.errorMessage = guiBridge.statusMessage || "Lỗi cập nhật hồ sơ gia sư!";
                                }
                            } else {
                                var gradeVal = parseInt(gradeInput.text) || 0;
                                var budgetVal = parseFloat(budgetInput.text) || 0.0;
                                var ok = guiBridge.updateStudentProfile(nameInput.text, emailInput.text, gradeVal, subjectInput.text, budgetVal);
                                if (ok) {
                                    profileEditModal.visible = false;
                                } else {
                                    profileEditModal.errorMessage = guiBridge.statusMessage || "Lỗi cập nhật hồ sơ học viên!";
                                }
                            }
                        } else {
                            profileEditModal.visible = false;
                        }

                        isSaving = false;
                    }
                }
            }
        }
    }
}
