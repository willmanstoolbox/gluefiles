#include <QApplication>
#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QFileSystemModel>
#include <QListWidget>
#include <QAbstractItemView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QDragMoveEvent>
#include <QDir>
#include <QFile>
#include <QProgressBar>
#include <QLineEdit>
#include <QCheckBox>
#include <QHeaderView>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QCoreApplication>
#include <QIcon>
#include <QGuiApplication>
#include <algorithm>

static constexpr const char* kDefaultOutputName = "output.txt";

class FileListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit FileListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setAcceptDrops(true);
        setDragDropMode(QAbstractItemView::InternalMove);
        setMinimumHeight(220);
    }

    QList<QString> currentPaths() const {
        QList<QString> out;
        out.reserve(count());
        for (int i = 0; i < count(); ++i) {
            auto* it = item(i);
            out.append(it->data(Qt::UserRole).toString());
        }
        return out;
    }

    void addPaths(const QList<QString>& paths) {
        QSet<QString> existing;
        for (int i = 0; i < count(); ++i)
            existing.insert(item(i)->data(Qt::UserRole).toString());

        for (const QString& raw : paths) {
            QFileInfo fi(raw);
            if (!fi.exists() || !fi.isFile()) continue;
            const QString abs = fi.absoluteFilePath();
            if (existing.contains(abs)) continue;
            existing.insert(abs);

            auto* it = new QListWidgetItem(abs);
            it->setToolTip(abs);
            it->setData(Qt::UserRole, abs);
            addItem(it);
        }
    }

protected:
    void dragEnterEvent(QDragEnterEvent* e) override {
        if (e->mimeData()->hasUrls()) e->acceptProposedAction();
        else QListWidget::dragEnterEvent(e);
    }
    void dragMoveEvent(QDragMoveEvent* e) override {
        if (e->mimeData()->hasUrls()) e->acceptProposedAction();
        else QListWidget::dragMoveEvent(e);
    }
    void dropEvent(QDropEvent* e) override {
        if (e->mimeData()->hasUrls()) {
            QList<QString> paths;
            for (const QUrl& u : e->mimeData()->urls())
                paths.append(u.toLocalFile());
            addPaths(paths);
            e->acceptProposedAction();
        } else QListWidget::dropEvent(e);
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle(tr("Glue Files"));
        resize(1040, 640);

        auto* central = new QWidget(this);
        setCentralWidget(central);
        auto* root = new QVBoxLayout(central);

        root->addWidget(new QLabel(tr("Browse on the left, add to the list on the right. Double-click a file or press “Add Selected →”")));

        auto* split = new QSplitter(Qt::Horizontal, this);
        root->addWidget(split, 1);

        auto* leftPanel = new QWidget(this);
        auto* leftLayout = new QVBoxLayout(leftPanel);

        auto* leftTools = new QHBoxLayout();
        addSelectedBtn_ = new QPushButton(tr("Add Selected →"));
        showHiddenCheck_ = new QCheckBox(tr("Show hidden"));
        leftTools->addWidget(addSelectedBtn_);
        leftTools->addStretch(1);
        leftTools->addWidget(showHiddenCheck_);
        leftLayout->addLayout(leftTools);

        fsModel_ = new QFileSystemModel(this);
        fsModel_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        const QString home = QDir::homePath();
        fsModel_->setRootPath(home);

        fileView_ = new QTreeView(this);
        fileView_->setModel(fsModel_);
        fileView_->setRootIndex(fsModel_->index(home));
        fileView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        fileView_->setSortingEnabled(true);
        fileView_->sortByColumn(0, Qt::AscendingOrder);
        fileView_->setAlternatingRowColors(true);
        fileView_->setAllColumnsShowFocus(true);
        fileView_->header()->setStretchLastSection(true);
        fileView_->setColumnWidth(0, 320);
        fileView_->setColumnWidth(1, 100);
        fileView_->setColumnWidth(2, 120);
        fileView_->setColumnWidth(3, 160);

        leftLayout->addWidget(fileView_);
        split->addWidget(leftPanel);

        connect(fileView_, &QTreeView::doubleClicked, this, [this](const QModelIndex& idx) {
            if (!idx.isValid()) return;
            if (!fsModel_->isDir(idx)) {
                list_->addPaths({ fsModel_->filePath(idx) });
            }
        });

        connect(showHiddenCheck_, &QCheckBox::toggled, this, [this](bool on) {
            auto f = QDir::AllEntries | QDir::NoDotAndDotDot;
            if (on) f |= QDir::Hidden;
            fsModel_->setFilter(f);
        });

        connect(addSelectedBtn_, &QPushButton::clicked, this, &MainWindow::onAddSelectedFromBrowser);

        auto* rightPanel = new QWidget(this);
        auto* right = new QVBoxLayout(rightPanel);

        list_ = new FileListWidget();
        right->addWidget(list_);

        {
            auto* row = new QHBoxLayout();
            auto* btnJson = new QPushButton(tr("Add from JSON…"));
            connect(btnJson, &QPushButton::clicked, this, &MainWindow::onAddFromJson);
            row->addWidget(btnJson);

            auto* btnRemove = new QPushButton(tr("Remove Selected"));
            connect(btnRemove, &QPushButton::clicked, this, &MainWindow::onRemoveSelected);
            row->addWidget(btnRemove);

            auto* btnClear = new QPushButton(tr("Clear"));
            connect(btnClear, &QPushButton::clicked, list_, &QListWidget::clear);
            row->addWidget(btnClear);

            row->addStretch(1);
            right->addLayout(row);
        }

        {
            auto* row = new QHBoxLayout();
            row->addWidget(new QLabel(tr("Output folder:")));
            outDirEdit_ = new QLineEdit(QDir::homePath());
            row->addWidget(outDirEdit_, 1);
            auto* btnBrowse = new QPushButton(tr("Browse…"));
            connect(btnBrowse, &QPushButton::clicked, this, &MainWindow::onBrowseOutDir);
            row->addWidget(btnBrowse);
            right->addLayout(row);
        }

        {
            auto* row = new QHBoxLayout();
            row->addWidget(new QLabel(tr("Output filename:")));
            outNameEdit_ = new QLineEdit(kDefaultOutputName);
            row->addWidget(outNameEdit_, 1);
            right->addLayout(row);
        }

        {
            auto* row = new QHBoxLayout();
            groupByFolderCheck_ = new QCheckBox(tr("Group by folder (sort by directory)"));
            groupByFolderCheck_->setChecked(true);
            row->addWidget(groupByFolderCheck_);
            row->addStretch(1);
            right->addLayout(row);
        }

        {
            auto* row = new QHBoxLayout();
            includePathHeaderCheck_ = new QCheckBox(tr("Include file path header"));
            includePathHeaderCheck_->setChecked(false);
            includePathHeaderCheck_->setToolTip(tr("When enabled, each file’s absolute path is written above its contents."));
            row->addWidget(includePathHeaderCheck_);
            row->addStretch(1);
            right->addLayout(row);
        }

        {
            auto* row = new QHBoxLayout();
            progress_ = new QProgressBar();
            progress_->setRange(0, 100);
            progress_->setValue(0);
            row->addWidget(progress_, 1);

            auto* btnBuild = new QPushButton(tr("Build Output"));
            connect(btnBuild, &QPushButton::clicked, this, &MainWindow::onBuild);
            row->addWidget(btnBuild);
            right->addLayout(row);
        }

        status_ = new QLabel("");
        right->addWidget(status_);

        split->addWidget(rightPanel);
        split->setStretchFactor(0, 1);
        split->setStretchFactor(1, 1);
    }

private slots:
    void onAddSelectedFromBrowser() {
        QList<QString> paths;
        const auto rows = fileView_->selectionModel()->selectedRows(0);
        for (const QModelIndex& idx : rows) {
            if (!fsModel_->isDir(idx))
                paths.append(fsModel_->filePath(idx));
        }
        if (!paths.isEmpty())
            list_->addPaths(paths);
    }

    void onAddFromJson() {
        bool ok = false;
        const QString text = QInputDialog::getMultiLineText(
            this, tr("Paste JSON array"),
            tr("Example:\n[\n  \"/path/a.js\",\n  \"/path/b.ts\"\n]"),
            QString(), &ok);
        if (!ok || text.trimmed().isEmpty()) return;

        QJsonParseError perr{};
        const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &perr);
        if (perr.error != QJsonParseError::NoError) {
            QMessageBox::critical(this, tr("Invalid JSON"), perr.errorString());
            return;
        }
        QList<QString> paths;
        if (doc.isArray()) {
            for (const auto& v : doc.array()) paths.append(v.toString());
        } else if (doc.isObject() && doc.object().contains("files")) {
            for (const auto& v : doc.object().value("files").toArray()) paths.append(v.toString());
        } else {
            QMessageBox::critical(this, tr("Invalid JSON"), tr("Must be an array of paths or {\"files\": [...]}."));
            return;
        }
        list_->addPaths(paths);
    }

    void onRemoveSelected() {
        const auto selected = list_->selectedItems();
        for (QListWidgetItem* it : selected) delete it;
    }

    void onBrowseOutDir() {
        const QString d = QFileDialog::getExistingDirectory(this, tr("Choose output folder"), outDirEdit_->text());
        if (!d.isEmpty()) outDirEdit_->setText(d);
    }

    static bool byFolderThenName(const QString& a, const QString& b) {
        QFileInfo fa(a), fb(b);
        const int cmp = QString::compare(fa.absolutePath(), fb.absolutePath(), Qt::CaseInsensitive);
        if (cmp < 0) return true;
        if (cmp > 0) return false;
        return QString::compare(fa.fileName(), fb.fileName(), Qt::CaseInsensitive) < 0;
    }

    void onBuild() {
        QList<QString> paths = list_->currentPaths();
        if (paths.isEmpty()) {
            QMessageBox::warning(this, tr("No files"), tr("Add some files first."));
            return;
        }

        QDir outDir(outDirEdit_->text());
        if (!outDir.exists() && !outDir.mkpath(".")) {
            QMessageBox::critical(this, tr("Cannot create folder"), outDir.absolutePath());
            return;
        }

        QString name = outNameEdit_->text().trimmed();
        if (name.isEmpty()) name = kDefaultOutputName;
        if (name.contains('/')) {
            QMessageBox::warning(this, tr("Invalid name"), tr("Filename must not contain path separators."));
            return;
        }

        if (groupByFolderCheck_->isChecked())
            std::sort(paths.begin(), paths.end(), byFolderThenName);

        const QString outPath = outDir.filePath(name);
        QFile out(outPath);
        if (!out.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Write failed"), out.errorString());
            return;
        }

        progress_->setRange(0, paths.size());
        progress_->setValue(0);

        const QByteArray nl("\n");
        for (int i = 0; i < paths.size(); ++i) {
            const QString& p = paths[i];

            if (includePathHeaderCheck_->isChecked()) {
                out.write(p.toUtf8());
                out.write(nl);
                out.write(nl);
            }

            QFile in(p);
            if (in.open(QIODevice::ReadOnly)) {
                const QByteArray data = in.readAll();
                out.write(data);
                if (data.isEmpty() || (!data.endsWith('\n') && !data.endsWith('\r'))) {
                    out.write(nl);
                }
            } else {
                QByteArray err = QByteArray("<ERROR reading ") + p.toUtf8()
                               + QByteArray(": ") + in.errorString().toUtf8() + QByteArray(">\n");
                out.write(err);
            }

            out.write(nl);
            progress_->setValue(i + 1);
        }

        out.close();
        status_->setText(tr("Wrote %1 files to %2").arg(paths.size()).arg(outPath));
        QMessageBox::information(this, tr("Done"), tr("Output written to:\n%1").arg(outPath));
    }

private:
    QFileSystemModel* fsModel_;
    QTreeView*        fileView_;
    QPushButton*      addSelectedBtn_;
    QCheckBox*        showHiddenCheck_;

    FileListWidget*   list_;
    QLineEdit*        outDirEdit_;
    QLineEdit*        outNameEdit_;
    QCheckBox*        groupByFolderCheck_;
    QCheckBox*        includePathHeaderCheck_;
    QProgressBar*     progress_;
    QLabel*           status_;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Glue Files"));
    QApplication::setApplicationDisplayName(QStringLiteral("Glue Files"));
    QApplication::setOrganizationName(QStringLiteral("SQ C"));
    QGuiApplication::setDesktopFileName(QStringLiteral("gluefiles"));
    QTranslator qtTr;
    qtTr.load(QLocale::system(), "qtbase", "_",
              QLibraryInfo::path(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTr);

    QTranslator appTr;
    const QLocale sys = QLocale::system();
    if (sys.language() == QLocale::Spanish) {
        const QStringList candidates = {
            "gluefiles_" + sys.name(),
            "gluefiles_es"
        };
        auto tryLoad = [&](const QString& base) -> bool {
            if (appTr.load(":/i18n/" + base + ".qm")) return true;
            if (appTr.load(base, QCoreApplication::applicationDirPath() + "/i18n")) return true;
            return false;
        };
        for (const QString& base : candidates) {
            if (tryLoad(base)) { app.installTranslator(&appTr); break; }
        }
    }

    app.setWindowIcon(QIcon(":/icons/icon.svg"));

    MainWindow w;
    w.show();
    return app.exec();
}

#include <QObject>
#include "gluefiles.moc"
