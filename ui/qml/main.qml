import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "TutorMatch - Nền Tảng Gợi Ý & Kết Nối Gia Sư Thông Minh (Qt 6.11 QML)"

    color: Theme.bgApp

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: loginViewComponent

        Component {
            id: loginViewComponent
            LoginView {
                onLoginSuccess: {
                    stackView.replace(mainDashboardComponent)
                }
            }
        }

        Component {
            id: mainDashboardComponent
            MainDashboard {
                onLogoutRequested: {
                    if (typeof guiBridge !== "undefined") {
                        guiBridge.qmlLogout()
                    }
                    stackView.replace(loginViewComponent)
                }
            }
        }
    }
}