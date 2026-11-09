import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Item {
    id: loginView
    signal loginSuccess()

    property bool isRegisterMode: false
    property bool isStudentRegistration: true
    property string localErrorMessage: ""
    property int selectedGradeLevel: 10

    Rectangle {
        anchors.fill: parent
        color: Theme.bgApp

        // Centered Auth Card
        Rectangle {
            id: authCard
            width: 480
            height: isRegisterMode ? Math.min(630, parent.height - 30) : Math.min(authCardColumn.implicitHeight + 48, parent.height - 30)
            anchors.centerIn: parent
            radius: 16
            color: Theme.cardBg
            border.color: Theme.cardBorder
            border.width: 1

            Behavior on height {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.InOutQuad
                }
            }

            ColumnLayout {
                id: authCardColumn
                anchors.fill: parent
                anchors.margins: 24
                spacing: 12

                // Header Branding
                RowLayout {
                    spacing: 12
                    Rectangle {
                        width: 40
                        height: 40
                        radius: 8
                        color: Theme.sidebarBg
                        Text {
                            anchors.centerIn: parent
                            text: "TM"
                            font.bold: true
                            font.pixelSize: 18
                            color: Theme.textLight
                        }
                    }
                    ColumnLayout {
                        spacing: 2
                        Text {
                            text: "TutorMatch Desktop"
                            font.bold: true
                            font.pixelSize: 18
                            color: Theme.textDark
                        }
                        Text {
                            text: "Nền Tảng Gợi Ý & Kết Nối Gia Sư"
                            font.pixelSize: 12
                            color: Theme.textMuted
                        }
                    }
                }

                // Mode Selector Bar (Segmented Control: Đăng nhập vs Đăng ký)
                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    radius: 8
                    color: Theme.bgApp
                    border.color: Theme.cardBorder
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 3
                        spacing: 4

                        // Button: Đăng nhập
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 6
                            color: !loginView.isRegisterMode ? Theme.sidebarBg : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: "Đăng nhập"
                                font.bold: true
                                font.pixelSize: 13
                                color: !loginView.isRegisterMode ? Theme.textLight : Theme.textDark
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    loginView.isRegisterMode = false
                                    loginView.localErrorMessage = ""
                                    if (typeof guiBridge !== "undefined") guiBridge.clearStatusMessage()
                                }
                            }
                        }

                        // Button: Đăng ký
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 6
                            color: loginView.isRegisterMode ? Theme.sidebarBg : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: "Đăng ký"
                                font.bold: true
                                font.pixelSize: 13
                                color: loginView.isRegisterMode ? Theme.textLight : Theme.textDark
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    loginView.isRegisterMode = true
                                    loginView.localErrorMessage = ""
                                    if (typeof guiBridge !== "undefined") guiBridge.clearStatusMessage()
                                }
                            }
                        }
                    }
                }

                // Status Banner Notification
                Rectangle {
                    Layout.fillWidth: true
                    height: 34
                    radius: 6
                    color: loginView.localErrorMessage !== "" ? "#fff1f0" : Theme.accentBlue
                    border.color: loginView.localErrorMessage !== "" ? "#ffa39e" : Theme.cardBorder
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 20
                        text: loginView.localErrorMessage !== "" ? loginView.localErrorMessage :
                              ((typeof guiBridge !== "undefined" && guiBridge.statusMessage !== "") ? guiBridge.statusMessage :
                               (loginView.isRegisterMode ? "Vui lòng điền thông tin để tạo tài khoản" : "Vui lòng nhập tài khoản để đăng nhập"))
                        font.pixelSize: 11
                        font.bold: true
                        color: loginView.localErrorMessage !== "" ? "#cf1322" : Theme.textDark
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // =========================================================
                // MODE 1: LOGIN FORM
                // =========================================================
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    visible: !loginView.isRegisterMode

                    ColumnLayout {
                        spacing: 4
                        Text {
                            text: "Tên đăng nhập:"
                            font.pixelSize: 12
                            font.bold: true
                            color: Theme.textDark
                        }
                        TextField {
                            id: usernameField
                            Layout.fillWidth: true
                            height: 38
                            text: "student_nam"
                            color: Theme.textDark
                            placeholderText: "Nhập tên đăng nhập..."
                            background: Rectangle {
                                color: Theme.bgApp
                                radius: 6
                                border.color: usernameField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                border.width: 1
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Text {
                            text: "Mật khẩu bảo mật:"
                            font.pixelSize: 12
                            font.bold: true
                            color: Theme.textDark
                        }
                        TextField {
                            id: passwordField
                            Layout.fillWidth: true
                            height: 38
                            text: "pass123"
                            echoMode: TextInput.Password
                            color: Theme.textDark
                            placeholderText: "Nhập mật khẩu..."
                            background: Rectangle {
                                color: Theme.bgApp
                                radius: 6
                                border.color: passwordField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                border.width: 1
                            }
                        }
                    }

                    // Quick Preset Chips for Debug
                    Text {
                        text: "Đăng nhập nhanh mẫu:"
                        font.pixelSize: 11
                        color: Theme.textMuted
                    }
                    RowLayout {
                        spacing: 8
                        Button {
                            text: "1. Học viên"
                            contentItem: Text {
                                text: parent.text
                                font.bold: true
                                font.pixelSize: 11
                                color: Theme.textDark
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                implicitWidth: 95
                                implicitHeight: 30
                                radius: 6
                                color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                                border.color: Theme.cardBorder
                            }
                            onClicked: {
                                usernameField.text = "student_nam"
                                passwordField.text = "pass123"
                            }
                        }
                        Button {
                            text: "2. Gia sư"
                            contentItem: Text {
                                text: parent.text
                                font.bold: true
                                font.pixelSize: 11
                                color: Theme.textDark
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                implicitWidth: 95
                                implicitHeight: 30
                                radius: 6
                                color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                                border.color: Theme.cardBorder
                            }
                            onClicked: {
                                usernameField.text = "tutor_math"
                                passwordField.text = "pass123"
                            }
                        }
                        Button {
                            text: "3. Admin"
                            contentItem: Text {
                                text: parent.text
                                font.bold: true
                                font.pixelSize: 11
                                color: Theme.textDark
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                implicitWidth: 95
                                implicitHeight: 30
                                radius: 6
                                color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                                border.color: Theme.cardBorder
                            }
                            onClicked: {
                                usernameField.text = "admin"
                                passwordField.text = "admin123"
                            }
                        }
                    }

                    // Login Submit Button
                    Button {
                        Layout.fillWidth: true
                        height: 42
                        contentItem: Text {
                            text: "ĐĂNG NHẬP HỆ THỐNG"
                            font.bold: true
                            font.pixelSize: 13
                            color: Theme.btnText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Theme.sidebarBg : Theme.btnPrimary
                            radius: 8
                        }
                        onClicked: {
                            loginView.localErrorMessage = ""
                            if (typeof guiBridge !== "undefined") {
                                if (guiBridge.qmlLogin(usernameField.text, passwordField.text)) {
                                    loginView.loginSuccess()
                                }
                            } else {
                                loginView.loginSuccess()
                            }
                        }
                    }
                }

                // =========================================================
                // MODE 2: REGISTRATION FORM
                // =========================================================
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10
                    visible: loginView.isRegisterMode

                    // Role Selector Bar (Học viên vs Gia sư)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "Vai trò:"
                            font.pixelSize: 12
                            font.bold: true
                            color: Theme.textDark
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 34
                            radius: 6
                            color: Theme.bgApp
                            border.color: Theme.cardBorder
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 2
                                spacing: 2

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 4
                                    color: loginView.isStudentRegistration ? Theme.sidebarBg : "transparent"
                                    Text {
                                        anchors.centerIn: parent
                                        text: "Học viên"
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: loginView.isStudentRegistration ? Theme.textLight : Theme.textDark
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            loginView.isStudentRegistration = true
                                            loginView.localErrorMessage = ""
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 4
                                    color: !loginView.isStudentRegistration ? Theme.sidebarBg : "transparent"
                                    Text {
                                        anchors.centerIn: parent
                                        text: "Gia sư"
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: !loginView.isStudentRegistration ? Theme.textLight : Theme.textDark
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            loginView.isStudentRegistration = false
                                            loginView.localErrorMessage = ""
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Form Fields in ScrollView
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: availableWidth

                        ColumnLayout {
                            width: parent.width
                            spacing: 8

                            // Full Name
                            ColumnLayout {
                                spacing: 2
                                Text {
                                    text: "Họ và tên:"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }
                                TextField {
                                    id: regFullNameField
                                    Layout.fillWidth: true
                                    height: 34
                                    placeholderText: "Ví dụ: Nguyễn Văn An"
                                    color: Theme.textDark
                                    background: Rectangle {
                                        color: Theme.bgApp
                                        radius: 6
                                        border.color: regFullNameField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                        border.width: 1
                                    }
                                }
                            }

                            // Username
                            ColumnLayout {
                                spacing: 2
                                Text {
                                    text: "Tên đăng nhập:"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }
                                TextField {
                                    id: regUsernameField
                                    Layout.fillWidth: true
                                    height: 34
                                    placeholderText: "Ví dụ: student_an hoặc tutor_an"
                                    color: Theme.textDark
                                    background: Rectangle {
                                        color: Theme.bgApp
                                        radius: 6
                                        border.color: regUsernameField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                        border.width: 1
                                    }
                                }
                            }

                            // Email
                            ColumnLayout {
                                spacing: 2
                                Text {
                                    text: "Địa chỉ Email:"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }
                                TextField {
                                    id: regEmailField
                                    Layout.fillWidth: true
                                    height: 34
                                    placeholderText: "Ví dụ: an.nguyen@gmail.com"
                                    color: Theme.textDark
                                    background: Rectangle {
                                        color: Theme.bgApp
                                        radius: 6
                                        border.color: regEmailField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                        border.width: 1
                                    }
                                }
                            }

                            // Password
                            ColumnLayout {
                                spacing: 2
                                Text {
                                    text: "Mật khẩu (tối thiểu 6 ký tự):"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }
                                TextField {
                                    id: regPasswordField
                                    Layout.fillWidth: true
                                    height: 34
                                    echoMode: TextInput.Password
                                    placeholderText: "Nhập mật khẩu..."
                                    color: Theme.textDark
                                    background: Rectangle {
                                        color: Theme.bgApp
                                        radius: 6
                                        border.color: regPasswordField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                        border.width: 1
                                    }
                                }
                            }

                            // Confirm Password
                            ColumnLayout {
                                spacing: 2
                                Text {
                                    text: "Xác nhận mật khẩu:"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }
                                TextField {
                                    id: regConfirmPasswordField
                                    Layout.fillWidth: true
                                    height: 34
                                    echoMode: TextInput.Password
                                    placeholderText: "Nhập lại mật khẩu..."
                                    color: Theme.textDark
                                    background: Rectangle {
                                        color: Theme.bgApp
                                        radius: 6
                                        border.color: regConfirmPasswordField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                        border.width: 1
                                    }
                                }
                            }

                            // Student-specific: Grade Level (10, 11, 12, Khác -> 0)
                            ColumnLayout {
                                visible: loginView.isStudentRegistration
                                spacing: 4
                                Text {
                                    text: "Khối lớp học tập:"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }
                                RowLayout {
                                    spacing: 6
                                    Repeater {
                                        model: [
                                            { label: "Lớp 10", val: 10 },
                                            { label: "Lớp 11", val: 11 },
                                            { label: "Lớp 12", val: 12 },
                                            { label: "Khác", val: 0 }
                                        ]
                                        Button {
                                            text: modelData.label
                                            contentItem: Text {
                                                text: parent.text
                                                font.bold: true
                                                font.pixelSize: 11
                                                color: (loginView.selectedGradeLevel === modelData.val) ? Theme.textLight : Theme.textDark
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            background: Rectangle {
                                                implicitWidth: 70
                                                implicitHeight: 30
                                                radius: 6
                                                color: (loginView.selectedGradeLevel === modelData.val) ? Theme.sidebarBg : Theme.bgApp
                                                border.color: Theme.cardBorder
                                            }
                                            onClicked: loginView.selectedGradeLevel = modelData.val
                                        }
                                    }
                                }
                            }

                            // Tutor-specific: Subject Specialty
                            ColumnLayout {
                                visible: !loginView.isStudentRegistration
                                spacing: 2
                                Text {
                                    text: "Môn học / Chuyên môn giảng dạy:"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }
                                TextField {
                                    id: regSubjectField
                                    Layout.fillWidth: true
                                    height: 34
                                    placeholderText: "Ví dụ: Toán 12, Vật lý 12, Tiếng Anh..."
                                    color: Theme.textDark
                                    background: Rectangle {
                                        color: Theme.bgApp
                                        radius: 6
                                        border.color: regSubjectField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                        border.width: 1
                                    }
                                }
                            }

                            // Tutor-specific: Hourly Rate
                            ColumnLayout {
                                visible: !loginView.isStudentRegistration
                                spacing: 2
                                Text {
                                    text: "Học phí mong muốn (VNĐ/buổi):"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }
                                TextField {
                                    id: regHourlyRateField
                                    Layout.fillWidth: true
                                    height: 34
                                    placeholderText: "Ví dụ: 150000"
                                    color: Theme.textDark
                                    inputMethodHints: Qt.ImhDigitsOnly
                                    background: Rectangle {
                                        color: Theme.bgApp
                                        radius: 6
                                        border.color: regHourlyRateField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                        border.width: 1
                                    }
                                }
                            }

                            // Tutor-specific: Bio
                            ColumnLayout {
                                visible: !loginView.isStudentRegistration
                                spacing: 2
                                Text {
                                    text: "Giới thiệu bản thân & Kinh nghiệm:"
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: Theme.textDark
                                }
                                TextField {
                                    id: regBioField
                                    Layout.fillWidth: true
                                    height: 48
                                    placeholderText: "Giới thiệu ngắn về trình độ và kinh nghiệm..."
                                    color: Theme.textDark
                                    background: Rectangle {
                                        color: Theme.bgApp
                                        radius: 6
                                        border.color: regBioField.activeFocus ? Theme.sidebarBg : Theme.cardBorder
                                        border.width: 1
                                    }
                                }
                            }
                        }
                    }

                    // Registration Submit Button
                    Button {
                        Layout.fillWidth: true
                        height: 40
                        contentItem: Text {
                            text: loginView.isStudentRegistration ? "ĐĂNG KÝ HỌC VIÊN" : "ĐĂNG KÝ GIA SƯ"
                            font.bold: true
                            font.pixelSize: 13
                            color: Theme.btnText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Theme.sidebarBg : Theme.btnPrimary
                            radius: 8
                        }
                        onClicked: {
                            loginView.localErrorMessage = ""

                            // 1. Common Validation
                            var fn = regFullNameField.text.trim()
                            var un = regUsernameField.text.trim()
                            var em = regEmailField.text.trim()
                            var pw = regPasswordField.text
                            var cpw = regConfirmPasswordField.text

                            if (fn === "") {
                                loginView.localErrorMessage = "⚠ Vui lòng nhập Họ và tên!"
                                return
                            }
                            if (un === "") {
                                loginView.localErrorMessage = "⚠ Vui lòng nhập Tên đăng nhập!"
                                return
                            }
                            if (em === "" || em.indexOf("@") === -1 || em.indexOf(".") === -1) {
                                loginView.localErrorMessage = "⚠ Vui lòng nhập địa chỉ Email hợp lệ!"
                                return
                            }
                            if (pw.length < 6) {
                                loginView.localErrorMessage = "⚠ Mật khẩu phải có ít nhất 6 ký tự!"
                                return
                            }
                            if (pw !== cpw) {
                                loginView.localErrorMessage = "⚠ Mật khẩu xác nhận không khớp!"
                                return
                            }

                            if (typeof guiBridge === "undefined") {
                                loginView.localErrorMessage = "⚠ Lỗi kết nối GUIBridge!"
                                return
                            }

                            // 2. Role-specific Submit
                            if (loginView.isStudentRegistration) {
                                var okS = guiBridge.qmlRegisterStudent(un, pw, fn, em, loginView.selectedGradeLevel, "", 0.0)
                                if (okS) {
                                    // Reset form fields
                                    regFullNameField.text = ""
                                    regUsernameField.text = ""
                                    regEmailField.text = ""
                                    regPasswordField.text = ""
                                    regConfirmPasswordField.text = ""

                                    // Switch to Login Mode and pre-fill username
                                    loginView.isRegisterMode = false
                                    usernameField.text = un
                                    passwordField.text = ""
                                    loginView.localErrorMessage = ""
                                }
                            } else {
                                var subj = regSubjectField.text.trim()
                                var rateStr = regHourlyRateField.text.trim()
                                var bio = regBioField.text.trim()

                                if (subj === "") {
                                    loginView.localErrorMessage = "⚠ Vui lòng nhập Môn học / Chuyên môn!"
                                    return
                                }
                                var rateVal = parseFloat(rateStr)
                                if (isNaN(rateVal) || rateVal <= 0) {
                                    loginView.localErrorMessage = "⚠ Vui lòng nhập học phí hợp lệ (> 0 VNĐ)!"
                                    return
                                }
                                if (bio === "") {
                                    loginView.localErrorMessage = "⚠ Vui lòng nhập Giới thiệu bản thân!"
                                    return
                                }

                                var okT = guiBridge.qmlRegisterTutor(un, pw, fn, em, subj, rateVal, bio)
                                if (okT) {
                                    // Reset form fields
                                    regFullNameField.text = ""
                                    regUsernameField.text = ""
                                    regEmailField.text = ""
                                    regPasswordField.text = ""
                                    regConfirmPasswordField.text = ""
                                    regSubjectField.text = ""
                                    regHourlyRateField.text = ""
                                    regBioField.text = ""

                                    // Switch to Login Mode and pre-fill username
                                    loginView.isRegisterMode = false
                                    usernameField.text = un
                                    passwordField.text = ""
                                    loginView.localErrorMessage = ""
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
