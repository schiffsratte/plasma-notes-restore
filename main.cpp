#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrl>
#include <QWidget>
#include <algorithm>

struct DesktopContainment {
    int id = -1;
    int lastScreen = -1;
    QString activityId;
};

class RestoreWidget : public QWidget {
public:
    RestoreWidget() {
        setWindowTitle("Plasma Notes Restore");
        auto *layout = new QVBoxLayout(this);
        m_info = new QLabel(this);
        m_target = new QComboBox(this);
        m_button = new QPushButton("restore lost Plasma Notes", this);
        m_openConfigButton = new QPushButton("open ~/.config  (open file plasma-org.kde.plasma.desktop-appletsrc)", this);
        m_openNotesButton = new QPushButton("open plasma_notes folder", this);
        m_status = new QLabel("Ready", this);

        auto *folderButtons = new QHBoxLayout;
        folderButtons->addWidget(m_openConfigButton);
        folderButtons->addWidget(m_openNotesButton);

        layout->addWidget(m_info);
        layout->addWidget(m_target);
        layout->addWidget(m_button);
        layout->addLayout(folderButtons);
        layout->addWidget(m_status);

        connect(m_button, &QPushButton::clicked, this, [this] { restore(); });
        connect(m_openConfigButton, &QPushButton::clicked, this, [this] {
            openFolder(QDir::homePath() + "/.config");
        });
        connect(m_openNotesButton, &QPushButton::clicked, this, [this] {
            openFolder(notesDirPath());
        });
        reload();
        resize(460, 180);
    }

private:
    QString configPath() const {
        return QDir::homePath() + "/.config/plasma-org.kde.plasma.desktop-appletsrc";
    }

    QString notesDirPath() const {
        return QDir::homePath() + "/.local/share/plasma_notes";
    }

    void openFolder(const QString &path) {
        QDir dir(path);
        if (!dir.exists()) {
            QMessageBox::warning(this, "Directory not found",
                                 QString("The directory does not exist:\n%1").arg(path));
            return;
        }

        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
            QMessageBox::warning(this, "Error",
                                 QString("The directory could not be opened :\n%1").arg(path));
        }
    }

    QString readFile(const QString &path) const {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
        return QString::fromUtf8(f.readAll());
    }

    QStringList noteFiles() const {
        QDir dir(notesDirPath());
        if (!dir.exists()) return {};
        QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        files.removeDuplicates();
        return files;
    }

    QList<DesktopContainment> desktopContainments(const QString &text) const {
        QList<DesktopContainment> out;
        QRegularExpression re(R"(^\[Containments\]\[(\d+)\]\s*$)", QRegularExpression::MultilineOption);
        auto it = re.globalMatch(text);
        QVector<QPair<int,int>> sections;
        while (it.hasNext()) {
            auto m = it.next();
            sections.append({m.captured(1).toInt(), m.capturedStart()});
        }
        for (int i=0;i<sections.size();++i) {
            int start = sections[i].second;
            int end = (i+1<sections.size()) ? sections[i+1].second : text.size();
            QString block = text.mid(start, end-start);
            if (!block.contains("plugin=org.kde.plasma.folder")) continue;
            if (!block.contains("formfactor=0")) continue;
            DesktopContainment c;
            c.id = sections[i].first;
            QRegularExpression sRe(R"(^lastScreen=(-?\d+)\s*$)", QRegularExpression::MultilineOption);
            auto sm = sRe.match(block);
            if (sm.hasMatch()) c.lastScreen = sm.captured(1).toInt();
            QRegularExpression aRe(R"(^activityId=(.+)\s*$)", QRegularExpression::MultilineOption);
            auto am = aRe.match(block);
            if (am.hasMatch()) c.activityId = am.captured(1).trimmed();
            out.append(c);
        }
        return out;
    }

    QMap<int, QStringList> notesByContainment(const QString &text) const {
        QMap<int, QStringList> out;
        QRegularExpression re(
            R"(^\[Containments\]\[(\d+)\]\[Applets\]\[(\d+)\]\[Configuration\]\[General\]\s*$)",
            QRegularExpression::MultilineOption);
        auto it = re.globalMatch(text);
        QVector<QPair<QPair<int,int>,int>> sections;
        while (it.hasNext()) {
            auto m = it.next();
            sections.append({{m.captured(1).toInt(), m.captured(2).toInt()}, m.capturedStart()});
        }
        for (int i=0;i<sections.size();++i) {
            int start = sections[i].second;
            int end = text.indexOf(QRegularExpression(R"(^\[)", QRegularExpression::MultilineOption), start + 1);
            if (end < 0) end = text.size();
            QString block = text.mid(start, end-start);
            QRegularExpression nRe(R"(^noteId=(.*)\s*$)", QRegularExpression::MultilineOption);
            auto nm = nRe.match(block);
            if (nm.hasMatch()) {
                QString id = nm.captured(1).trimmed();
                if (!id.isEmpty()) out[sections[i].first.first].append(id);
            }
        }
        return out;
    }

    int maxNumericId(const QString &text) const {
        int maxId = -1;
        QRegularExpression re(R"(\[(\d+)\])");
        auto it = re.globalMatch(text);
        while (it.hasNext()) maxId = std::max(maxId, it.next().captured(1).toInt());
        return maxId;
    }

    QString appendGeometry(QString text, int containmentId, const QList<int> &newAppletIds) const {
        if (newAppletIds.isEmpty()) return text;
        QStringList geom;
        int i=0;
        for (int id : newAppletIds) {
            int x = 100 + (i % 8) * 40;
            int y = 100 + (i % 8) * 40 + (i / 8) * 60;
            geom << QString("Applet-%1:%2,%3,448,448,0;").arg(id).arg(x).arg(y);
            ++i;
        }
        QString extra = geom.join("");
        QRegularExpression header(QString(R"(^\[Containments\]\[%1\]\s*$)").arg(containmentId), QRegularExpression::MultilineOption);
        auto hm = header.match(text);
        if (!hm.hasMatch()) return text;
        int blockEnd = text.indexOf(QRegularExpression(R"(^\[)", QRegularExpression::MultilineOption), hm.capturedEnd());
        if (blockEnd < 0) blockEnd = text.size();
        QString block = text.mid(hm.capturedStart(), blockEnd-hm.capturedStart());
        QStringList keys = {"ItemGeometries-1920x1080", "ItemGeometriesHorizontal"};
        for (const QString &key : keys) {
            QRegularExpression lineRe(QString(R"(^%1=(.*)$)").arg(QRegularExpression::escape(key)), QRegularExpression::MultilineOption);
            auto lm = lineRe.match(block);
            if (lm.hasMatch()) {
                int absStart = hm.capturedStart() + lm.capturedStart(1);
                text.insert(absStart + lm.capturedLength(1), extra);
                block.insert(lm.capturedStart(1) + lm.capturedLength(1), extra);
            } else {
                int insertPos = hm.capturedEnd();
                text.insert(insertPos, "\n" + key + "=" + extra);
            }
        }
        return text;
    }

    bool writeBackup(const QString &src, QString *backupPath) const {
        const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
        QString dst = src + ".bak-" + stamp;
        if (!QFile::copy(src, dst)) return false;
        if (backupPath) *backupPath = dst;
        return true;
    }

    void reload() {
        const QString text = readFile(configPath());
        const auto files = noteFiles();
        const auto groups = notesByContainment(text);
        m_target->clear();
        m_target->setVisible(groups.isEmpty());
        if (groups.isEmpty()) {
            for (const auto &c : desktopContainments(text)) {
                m_target->addItem(QString("Screen %1 – Containment %2").arg(c.lastScreen).arg(c.id), c.id);
            }
        }
        int configured = 0;
        for (auto it = groups.begin(); it != groups.end(); ++it) configured += it.value().size();
        m_info->setText(QString("Saved Notes: %1\nconfigure Notes: %2").arg(files.size()).arg(configured));
    }

    void restore() {
        QString text = readFile(configPath());
        if (text.isEmpty()) {
            QMessageBox::critical(this, "Error", "Can not read config file.");
            return;
        }
        QStringList files = noteFiles();
        if (files.isEmpty()) {
            QMessageBox::information(this, "no notes", "No files found in the plasma_notes directory.");
            return;
        }

        auto groups = notesByContainment(text);
        int targetContainment = -1;
        QStringList existing;
        if (!groups.isEmpty()) {
            int bestCount = -1;
            for (auto it = groups.begin(); it != groups.end(); ++it) {
                if (it.value().size() > bestCount) {
                    bestCount = it.value().size();
                    targetContainment = it.key();
                    existing = it.value();
                }
            }
        } else {
            if (m_target->count() == 0) {
                QMessageBox::critical(this, "Error", "Kein geeignetes Desktop-Containment gefunden.");
                return;
            }
            targetContainment = m_target->currentData().toInt();
        }

        QStringList missing;
        for (const QString &f : files) if (!existing.contains(f)) missing << f;
        if (missing.isEmpty()) {
            QMessageBox::information(this, "Ready", "All saved Notes are already restored.");
            return;
        }

        QString backup;
        if (!writeBackup(configPath(), &backup)) {
            QMessageBox::critical(this, "Error", "Could not backup the config file.");
            return;
        }

        int nextId = maxNumericId(text) + 1;
        QList<int> newIds;
        QString append;
        for (const QString &noteId : missing) {
            int id = nextId++;
            newIds << id;
            append += QString("\n[Containments][%1][Applets][%2]\n"
                              "immutability=1\n"
                              "plugin=org.kde.plasma.notes\n\n"
                              "[Containments][%1][Applets][%2][Configuration][General]\n"
                              "cursorPosition=0\n"
                              "fontSize=11\n"
                              "noteId=%3\n")
                          .arg(targetContainment).arg(id).arg(noteId);
        }
        text = appendGeometry(text, targetContainment, newIds);
        text += append;

        QSaveFile out(configPath());
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Could not open the config file for writing.");
            return;
        }
        out.write(text.toUtf8());
        if (!out.commit()) {
            QMessageBox::critical(this, "Fehler", "Die Plasma-Konfiguration konnte nicht sicher gespeichert werden.");
            return;
        }
        m_status->setText(QString("%1 Notes restored").arg(missing.size()));
        QMessageBox::information(this, "Ready",
            QString("%1 Notes have been restored.\n\nBackup:\n%2\n\nplasmashell has to reload the configuration .")
                .arg(missing.size()).arg(backup));
        reload();
    }

    QLabel *m_info = nullptr;
    QComboBox *m_target = nullptr;
    QPushButton *m_button = nullptr;
    QPushButton *m_openConfigButton = nullptr;
    QPushButton *m_openNotesButton = nullptr;
    QLabel *m_status = nullptr;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    RestoreWidget w;
    w.show();
    return app.exec();
}
