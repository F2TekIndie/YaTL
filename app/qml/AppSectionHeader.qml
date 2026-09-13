import QtQuick
import QtQuick.Controls

Row { property string title: ""; property string subtitle: ""; spacing: AppTheme.space2; height: Math.max(titleText.implicitHeight, subtitleText.implicitHeight); Text { id: titleText; text: parent.title; color: AppTheme.onSurface; font.pixelSize: AppTheme.titleSize; font.bold: true } Text { id: subtitleText; text: parent.subtitle; color: AppTheme.onSurfaceVariant; font.pixelSize: AppTheme.bodySize; anchors.bottom: titleText.bottom } }
