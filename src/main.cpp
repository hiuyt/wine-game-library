#include <QApplication>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QStyle>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

struct Game {
    QString id;
    QString name;
    QString exe;
};

class Launcher final : public QWidget {
public:
    Launcher() {
        setWindowTitle(tr("Wine 游戏库"));
        resize(960, 680);
        setMinimumSize(720, 500);
        setStyleSheet(R"(
            QWidget { font-size: 14px; }
            QLabel#title { font-size: 26px; font-weight: 700; }
            QLabel#subtitle { color: palette(mid); }
            QFrame#panel { background: palette(base); border: 1px solid palette(midlight); border-radius: 10px; }
            QListWidget { border: 0; background: transparent; outline: 0; }
            QListWidget::item { padding: 10px 12px; border-radius: 7px; }
            QListWidget::item:selected { background: palette(highlight); color: palette(highlighted-text); }
            QPushButton { min-height: 32px; padding: 2px 12px; }
            QPushButton#primary { background: palette(highlight); color: palette(highlighted-text); font-weight: 600; }
            QTextEdit { border: 1px solid palette(midlight); border-radius: 8px; font-family: monospace; }
        )");

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(22, 18, 22, 20);
        layout->setSpacing(14);
        auto* header = new QHBoxLayout;
        auto* heading = new QVBoxLayout;
        auto* title = new QLabel(tr("Wine 游戏库"));
        title->setObjectName("title");
        auto* subtitle = new QLabel(tr("系统 Wine · 独立 Prefix · 可选沙箱"));
        subtitle->setObjectName("subtitle");
        heading->addWidget(title);
        heading->addWidget(subtitle);
        auto* add = new QPushButton(tr("添加游戏…"));
        auto* remove = new QPushButton(tr("移除条目"));
        add->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
        remove->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
        sandbox_ = new QCheckBox(tr("启用沙箱保护"));
        sandbox_->setChecked(true);
        header->addLayout(heading);
        header->addStretch();
        header->addWidget(sandbox_);
        header->addWidget(remove);
        header->addWidget(add);
        layout->addLayout(header);

        auto* libraryPanel = new QFrame;
        libraryPanel->setObjectName("panel");
        auto* libraryLayout = new QVBoxLayout(libraryPanel);
        libraryLayout->setContentsMargins(10, 10, 10, 10);
        games_ = new QListWidget;
        games_->setAlternatingRowColors(true);
        games_->setSpacing(3);
        games_->setViewMode(QListView::IconMode);
        games_->setIconSize(QSize(88, 88));
        games_->setGridSize(QSize(150, 126));
        games_->setResizeMode(QListView::Adjust);
        games_->setMovement(QListView::Static);
        games_->setWordWrap(true);
        libraryLayout->addWidget(games_);
        layout->addWidget(libraryPanel, 2);

        auto* actions = new QHBoxLayout;
        for (const auto& action : QList<QPair<QString, QString>>{
                 {tr("启动游戏"), "run"}, {tr("Wine 设置"), "winecfg"},
                 {tr("安装组件"), "winetricks"}, {tr("打开 Prefix"), "open"},
                 {tr("打开日志文件"), "log"}}) {
            auto* button = new QPushButton(action.first);
            if (action.second == "run") {
                button->setObjectName("primary");
                button->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
                button->setDefault(true);
            } else if (action.second == "open" || action.second == "log") {
                button->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
            }
            actions->addWidget(button);
            connect(button, &QPushButton::clicked, this, [this, key = action.second] { act(key); });
        }
        actions->addStretch();
        layout->addLayout(actions);

        status_ = new QLabel(tr("就绪 · 使用系统 Wine 11.0"));
        status_->setObjectName("subtitle");
        output_ = new QTextEdit;
        output_->setReadOnly(true);
        output_->setPlaceholderText(tr("本次运行输出会显示在这里…"));
        auto* logHeader = new QHBoxLayout;
        auto* logTitle = new QLabel(tr("运行日志"));
        QFont logTitleFont = logTitle->font();
        logTitleFont.setBold(true);
        logTitle->setFont(logTitleFont);
        logHeader->addWidget(logTitle);
        logHeader->addStretch();
        logHeader->addWidget(status_);
        layout->addLayout(logHeader);
        layout->addWidget(output_, 1);

        connect(add, &QPushButton::clicked, this, [this] { addGame(); });
        connect(remove, &QPushButton::clicked, this, [this] { removeGame(); });
        load();
    }

private:
    QString dataRoot() const {
        return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    }

    QString prefix(const Game& game) const { return dataRoot() + "/prefixes/" + game.id; }
    QString logPath(const Game& game) const { return dataRoot() + "/logs/" + game.id + ".log"; }

    void load() {
        QSettings settings;
        const int count = settings.beginReadArray("games");
        for (int i = 0; i < count; ++i) {
            settings.setArrayIndex(i);
            Game game{settings.value("id").toString(), settings.value("name").toString(),
                      settings.value("exe").toString()};
            if (!game.id.isEmpty()) {
                game.name = QFileInfo(game.exe).dir().dirName();
                addItem(game);
            }
        }
        settings.endArray();
        save();
    }

    void save() const {
        QSettings settings;
        settings.beginWriteArray("games");
        for (int i = 0; i < games_->count(); ++i) {
            settings.setArrayIndex(i);
            const auto game = itemGame(games_->item(i));
            settings.setValue("id", game.id);
            settings.setValue("name", game.name);
            settings.setValue("exe", game.exe);
        }
        settings.endArray();
    }

    void addItem(const Game& game) {
        auto* item = new QListWidgetItem(gameIcon(game), game.name, games_);
        item->setToolTip(game.exe);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
        item->setData(Qt::UserRole, game.id);
        item->setData(Qt::UserRole + 1, game.name);
        item->setData(Qt::UserRole + 2, game.exe);
    }

    QIcon gameIcon(const Game& game) const {
        const QString iconDir = dataRoot() + "/icons/" + game.id;
        QDir dir(iconDir);
        auto icons = dir.entryList({"*.ico"}, QDir::Files);
        if (icons.isEmpty()) {
            QDir().mkpath(iconDir);
            QProcess::execute("/usr/bin/wrestool", {"-x", "-t", "14", "-o", iconDir, game.exe});
            icons = dir.entryList({"*.ico"}, QDir::Files);
        }
        return icons.isEmpty() ? style()->standardIcon(QStyle::SP_ComputerIcon)
                               : QIcon(dir.filePath(icons.first()));
    }

    static Game itemGame(const QListWidgetItem* item) {
        return {item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString(),
                item->data(Qt::UserRole + 2).toString()};
    }

    void addGame() {
        const QString root = QDir::homePath() + "/galgame";
        const QString exe = QFileDialog::getOpenFileName(this, tr("选择游戏 EXE"), root,
                                                         tr("Windows 程序 (*.exe *.EXE)"));
        if (exe.isEmpty()) return;
        const QString canonical = QFileInfo(exe).canonicalFilePath();
        if (canonical.isEmpty()) {
            QMessageBox::warning(this, tr("文件无效"), tr("无法读取所选 EXE。"));
            return;
        }
        for (int i = 0; i < games_->count(); ++i) {
            if (itemGame(games_->item(i)).exe == canonical) return;
        }
        const QString id = QString::fromLatin1(
            QCryptographicHash::hash(canonical.toUtf8(), QCryptographicHash::Sha256).toHex().left(16));
        addItem({id, QFileInfo(canonical).dir().dirName(), canonical});
        games_->setCurrentRow(games_->count() - 1);
        save();
    }

    void removeGame() {
        auto* item = games_->currentItem();
        if (!item) return;
        if (QMessageBox::question(this, tr("移除条目"),
                                  tr("只移除启动器条目；游戏和 Prefix 都会保留。")) == QMessageBox::Yes) {
            delete item;
            save();
        }
    }

    void act(const QString& action) {
        auto* item = games_->currentItem();
        if (!item) {
            QMessageBox::information(this, tr("未选择游戏"), tr("请先选择一个游戏条目。"));
            return;
        }
        const Game game = itemGame(item);
        QDir().mkpath(prefix(game));
        QDir().mkpath(QFileInfo(logPath(game)).absolutePath());
        if (action == "open") {
            QDesktopServices::openUrl(QUrl::fromLocalFile(prefix(game)));
            return;
        }
        if (action == "log") {
            QDesktopServices::openUrl(QUrl::fromLocalFile(logPath(game)));
            return;
        }

        if (action == "run" && !QFileInfo::exists(prefix(game) + "/.launcher-ready-v4")) {
            status_->setText(tr("首次使用：正在初始化 Wine 组件…"));
            QApplication::processEvents();
            QList<QPair<QString, QStringList>> setup{
                {"/usr/bin/wineboot", {"--init"}},
                {"/usr/bin/wine", {"regsvr32", "/s", "mmdevapi.dll"}},
                {"/usr/bin/wine", {"regsvr32", "/s", "devenum.dll"}},
                {"/usr/bin/wine", {"regsvr32", "/s", "quartz.dll"}},
                {"/usr/bin/wine", {"regsvr32", "/s", "evr.dll"}},
                {"/usr/bin/wine", {"reg", "add", "HKCU\\Software\\Wine\\Drivers", "/v", "Audio",
                                    "/t", "REG_SZ", "/d", "pulse", "/f"}}};
            const QList<QPair<QString, QString>> fonts{
                {"MS Gothic", "Noto Sans CJK JP"}, {"MS PGothic", "Noto Sans CJK JP"},
                {"MS UI Gothic", "Noto Sans CJK JP"}, {"Meiryo", "Noto Sans CJK JP"},
                {"Yu Gothic", "Noto Sans CJK JP"}, {"MS Mincho", "Noto Serif CJK JP"},
                {"MS PMincho", "Noto Serif CJK JP"}, {"SimSun", "Noto Sans CJK SC"},
                {"NSimSun", "Noto Sans CJK SC"}, {"Microsoft YaHei", "Noto Sans CJK SC"}};
            for (const auto& font : fonts) {
                setup.append({"/usr/bin/wine",
                              {"reg", "add", "HKCU\\Software\\Wine\\Fonts\\Replacements",
                               "/v", font.first, "/t", "REG_SZ", "/d", font.second, "/f"}});
            }
            auto runSetup = [this, &game](QString program, QStringList args) {
                if (sandbox_->isChecked()) {
                    args = sandboxArgs(game, program, args);
                    program = "/usr/bin/bwrap";
                }
                QProcess init;
                auto env = QProcessEnvironment::systemEnvironment();
                env.insert("WINEPREFIX", prefix(game));
                init.setProcessEnvironment(env);
                init.start(program, args);
                return init.waitForFinished(60000) && init.exitCode() == 0;
            };
            bool setupOk = true;
            if (sandbox_->isChecked()) {
                QString script = "set -e; wineboot --init";
                for (qsizetype i = 1; i < setup.size(); ++i) {
                    QStringList quoted;
                    for (const auto& arg : setup[i].second) quoted << "'" + arg + "'";
                    script += "; '" + setup[i].first + "' " + quoted.join(' ');
                }
                script += "; wineserver -k; wineserver -w";
                setupOk = runSetup("/bin/sh", {"-c", script});
            } else {
                for (const auto& command : setup) {
                    if (!runSetup(command.first, command.second)) {
                        setupOk = false;
                        break;
                    }
                }
            }
            if (!setupOk) {
                QMessageBox::warning(this, tr("Prefix 初始化失败"),
                                     tr("Wine 组件初始化失败，请重试或查看日志。"));
                return;
            }
            QFile marker(prefix(game) + "/.launcher-ready-v4");
            if (!marker.open(QIODevice::WriteOnly)) {
                QMessageBox::warning(this, tr("Prefix 初始化失败"), tr("无法保存初始化状态。"));
                return;
            }
        }

        QString program = action == "winetricks" ? "/usr/bin/winetricks" : "/usr/bin/wine";
        QStringList args;
        if (action == "run") args << game.exe;
        if (action == "winecfg") args << "winecfg";

        if (sandbox_->isChecked()) {
            args = sandboxArgs(game, program, args);
            program = "/usr/bin/bwrap";
        }
        start(game, program, args);
    }

    QStringList sandboxArgs(const Game& game, const QString& program, const QStringList& commandArgs) const {
        const QString gameDir = QFileInfo(game.exe).absolutePath();
        const QString prefixDir = prefix(game);
        const QString home = QDir::homePath();
        QStringList args{"--die-with-parent", "--new-session", "--unshare-all", "--share-net",
                         "--ro-bind", "/usr", "/usr", "--ro-bind", "/etc", "/etc",
                         "--ro-bind", "/var", "/var", "--ro-bind", "/sys", "/sys",
                         "--symlink", "usr/bin", "/bin", "--symlink", "usr/lib64", "/lib64",
                         "--dev", "/dev", "--proc", "/proc", "--tmpfs", "/tmp",
                         "--tmpfs", "/home", "--dir", home};
        for (const QString& deviceDir : {QStringLiteral("/dev/dri"), QStringLiteral("/dev/snd")}) {
            if (QFileInfo::exists(deviceDir)) args << "--dev-bind" << deviceDir << deviceDir;
        }
        const auto env = QProcessEnvironment::systemEnvironment();
        const QString runtime = env.value("XDG_RUNTIME_DIR");
        if (!runtime.isEmpty()) {
            args << "--dir" << "/run" << "--dir" << "/run/user" << "--dir" << runtime;
            for (const QString& socket : {env.value("WAYLAND_DISPLAY"), QStringLiteral("pipewire-0")}) {
                const QString path = runtime + "/" + socket;
                if (!socket.isEmpty() && QFileInfo::exists(path)) args << "--bind" << path << path;
            }
            const QString pulse = runtime + "/pulse";
            if (QFileInfo::exists(pulse)) args << "--bind" << pulse << pulse;
        }
        const QString xauthority = env.value("XAUTHORITY");
        if (!xauthority.isEmpty() && QFileInfo::exists(xauthority)) args << "--ro-bind" << xauthority << xauthority;
        if (QFileInfo::exists("/tmp/.X11-unix")) args << "--ro-bind" << "/tmp/.X11-unix" << "/tmp/.X11-unix";
        auto addParents = [&args, &home](const QString& path) {
            QString current;
            const auto parts = QFileInfo(path).absolutePath().split('/', Qt::SkipEmptyParts);
            for (const auto& part : parts) {
                current += "/" + part;
                if (current != "/home" && current != home) args << "--dir" << current;
            }
        };
        addParents(gameDir);
        addParents(prefixDir);
        args << "--bind" << gameDir << gameDir << "--bind" << prefixDir << prefixDir
             << "--chdir" << gameDir << "--setenv" << "HOME" << home
             << "--setenv" << "WINEPREFIX" << prefixDir << "--" << program;
        args << commandArgs;
        return args;
    }

    void start(const Game& game, const QString& program, const QStringList& args) {
        auto* process = new QProcess(this);
        auto env = QProcessEnvironment::systemEnvironment();
        env.insert("WINEPREFIX", prefix(game));
        process->setProcessEnvironment(env);
        process->setWorkingDirectory(QFileInfo(game.exe).absolutePath());
        process->setProcessChannelMode(QProcess::MergedChannels);
        output_->clear();
        output_->append(tr("启动：%1 %2").arg(program, args.join(' ')));
        status_->setText(tr("运行中：%1").arg(game.name));
        connect(process, &QProcess::readyRead, this, [this, process] {
            output_->append(QString::fromLocal8Bit(process->readAll()));
        });
        connect(process, &QProcess::finished, this, [this, process, game](int code, QProcess::ExitStatus) {
            const QString text = output_->toPlainText();
            const QString path = logPath(game);
            if (QFileInfo(path).size() >= 1024 * 1024) {
                QFile::remove(path + ".1");
                QFile::rename(path, path + ".1");
            }
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                file.write(("\n=== " + QDateTime::currentDateTime().toString(Qt::ISODate) + " ===\n").toUtf8());
                file.write(text.toUtf8());
            }
            status_->setText(tr("已结束（退出码 %1），日志已保存。").arg(code));
            process->deleteLater();
        });
        process->start(program, args);
    }

    QListWidget* games_{};
    QCheckBox* sandbox_{};
    QLabel* status_{};
    QTextEdit* output_{};
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("Local");
    QApplication::setApplicationName("SystemWineLauncher");
    QApplication::setWindowIcon(QIcon(":/icons/assets/wine-game-library.png"));
    Launcher launcher;
    launcher.show();
    return app.exec();
}
